
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

static void rs_free_redis_thread(void *data);
static int rs_redis_dml_data(rs_slave_info_t *si, char *buf, uint32_t len);

void *rs_start_redis_thread(void *data) 
{
    int                         r, n, i, s;
    char                        *p;
    rs_ring_buffer2_data_t      *d;
    rs_slave_info_t             *si;

    s = 0;
    si = (rs_slave_info_t *) data;

    /* push cleanup handle */
    pthread_cleanup_push(rs_free_redis_thread, (void *) si);

    if(si == NULL) {
        rs_log_err(0, "redis thread can not get slave info struct");
        goto free;
    }

    for( ;; ) {

        r = rs_get_ring_buffer2(si->ring_buf, &d);

        if(r == RS_ERR) {
            rs_log_err(0, "rs_get_ring_buffer() failed, get dump data");
            goto free;
        }

        if(r == RS_EMPTY) {

            /* commit redis cmd */
            if(si->cmdn) {
                if(rs_redis_get_replies(si, si->cmdn) != RS_OK) {
                    goto free;
                }

                si->cmdn = 0;
            }

            si->cur_binlog_save++;

            for (n = 1; n < RS_RING_BUFFER_SPIN; n <<= 1) {

                for (i = 0; i < n; i++) {
                    rs_cpu_pause();
                }

                r = rs_get_ring_buffer2(si->ring_buf, &d);

                if(r == RS_OK) {
                    break;
                }
            }

            if(r != RS_OK) {
                if(s % 60000000 == 0) {
                    s = 0;
                    rs_log_info("redis thread wait ring buffer fill data");
                }

                s += RS_RING_BUFFER_EMPTY_SLEEP_USEC;

                if(rs_flush_slave_info(si) != RS_OK) {
                    goto free;
                }

                usleep(RS_RING_BUFFER_EMPTY_SLEEP_USEC);
                continue;
            }
        }

        p = d->data;

        /* get flush info */
        if((p = rs_strchr(p, ',')) == NULL) {
            rs_log_err(0, "rs_strchr(\"%s\", ',') failed", p);
            goto free;
        }

        rs_memcpy(si->dump_file, d->data, p - (char *) d->data);

        p++; /* NOTICE : file,pos */

        if((p = rs_strchr(p, ',')) == NULL) {
            rs_log_err(0, "rs_strchr(\"%s\", ',') failed", p);
            goto free;
        }

        si->dump_pos = rs_estr_to_uint32(p - 1);

        /* COMMIT TO REDIS */
        if(rs_redis_dml_data(si, p, d->len - (p - (char *) d->data)) 
                != RS_OK) 
        {
            goto free;
        }

        si->cur_binlog_save++;

        if(rs_flush_slave_info(si) != RS_OK) {
            goto free;
        }

        rs_get_ring_buffer2_advance(si->ring_buf);
    } 

free:;

    /* pop cleanup handle and execute */
    pthread_cleanup_pop(1);

    pthread_exit(NULL);
}

static int rs_redis_dml_data(rs_slave_info_t *si, char *buf, uint32_t len) 
{/*{{{*/
    char                *p, t, *e;
    uint32_t            tl, rl;

    p = buf;
    t = 0;
    tl = 0;
    rl = 0;
    e =  NULL;

    p++; /* NOTICE : pos,mev */
    t = *p++;

    if(t == RS_MYSQL_SKIP_DATA) {
        return RS_OK;

    } else if(t == RS_WRITE_ROWS_EVENT || t == RS_UPDATE_ROWS_EVENT || 
            t == RS_DELETE_ROWS_EVENT) 
    {
        p++; /* NOTICE : mev,db.tabletran */

        if((e = rs_strchr(p, ',')) == NULL) {
            rs_log_err(0, "unkown cmd format");
            return RS_ERR;
        }

        tl = e++ - p;
        rl = len - 4 - tl;

        if(rs_tables_handle(si, p, tl, e, rl, t) != RS_OK) {
            return RS_ERR;
        }

        /* commit redis cmd */
        if(si->cmdn >= RS_REDIS_CMD_COMMIT_NUM) {
            if(rs_redis_get_replies(si, si->cmdn) != RS_OK) {
                return RS_ERR;
            }

            si->cmdn = 0;
        }

    } else {
        rs_log_err(0, "unkonw cmd type = %d", t);
        return RS_ERR;
    }

    return RS_OK;
}/*}}}*/

static void rs_free_redis_thread(void *data)
{/*{{{*/
    rs_slave_info_t *si;

    si = (rs_slave_info_t *) data;

    if(si != NULL) {
        rs_log_info("set redis thread exit state");
        si->redis_thread_exit = 1;
        if(rs_quit == 0 && rs_reload == 0) {
            rs_log_info("redis thread send SIGQUIT signal");
            kill(rs_pid, SIGQUIT);
        }
    }
}/*}}}*/
