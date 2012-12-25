
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>


int rs_def_filter_data_handle(rs_reqdump_data_t *rd)
{
    return RS_OK;
}

int rs_binlog_create_data(rs_reqdump_data_t *rd) 
{
    int                     r, len;
    char                    istr[UINT32_LEN + 1], *p;
    rs_binlog_info_t        *bi;
    rs_ringbuf_data_t       *rbd;

    bi = &(rd->binlog_info);
    p = NULL;

    if(bi->mev == 0) {
        if(bi->skip_n++ % RS_SKIP_DATA_FLUSH_NUM != 0) {
            bi->skip_n = 1;
            return RS_OK;
        }
    }

    for( ;; ) {

        r = rs_ringbuf_set(rd->ringbuf, &rbd);

        if(r == RS_FULL) {
            sleep(RS_RING_BUFFER_FULL_SLEEP_SEC);
            continue;
        } else if(r == RS_ERR) {
            return RS_ERR;
        }

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

            len = snprintf(rbd->data, rbd->len, "%s,%s\n%c", rd->dump_file, 
                    istr, bi->mev);

            if(len < 0) {
                rs_log_error(RS_LOG_ERR, rs_errno, "snprintf() failed");
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
                        "%s,%u\n%c,%s.%s%c", 
                        rd->dump_file, rd->dump_pos, bi->mev, bi->db, 
                        bi->tb, 0);

                if(len < 0) {
                    rs_log_error(RS_LOG_ERR, rs_errno, "snprintf() failed");
                    return RS_ERR;
                }

                p = (char *) rbd->data + len;
                p = rs_cpymem(p, &(bi->cn), 4);
                p = rs_cpymem(p, bi->ct, bi->cn);
                p = rs_cpymem(p, &(bi->ml), 4);
                p = rs_cpymem(p, bi->cm, bi->ml);

                rs_memcpy(p, bi->data, rbd->len - len - 8 - bi->cn - 
                        bi->ml);
            }
        }

        rs_ringbuf_set_advance(rd->ringbuf);
        break;

    } // EXIT RING BUFFER 

    bi->flush = 0;
    bi->mev = 0;
    bi->sent = 1;

    return RS_OK;
}
