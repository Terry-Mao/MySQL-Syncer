
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>


int rs_def_filter_data_handle(rs_request_dump_t *rd)
{
    return RS_OK;
}

int rs_def_create_data_handle(rs_request_dump_t *rd) 
{
    int                     i, r, len;
    char                    *p, istr[UINT32_LEN + 1];
    rs_binlog_info_t        *bi;
    rs_ring_buffer2_data_t  *d;
    rs_slab_t               *sl;

    if(rd == NULL) {
        return RS_ERR;
    }

    i = 0;
    bi = &(rd->binlog_info);
    sl = &(rd->slab);


    if(bi->mev == 0) {
        if(bi->skip_n++ % RS_SKIP_DATA_FLUSH_NUM != 0) {
            bi->skip_n = 1;
            return RS_OK;
        }
    }

    for( ;; ) {

        r = rs_set_ring_buffer2(&(rd->ring_buf), &d);

        if(r == RS_FULL) {
            if(i % 60 == 0) {
                rs_log_info("request dump's ring buffer is full");
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

            /* free slab chunk */
            if(d->data != NULL && d->id >= 0 && d->len > 0) {
                rs_free_slab_chunk(sl, d->data, d->id); 
                rs_ring_buffer2_data_t_init(d);
            }

            /* rs_uint32_to_str(rd->dump_pos, istr); */
            len = snprintf(istr, UINT32_LEN + 1, "%u", rd->dump_pos);
            len += rs_strlen(rd->dump_file) + 1 + 1 + 1; 

            if(bi->mev == 0) {

                d->len = len;
                d->id = rs_slab_clsid(sl, len);
                d->data = rs_alloc_slab_chunk(sl, len, d->id);

                if(d->data == NULL) {
                    return RS_ERR;
                }

                len = snprintf(d->data, d->len, "%s,%s,%c", rd->dump_file, 
                        istr, bi->mev);

                if(len < 0) {
                    rs_log_err(rs_errno, "snprint() failed, dump cmd"); 
                    return RS_ERR;
                }

            } else {

                if(bi->log_format == RS_BINLOG_FORMAT_ROW_BASED) {

                    /* RBR */
                    len += bi->el - RS_BINLOG_EVENT_HEADER_LEN 
                        + rs_strlen(bi->tb) + 3 + rs_strlen(bi->db) + 4 + 
                        bi->cn + 4 + bi->ml;

                    rs_log_debug(0, "CMD LEN = %d", len);

                    d->len = len;
                    d->id = rs_slab_clsid(sl, len);
                    d->data = rs_alloc_slab_chunk(sl, len, d->id);

                    if(d->data == NULL) {
                        return RS_ERR;
                    }

                    len = snprintf(d->data, d->len, "%s,%u,%c,%s.%s,", 
                            rd->dump_file, rd->dump_pos, bi->mev, bi->db, 
                            bi->tb);

                    rs_log_debug(0, "RBR PREFIX LEN = %d", len);

                    if(len < 0) {
                        rs_log_err(rs_errno, "snprint() failed, dump cmd"); 
                        return RS_ERR;
                    }

                    p = (char *) d->data + len;
                    p = rs_cpymem(p, &(bi->cn), 4);
                    p = rs_cpymem(p, bi->ct, bi->cn);
                    p = rs_cpymem(p, &(bi->ml), 4);
                    p = rs_cpymem(p, bi->cm, bi->ml);

                    rs_memcpy(p, bi->data, d->len - len - 8 - bi->cn - bi->ml);
                }
            }

            rs_set_ring_buffer2_advance(&(rd->ring_buf));

            break;
        }

    } // EXIT RING BUFFER 

    bi->flush = 0;
    bi->mev = 0;
    bi->sent = 1;

    return RS_OK;
}
