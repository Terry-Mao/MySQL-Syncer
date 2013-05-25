
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

static void *rs_start_io_thread(void *data);
static int rs_parse_dumpcmd(rs_reqdump_data_t *rd);

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
    int                 err, len;
    char                t;
    rs_reqdump_data_t   *rd;
    rs_ringbuf_data_t   *rbd;

    rd = (rs_reqdump_data_t *) data;
    rbd = NULL;
    pthread_cleanup_push(rs_free_dump_thread, (void *) rd);
    /* get dumpcmd info */
    if(rs_parse_dumpcmd(rd) != RS_OK) {
        goto free;
    }

    /* start io thread, read binlog */
    if((err = pthread_create(&(rd->io_thread), &(rd->req_dump->thr_attr), 
                    rs_start_io_thread, (void *) rd)) != 0) 
    {
        rs_log_error(RS_LOG_ERR, err, "pthread_create() failed");
        goto free;
    }

    /* loop get ringbuffer data */
    for( ;; ) {
        err = rs_ringbuf_get(rd->ringbuf, &rbd);
        if(err == RS_EMPTY) {
            /* send reserved buf, so won't hungry */
            if(rs_send_tmpbuf(rd->send_buf, rd->cli_fd) != RS_OK) {
                goto free;
            }

            /* spin wait */
            err = rs_ringbuf_spin_wait(rd->ringbuf, &rbd);
            if(err != RS_OK) {
                /* sleep wait, release cpu */
                err = rs_timed_select(rd->cli_fd, 0, rd->rb_esusec);
                if(err == RS_ERR) {
                    goto free;
                } else if(err == RS_OK) {
                    /* test slave alive */
                    if(rs_read(rd->cli_fd, (void *) &t, 1) <= 0) {
                        goto free;
                    }

                    /* won't happen */
                    goto free;
                }

                continue;
            }
        }

        // TODO protocol

        len = 4 + rbd->len;
        if((rd->send_buf->end - rd->send_buf->last) >= len) {
            // if have enough space buffer data
            rs_memcpy(rd->send_buf->last, &(rbd->len), 4);
            rs_memcpy(rd->send_buf->last + 4, rbd->data, rbd->len);
            rd->send_buf->last += len;
            rs_ringbuf_get_advance(rd->ringbuf);
            continue;
        }

        /* send buf */
        if(rs_send_tmpbuf(rd->send_buf, rd->cli_fd) != RS_OK) {
            goto free;
        }
    }

free:;
     pthread_cleanup_pop(1);
     pthread_exit(NULL);
}


static int rs_parse_dumpcmd(rs_reqdump_data_t *rd)
{
    int         id;
    uint32_t    pack_len;
    char        *p;

    p = NULL;

    /* get packet length (litte endian) */
    if(rs_size_read(rd->cli_fd, &pack_len, RS_SLAVE_CMD_PACK_LEN) == RS_ERR) {
        goto free;
    }

    /* alloc cmd buf from mempool */
    id = rs_palloc_id(rd->pool, pack_len);
    rd->cbuf = (char *) rs_palloc(rd->pool, pack_len, id);

    if(rd->cbuf == NULL) {
        return RS_ERR;
    }

    /* while get a full packet */
    if(rs_size_read(rd->cli_fd, rd->cbuf, pack_len) == RS_ERR) {
        goto free;
    }

    /* get slave.info */
    if((p = rs_strchr(rd->cbuf, ',')) == NULL) {
        goto free;
    }

    rs_memcpy(rd->dump_file, rd->cbuf, p - rd->cbuf);
    rd->dump_num = rs_estr_to_uint32(p - 1);
    rd->dump_pos = rs_str_to_uint32(p + 1);
    /* get filter tables */
    if((p = rs_strchr(p, '\n')) == NULL) {
        goto free;
    }

    if(*(++p) != ',') {
        goto free;
    }

    rd->filter_tables = p;
    if((p = rs_strrchr(rd->filter_tables, ',')) == NULL) {
        goto free;
    }

    if((p - rd->filter_tables) < 2) {
        goto free;
    }

    p += 2;
    /* get ringbuf sleep usec */
    rs_memcpy(&(rd->rb_esusec), p, 4);
    rs_log_error(RS_LOG_INFO, 0,
            "\n========== SLAVE CMD ==============\n"
            "cmd = %s\n"
            "dump_file = %s\n"
            "dump_num = %u\n"
            "dump_pos = %u\n" 
            "filter_tables = %s\n"
            "ringbuf_susec = %u"
            "\n===================================\n",
            rd->cbuf, 
            rd->dump_file, 
            rd->dump_num, 
            rd->dump_pos, 
            rd->filter_tables,
            rd->rb_esusec
            );
    /* no filter tables , all db and tables */
    if(strncmp(",,", rd->filter_tables, 2) == 0) {
        rd->filter_tables = NULL;
    }

    return RS_OK;

free:
    rs_log_error(RS_LOG_ERR, 0, "get dumpcmd failed");
    return RS_ERR;
}

static void *rs_start_io_thread(void *data)
{
    rs_reqdump_data_t   *rd;

    rd = (rs_reqdump_data_t *) data;
    pthread_cleanup_push(rs_free_io_thread, rd);
    /* open binlog index file */
    if((rd->binlog_idx_fp = fopen(rd->binlog_idx_file, "r")) == NULL) {
        rs_log_error(RS_LOG_ERR, rs_errno, "fopen(\"%s\") failed");
        goto free;
    }

    /* read file, parse file, store sp */
    for( ;; ) {
        // TODO Clear io_buf
        /* open new binlog */
        rs_log_error(RS_LOG_INFO, 0, "open a new binlog = %s", rd->dump_file);
        if((rd->binlog_fp = fopen(rd->dump_file, "r")) == NULL) {
            rs_log_error(RS_LOG_ERR, rs_errno, "fopen(\"%s\") failed", 
                    rd->dump_file);
            goto free;
        }

        /* skip magic num */
        rd->dump_pos = rd->dump_pos == 0 ? RS_BINLOG_MAGIC_NUM_LEN : 
            rd->dump_pos;
        if(fseek(rd->binlog_fp, rd->dump_pos, SEEK_SET) == -1) {
            rs_log_error(RS_LOG_ERR, rs_errno, "fseek() failed, pos = %u", 
                    rd->dump_pos);
            goto free;
        }

        /* read binlog */
        if(rs_read_binlog(rd) == RS_ERR) {
            goto free;
        }

        /* close old binlog */
        if(fclose(rd->binlog_fp) != RS_OK) {
            rs_log_error(RS_LOG_ERR, rs_errno, "fclose() failed");
            goto free;
        }
    }

free:;
     pthread_cleanup_pop(1);
     pthread_exit(NULL);
}

rs_reqdump_t *rs_create_reqdump(rs_pool_t *p, uint32_t num)
{
    int                 err, id;
    uint32_t            i, len;
    rs_reqdump_t        *rd;
    rs_reqdump_data_t   *next;

    rd = NULL;
    len = sizeof(rs_reqdump_data_t) * num;
    id = rs_palloc_id(p, sizeof(rs_reqdump_t) + len);
    rd = rs_palloc(p, sizeof(rs_reqdump_t) + len, id);

    if(rd == NULL) {
        return NULL;
    }

    rd->pool = p;
    rd->data = (rs_reqdump_data_t *) ((char *) rd + sizeof(rs_reqdump_t));
    rd->id = id;
    /* init thread mutex */
    if((err = pthread_mutex_init(&(rd->thr_mutex), NULL)) != 0) {
        rs_log_error(RS_LOG_ERR, err, "pthread_mutex_init() failed");
        return NULL;
    }

    /* init thread attr */
    if((err = pthread_attr_init(&(rd->thr_attr))) != 0) {
        rs_log_error(RS_LOG_ERR, err, "pthread_attr_init() failed");
        return NULL;
    }

    /* set thread detached */
    if((err = pthread_attr_setdetachstate(&(rd->thr_attr), 
                    PTHREAD_CREATE_DETACHED)) != 0) 
    {
        rs_log_error(RS_LOG_ERR, err, "pthread_attr_setdetachstate() failed");
        return NULL;
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
    return rd;
}

rs_reqdump_data_t *rs_get_reqdump_data(rs_reqdump_t *rd) 
{
    int                 err;
    rs_reqdump_data_t   *d;

    /* mutex_lock */
    if((err = pthread_mutex_lock(&(rd->thr_mutex))) != 0) {
        rs_log_error(RS_LOG_ERR, err, "pthread_mutex_lock() failed");
        return NULL;
    }

    d = rd->free;
    if(rd == NULL) {
        /* mutex_unlock */
        if((err = pthread_mutex_unlock(&(rd->thr_mutex))) != 0) {
            rs_log_error(RS_LOG_ERR, err, "pthread_mutex_unlock() failed");
        }

        return NULL;
    }

    rd->free = d->data;
    rd->free_num--;
    d->open = 1;
    /* mutex_unlock */
    if((err = pthread_mutex_unlock(&(rd->thr_mutex))) != 0) {
        rs_log_error(RS_LOG_ERR, err, "pthread_mutex_unlock() failed");
        return NULL;
    }

    return d;
}

void rs_free_reqdump_data(rs_reqdump_t *rd, rs_reqdump_data_t *d) 
{
    int err;

    if(!d->open) {
        return;
    }

    /* mutex_lock */
    if((err = pthread_mutex_lock(&(rd->thr_mutex))) != 0) {
        rs_log_error(RS_LOG_ERR, err, "pthread_mutex_lock() failed");
        return;
    }

    if(!d->open) {
        if((err = pthread_mutex_unlock(&(rd->thr_mutex))) != 0) {
            rs_log_error(RS_LOG_ERR, err, "pthread_mutex_unlock() failed");
        }
        return;
    }

    d->open = 0;
    /* free request dump */
    if(d->io_thread != 0 && !d->io_thread_exit) {
        if((err = pthread_cancel(d->io_thread)) != 0) {
            rs_log_error(RS_LOG_ERR, err, "pthread_cancel() failed");
        }
    }

    if(d->dump_thread != 0 && !d->dump_thread_exit) {
        if((err = pthread_cancel(d->dump_thread)) != 0) {
            rs_log_error(RS_LOG_ERR, err, "pthread_cancel() failed");
        }
    }

    if(d->cli_fd != -1) {
        if(close(d->cli_fd) != 0) {
            rs_log_error(RS_LOG_ERR, rs_errno, "close() failed"); 
        }
    }

    if(d->notify_fd != -1) {
        if(close(d->notify_fd) != 0) {
            rs_log_error(RS_LOG_ERR, rs_errno, "close() failed"); 
        }
    }

    sleep(10);
    /* free ring buffer */
    rs_destroy_ringbuf(d->ringbuf);
    /* free packbuf */
    rs_destroy_tmpbuf(d->send_buf);
    /* free iobuf */
    rs_destroy_tmpbuf(d->io_buf);
    /* close binlog fp */
    if(d->binlog_fp != NULL && fclose(d->binlog_fp) != 0) {
        rs_log_error(RS_LOG_ERR, rs_errno, "fclose() failed");
    }

    if(d->binlog_idx_fp != NULL && fclose(d->binlog_idx_fp) != 0) {
        rs_log_error(RS_LOG_ERR, rs_errno, "fclose() failed");
    }

    /* free cmd buf */
    /* free pool */
    rs_destroy_pool(d->pool);
    /* update free thread p */
    d->data = rd->free;
    rd->free = d;
    rd->free_num++;
    rs_reqdump_data_t_init(d);
    /* mutex_unlock */
    if((err = pthread_mutex_unlock(&(rd->thr_mutex))) != 0) {
        rs_log_error(RS_LOG_ERR, err, "pthread_mutex_unlock() failed");
    }
}

void rs_freeall_reqdump_data(rs_reqdump_t *rd)
{
    uint32_t i;

    for(i = 0; i < rd->num; i++) {
        if(rd->data[i].open) {
            rs_free_reqdump_data(rd, &(rd->data[i]));
        }
    }
}

void rs_destroy_reqdump(rs_reqdump_t *rd)
{
    int err;

    rs_freeall_reqdump_data(rd);
    if((err = pthread_mutex_destroy(&(rd->thr_mutex))) != 0) {
        rs_log_error(RS_LOG_ERR, err, "pthread_mutex_destroy() failed");
    }

    if((err = pthread_attr_destroy(&(rd->thr_attr))) != 0) {
        rs_log_error(RS_LOG_ERR, err, "pthread_attr_destroy() failed");
    }

    rs_pfree(rd->pool, rd, rd->id);
}

void rs_free_io_thread(void *data)
{
    rs_reqdump_data_t   *rd;

    rd = (rs_reqdump_data_t *) data;
    rd->io_thread_exit = 1;
    rs_free_reqdump_data(rd->req_dump, rd);
}

void rs_free_dump_thread(void *data)
{
    rs_reqdump_data_t   *rd;

    rd = (rs_reqdump_data_t *) data;
    rd->dump_thread_exit = 1;
    rs_free_reqdump_data(rd->req_dump, rd);
}
