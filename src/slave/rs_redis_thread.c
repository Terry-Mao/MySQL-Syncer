
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

static void rs_free_redis_thread(void *data);
static int rs_redis_dml_data(rs_slave_info_t *si, char *buf, uint32_t len);
static int rs_flush_slave_info(rs_slave_info_t *si, char *buf, uint32_t len);
static int rs_redis_batch_commit(rs_slave_info_t *si);

void *rs_start_redis_thread(void *data) 
{
    int                 err;
    uint32_t            len;
    char                *p;
    rs_ringbuf_data_t   *rbd;
    rs_slave_info_t     *si;

    si = (rs_slave_info_t *) data;

    /* push cleanup handle */
    pthread_cleanup_push(rs_free_redis_thread, (void *) si);

    for( ;; ) {

        err = rs_get_ring_buffer2(si->ring_buf, &rbd);

        if(err == RS_ERR) {
            rs_log_err(0, "rs_get_ring_buffer() failed");
            goto free;
        }

        if(err == RS_EMPTY) {
            /* commit redis cmd */
            if(rs_redis_batch_commit(si) != RS_OK) {
                goto free;
            }

            err = rs_ringbuf_spin_wait(si->ringbuf);

            if(err != RS_OK) {
                if(rs_flush_slave_info(si) != RS_OK) {
                    goto free;
                }

                continue;
            }
        }

        p = rbd->data;

        /* get flush info */
        if((p = rs_strchr(p, '\n')) == NULL) {
            rs_log_err(0, "rs_strchr(\"%s\", '\\n') failed", p);
            goto free;
        }

        len = (p - (char *) rbd->data);

        /* commit to redis */
        if(rs_redis_dml_data(si, p, rbd->len - len) != RS_OK) {
            goto free;
        }

        /* flush slave.info */
        if(rs_flush_slave_info(si, (char *) rbd->data, len) != RS_OK) {
            goto free;
        }

        rs_ringbuf_get_advance(si->ringbuf);
    } 

free:;

    /* pop cleanup handle and execute */
    pthread_cleanup_pop(1);
    pthread_exit(NULL);
}

static int rs_redis_dml_data(rs_slave_info_t *si, char *buf, uint32_t len) 
{
    char                *p, t, *e;
    uint32_t            rl;
    rs_redis_dml_func   handler;

    p = buf;
    t = 0;
    rl = 0;
    e =  NULL;

    p++; /* NOTICE : pos,mev */
    t = *p++;

    if(t == RS_MYSQL_SKIP_DATA) {
        return RS_OK;
    } else if(t == RS_WRITE_ROWS_EVENT || t == RS_UPDATE_ROWS_EVENT || 
            t == RS_DELETE_ROWS_EVENT) 
    {
        p++; /* NOTICE : mev,db.table\0 */
        tl = rs_strlen(p) + 1;
        rl = len - 4 - tl;

        if(rs_shash_get(si->table_func, p, &handler) != RS_OK) {
            return RS_ERR;
        }

        if(handler(si, e, rl, t) != RS_OK) {
            return RS_ERR;
        }

        /* commit redis cmd */
        if(rs_redis_batch_commit(si) != RS_OK) {
            return RS_ERR;
        }

    } else {
        rs_log_err(0, "unkonw cmd type = %d", t);
        return RS_ERR;
    }

    return RS_OK;
}

/*
 *  rs_flush_slave_info
 *  @s:rs_slave_info_s struct
 *
 *  Flush slave into to disk, format like rsylog_path,rsylog_pos
 *
 *  On success, RS_OK is returned. On error, RS_ERR is returned
 */
static int rs_flush_slave_info(rs_slave_info_t *si, char *buf, uint32_t len) 
{
    struct timeval  tv;
    ssize_t         n;

    if(gettimeofday(&tv, NULL) != 0) {
        rs_log_err(rs_errno, "gettimeofday() failed");
        return RS_ERR;
    }

    if(++si->cur_binlog_save < si->binlog_save == 0 && 
            tv.tv_sec - si->cur_binlog_savesec < si->binlog_savesec) 
    {
        return RS_OK;
    }

    if(lseek(si->info_fd, 0, SEEK_SET) == -1) {
        rs_log_err(rs_errno, "lseek() failed");
        return RS_ERR;
    }

    si->cur_binlog_save = 1;
    si->cur_binlog_savesec = tv.tv_sec;

    rs_log_info("flush slave.info %*.*s", len, len, buf);

    n = rs_write(si->info_fd, buf, len);

    if(n != len) {
        return RS_ERR;
    } 

    /* truncate other remained file bytes */
    if(truncate(si->slave_info, len) != 0) {
        rs_log_err(rs_errno, "truncate() failed");
        return RS_ERR;
    }

    return RS_OK; 
}

static int rs_redis_batch_commit(rs_slave_info_t *si)
{
    if(si->cmdn < RS_REDIS_CMD_COMMIT_NUM) {
        return RS_OK;
    }

    if(rs_redis_get_replies(si, si->cmdn) != RS_OK) {
        return RS_ERR;
    }

    si->cmdn = 0;

    return RS_OK;
}

static void rs_free_redis_thread(void *data)
{
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
}
