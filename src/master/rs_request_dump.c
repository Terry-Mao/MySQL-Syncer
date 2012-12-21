
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

static void *rs_start_io_thread(void *data);

/*
 * DESCRIPTION 
 *    dump thread start routine
 *    
 *
 * RETURN VALUE
 *    No return value
 */
void *rs_start_dump_thread(void *data) 
{
    int                 err, id;
    uint32_t            pack_len;
    char                *cbuf, *p;
    rs_reqdump_data_t   *d;
    rs_ringbuf_data_t   *rd;

    cbuf = NULL;
    p = NULL;
    d = (rs_reqdump_data_t *) data;
    d = NULL;

    pthread_cleanup_push(rs_free_dump_thread, (void *) rd);

    /* get packet length (litte endian) */
    if(rs_size_read(d->cli_fd, &pack_len, RS_SLAVE_CMD_PACK_HEADER_LEN) 
            == RS_ERR) 
    {
        goto free;
    }

    /* alloc cmd buf from mempool */
    id = rs_palloc_id(d->pool, pack_len + 1);
    cbuf = (char *) rs_palloc(d->pool, pack_len + 1, id);

    if(cbuf == NULL) {
        goto free;
    }

    /* while get a full packet */
    if(rs_size_read(d->cli_fd, cbuf, pack_len) == RS_ERR) {
        goto free;
    }

    cbuf[pack_len] = '\0';

    /* parse command then open and seek file */
    if((p = rs_strchr(cbuf, ',')) == NULL) {
        rs_log_err(0, "rs_start_dump_thread() failed, cmd = %s invalid format", 
                cbuf);
        goto free;
    }

    /* alloc dump_file and dump_tmp_file from mempool */
    id = rs_palloc_id(d->pool, PATH_MAX + 1);
    if((d->dump_file = (char *) rs_palloc(d->pool, PATH_MAX + 1, id)) == NULL) 
    {
        goto free;
    }

    if((d->dump_tmp_file = (char *) rs_palloc(d->pool, PATH_MAX + 1, id)) 
            == NULL) 
    {
        goto free;
    }

    rs_memcpy(d->dump_file, cbuf, p - cbuf);
    d->dump_num = rs_estr_to_uint32(p - 1);
    d->dump_pos = rs_str_to_uint32(p + 1);

    /* get filter tables */
    if((p = rs_strchr(p + 1, ',')) != NULL) {
        d->filter_tables = p;
    }

    rs_log_info(
            "\n========== SLAVE CMD ==============\n"
            "cmd = %s"
            "dump_file = %s"
            "dump_num = %u"
            "dump_pos = %u" 
            "filter_tables = %s"
            "\n===================================\n",
            cbuf, 
            d->dump_file, 
            d->dump_num, 
            d->dump_pos, 
            d->filter_tables);

    /* start io thread, read binlog */
    if((err = pthread_create(&(d->io_thread), &(d->req_dump->thr_attr)
                    , rs_start_io_thread, (void *) d)) != 0) 
    {
        rs_log_err(err, "pthread_create() faile");
        goto free;
    }

    /* loop get ringbuffer data */
    for( ;; ) {

        err = rs_ringbuf_get(d->ringbuf, &rd);

        if(err == RS_ERR) {
            goto free;
        }

        if(err == RS_EMPTY) {

            /* send reserved buf, so won't hungry */
            if(rs_send_tmpbuf(d->send_buf, d->cli_fd) != RS_OK) {
                goto free;
            }

            /* spin wait */
            err = rs_ringbuf_spin_wait(d->ringbuf, &rd);

            if(err != RS_OK) {
                /* sleep wait, release cpu */
                err = rs_timed_select(d->cli_fd, 0, 
                        RS_RING_BUFFER_EMPTY_SLEEP_USEC);

                if(err == RS_ERR) {
                    goto free;
                } else if(err == RS_OK) {
                    /* test slave alive */
                    if(rs_read(d->cli_fd, (void *) &pack_len, 1) <= 0) {
                        goto free;
                    }

                    /* won't happen */
                    goto free;
                }

                continue;
            }
        }

        // if have enough space buffer data
        if((uint32_t) (d->send_buf->end - d->send_buf->last) >= 4 + rd->len) {

            d->send_buf->last = rs_cpymem(d->send_buf->last, &(rd->len), 4);
            d->send_buf->last = rs_cpymem(d->send_buf->last, rd->data, 
                    rd->len);

            rs_ringbuf_get_advance(d->ringbuf);

        } else {
            /* send buf */
            if(rs_send_tmpbuf(d->send_buf, d->cli_fd) != RS_OK) {
                goto free;
            }
        }
    } // END FOR

free:;

     pthread_cleanup_pop(1);
     pthread_exit(NULL);
}


static void *rs_start_io_thread(void *data)
{
    rs_reqdump_data_t   *d;

    d = (rs_reqdump_data_t *) data;

    pthread_cleanup_push(rs_free_io_thread, d);

    /* open binlog index file */
    if((d->binlog_idx_fp = fopen(d->binlog_idx_file, "r")) == NULL) {
        rs_log_err(rs_errno, "fopen(\"%s\") failed, binlog_idx_file");
        goto free;
    }

    /* read file, parse file, store sp */
    for( ;; ) {

        /* open new binlog */
        if((d->binlog_fp = fopen(d->dump_file, "r")) == NULL) {
            rs_log_err(rs_errno, "fopen(\"%s\", \"r\") failed", 
                    d->dump_file);
            goto free;
        }

        rs_log_info("open a new binlog = %s", d->dump_file);

        /* skip magic num */
        d->dump_pos = d->dump_pos == 0 ? RS_BINLOG_MAGIC_NUM_LEN : 
            d->dump_pos;

        if(fseek(d->binlog_fp, d->dump_pos, SEEK_SET) == -1) {
            rs_log_err(rs_errno, "fseek() failed, seek_set pos = %u", 
                    d->dump_pos);
            goto free;
        }

        /* read binlog */
        if(rs_read_binlog(d) == RS_ERR) {
            goto free;
        }

        /* close old binlog */
        if(fclose(d->binlog_fp) != RS_OK) {
            rs_log_err(rs_errno, "fclose() failed");
            goto free;
        }
    }

free:;

     pthread_cleanup_pop(1);
     pthread_exit(NULL);
}

void rs_free_io_thread(void *data)
{
    rs_reqdump_data_t   *d;

    d = (rs_reqdump_data_t *) data;

    d->io_thread_exit = 1;
    rs_free_reqdump_data(d->req_dump, d);

    rs_log_debug(0, "io thread stoped");
}

void rs_free_dump_thread(void *data)
{
    rs_reqdump_data_t   *d;

    d = (rs_reqdump_data_t *) data;

    d->dump_thread_exit = 1;
    rs_free_reqdump_data(d->req_dump, d);

    rs_log_debug(0, "dump thread stoped");
}

int rs_init_reqdump(rs_reqdump_t *rd, uint32_t num)
{
    int                 err, id;
    uint32_t            i;
    rs_reqdump_data_t   *next;

    id = rs_palloc_id(rd->pool, sizeof(rs_reqdump_data_t) * num);
    rd->data = (rs_reqdump_data_t *) rs_palloc(rd->pool, 
            sizeof(rs_reqdump_data_t) * num, id);

    if(rd->data == NULL) {
        return RS_ERR;
    }

    rd->data_id = id;

    /* init thread mutex */
    if((err = pthread_mutex_init(&(rd->thr_mutex), NULL)) != 0) {
        rs_log_err(err, "pthread_mutex_init() failed");
        return RS_ERR;
    }

    /* init thread attr */
    if((err = pthread_attr_init(&(rd->thr_attr))) != 0) {
        rs_log_err(err, "pthread_attr_init() failed");
        return RS_ERR;
    }

    /* set thread detached */
    if((err = pthread_attr_setdetachstate(&(rd->thr_attr), 
                    PTHREAD_CREATE_DETACHED)) != 0) 
    {
        rs_log_err(err, "pthread_attr_setdetachstate() failed");
        return RS_ERR;
    }

    rd->num = num;
    rd->free_num = num;

    i = num;
    next = NULL;

    do {

        i--;

        rd->data[i].data = next;
        next = &(rd->data[i]);
        rs_reqdump_data_t_init(next);

    } while(i);

    rd->free = next;

    return RS_OK;
}

rs_reqdump_data_t *rs_get_reqdump_data(rs_reqdump_t *rd) 
{
    int                 err;
    rs_reqdump_data_t   *d;

    /* mutex_lock */
    if((err = pthread_mutex_lock(&(rd->thr_mutex))) != 0) {
        rs_log_err(err, "pthread_mutex_lock() failed");
        return NULL;
    }

    d = rd->free;

    if(rd == NULL) {
        /* mutex_unlock */
        if((err = pthread_mutex_unlock(&(rd->thr_mutex))) != 0) {
            rs_log_err(err, "pthread_mutex_unlock() failed");
        }

        return NULL;
    }

    rd->free = d->data;
    rd->free_num--;

    /* mutex_unlock */
    if((err = pthread_mutex_unlock(&(rd->thr_mutex))) != 0) {
        rs_log_err(err, "pthread_mutex_unlock() failed");
        return NULL;
    }

    d->open = 1;
    return d;
}

void rs_free_reqdump_data(rs_reqdump_t *d, rs_reqdump_data_t *rd) 
{
    int         err;

    if(!rd->open) {
        return;
    }

    /* mutex_lock */
    if((err = pthread_mutex_lock(&(d->thr_mutex))) != 0) {
        rs_log_err(err, "pthread_mutex_lock() failed");
        return;
    }

    if(!rd->open) {
        if((err = pthread_mutex_unlock(&(d->thr_mutex))) != 0) {
            rs_log_err(err, "pthread_mutex_unlock() failed");
        }
        return;
    }

    rd->open = 0;

    /* free request dump */
    if(rd->io_thread != 0) {
        if(!rd->io_thread_exit) {
            rs_log_debug(0, "stop io thread");
            if((err = pthread_cancel(rd->io_thread)) != 0) {
                rs_log_err(err, "pthread_cancel() failed");
            }
        }
    }

    if(rd->dump_thread != 0) {
        if(!rd->dump_thread_exit) {
            rs_log_debug(0, "stop dump thread");
            if((err = pthread_cancel(rd->dump_thread)) != 0) {
                rs_log_err(err, "pthread_cancel() failed");
            }
        }
    }

    if(rd->cli_fd != -1) {
        rs_close(rd->cli_fd);
    }

    if(rd->notify_fd != -1) {
        rs_close(rd->notify_fd);
    }

    sleep(10);

    /* free ring buffer */
    rs_destroy_ringbuf(rd->ringbuf);

    /* free packbuf */
    rs_destroy_tmpbuf(rd->send_buf);

    /* free iobuf */
    rs_destroy_tmpbuf(rd->io_buf);

    /* close binlog fp */
    if(rd->binlog_fp != NULL) {
        if(fclose(rd->binlog_fp) != 0) {
            rs_log_err(rs_errno, "fclose() failed");
        }
    }

    if(rd->binlog_idx_fp != NULL) {
        if(fclose(rd->binlog_idx_fp) != 0) {
            rs_log_err(rs_errno, "fclose() failed");
        }
    }


    /* update free thread p */
    rd->data = d->free;
    d->free = rd;
    d->free_num++;

    rs_reqdump_data_t_init(rd);

    /* mutex_unlock */
    if((err = pthread_mutex_unlock(&(d->thr_mutex))) != 0) {
        rs_log_err(err, "pthread_mutex_unlock() failed");
    }
}

void rs_freeall_reqdump_data(rs_reqdump_t *d) 
{
    uint32_t i;

    for(i = 0; i < d->num; i++) {
        if(d->data[i].open) {
            rs_free_reqdump_data(d, &(d->data[i]));
        }
    }
}

void rs_destroy_reqdump(rs_reqdump_t *d)
{
    int err;

    rs_freeall_reqdump_data(d);

    if((err = pthread_mutex_destroy(&(d->thr_mutex))) != 0) {
        rs_log_err(err, "pthread_mutex_destroy() failed");
    }

    if((err = pthread_attr_destroy(&(d->thr_attr))) != 0) {
        rs_log_err(err, "pthread_attr_destroy() failed");
    }

    if(d->data != NULL) {
        rs_pfree(d->pool, d->data, d->data_id);
    }
}
