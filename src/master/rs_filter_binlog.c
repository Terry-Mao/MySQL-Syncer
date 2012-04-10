
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

static char *rs_create_data_event(char *buf, void *data);

int rs_def_filter_data_handle(rs_request_dump_t *rd)
{
    return RS_OK;
}

int rs_def_create_data_handle(rs_request_dump_t *rd) 
{
    int                     i, r, len;
    char                    *p;
    rs_binlog_info_t        *bi;
    rs_ring_buffer_data_t   *d;

    i = 0;
    bi = &(rd->binlog_info);

    for( ;; ) {

        r = rs_set_ring_buffer(&(rd->ring_buf), &d);

        if(r == RS_FULL) {
            if(i % 60 == 0) {
                rs_log_info("request dump's ring buffer is full, wait %u "
                        "secs" ,RS_RING_BUFFER_FULL_SLEEP_SEC);
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
            p = d->data;

            len = snprintf(p, RS_SYNC_DATA_CMD_SIZE, "%s,%u,%c", 
                    rd->dump_file, rd->dump_pos, bi->mev); 

            if(len < 0) {
                rs_log_err(rs_errno, "snprintf() failed");
                return RS_ERR;
            }

            if(bi->mev == 0) {
                d->len = len;
            } else {
                p += len;
                p = rs_create_data_event(p, bi->data);

                if(p != NULL) {
                    d->len = p - d->data;
                    rs_set_ring_buffer_advance(&(rd->ring_buf));
                }
            }

            rs_set_ring_buffer_advance(&(rd->ring_buf));

            break;
        }

    } // EXIT RING BUFFER 

    bi->mev = 0;
    bi->sent = 1;

    return RS_OK;
}

static char *rs_create_data_event(char *buf, void *data)
{
    return buf;
}
