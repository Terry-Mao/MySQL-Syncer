
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>


int rs_def_filter_data_handle(rs_reqdump_data_t *rd)
{
    return RS_OK;
}

int rs_binlog_create_data(rs_reqdump_data_t *rd) 
{
    int                     i, r, len;
    char                    istr[UINT32_LEN + 1];
    rs_binlog_info_t        *bi;
    rs_ringbuf_data_t       *rbd;

    i = 0;
    bi = &(rd->binlog_info);

    if(bi->mev == 0) {
        if(bi->skip_n++ % RS_SKIP_DATA_FLUSH_NUM != 0) {
            bi->skip_n = 1;
            return RS_OK;
        }
    }

    for( ;; ) {

        r = rs_ringbuf_set(rd->ringbuf, &rbd);

        if(r == RS_FULL) {
            if(i % 60 == 0) {
                rs_log_debug(0, "ringbuf is full");
                i = 0;
            }

            i += RS_RING_BUFFER_FULL_SLEEP_SEC;

            sleep(RS_RING_BUFFER_FULL_SLEEP_SEC);
            continue;
        }

        if(r == RS_ERR) {
            return RS_ERR;
        }

        if(r == RS_OK) {

            /* free memory */
            if(rbd->data != NULL) {
                rs_pfree(rd->pool, rbd->data, rbd->id); 
                rs_ringbuf_data_t_init(rbd);
            }

            /* rs_uint32_to_str(rd->dump_pos, istr); */
            len = snprintf(istr, UINT32_LEN + 1, "%u", rd->dump_pos);
            len += rs_strlen(rd->dump_file) + 1 + 1 + 1; 

            if(bi->mev == 0) {

                rbd->len = len;
                rbd->id = rs_palloc_id(rd->pool, len);
                rbd->data = rs_palloc(rd->pool, len, rbd->id);

                if(rbd->data == NULL) {
                    return RS_ERR;
                }

                len = snprintf(rbd->data, rbd->len, "%s,%s,%c", rd->dump_file, 
                        istr, bi->mev);

                if(len < 0) {
                    rs_log_err(rs_errno, "snprintf() failed");
                    return RS_ERR;
                }

            } else {

                if(bi->log_format == RS_BINLOG_FORMAT_ROW_BASED) {

                    /* RBR */
                    len += bi->el - RS_BINLOG_EVENT_HEADER_LEN 
                        + rs_strlen(bi->tb) + 3 + rs_strlen(bi->db) + 4 + 
                        bi->cn + 4 + bi->ml;

                    rbd->len = len;
                    rbd->id = rs_palloc_id(rd->pool, len);
                    rbd->data = rs_palloc(rd->pool, len, rbd->id);

                    if(rbd->data == NULL) {
                        return RS_ERR;
                    }

                    len = snprintf(rbd->data, rbd->len, 
                            "%s,%u\n%c,%s.%s%c%u%s%u%s%s", 
                            rd->dump_file, rd->dump_pos, bi->mev, bi->db, 
                            bi->tb, 0, bi->cn, bi->ct, bi->ml, bi->cm, 
                            (char *) bi->data);

                    if(len < 0) {
                        rs_log_err(rs_errno, "snprintf() failed");
                        return RS_ERR;
                    }
                }
            }

            rs_ringbuf_set_advance(rd->ringbuf);
            break;
        }

    } // EXIT RING BUFFER 

    bi->flush = 0;
    bi->mev = 0;
    bi->sent = 1;

    return RS_OK;
}
