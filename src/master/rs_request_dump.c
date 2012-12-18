
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
    int                     err, id;
    uint32_t                pack_len;
    char                    *cbuf, *p;
    rs_request_dump_t       *rd;
    rs_ring_buffer2_data_t  *d;
    rs_slab_t               *sl;
    rs_buf_t                *send_buf;

    cbuf = NULL;
    p = NULL;
    rd = (rs_request_dump_t *) data;
    d = NULL;
    sl = NULL;
    send_buf = NULL;

    pthread_cleanup_push(rs_free_dump_thread, (void *) rd);

    if(rd == NULL || rd->rdi == NULL) {
        rs_log_err(rs_errno, "rs_start_dump_thread() failed, data is null");
        goto free;
    }

    sl = &(rd->slab);
    send_buf = &(rd->send_buf);

    /* get packet length (litte endian) */
    if(rs_size_read(rd->cli_fd, &pack_len, RS_SLAVE_CMD_PACK_HEADER_LEN) 
            == RS_ERR) 
    {
        rs_log_err(rs_errno, "rs_size_read() in rs_start_dump_thread failed");
        goto free;
    }

    /* alloc cmd buf from mempool */
    id = rs_slab_clsid(sl, pack_len + 1);
    cbuf = (char *) rs_alloc_slab_chunk(sl, pack_len + 1, id);

    if(cbuf == NULL) {
        rs_log_err(0, "rs_start_dump_thread() failed, can't alloc cmd mem");
        goto free;
    } 

    /* while get a full packet */
    if(rs_size_read(rd->cli_fd, cbuf, pack_len) == RS_ERR) {
        rs_log_err(rs_errno, "rs_size_read() in rs_start_dump_thread failed");
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
    id = rs_slab_clsid(sl, PATH_MAX + 1);
    rd->dump_file = (char *) rs_alloc_slab_chunk(sl, PATH_MAX + 1, id);

    if(rd->dump_file == NULL) { 
        rs_log_err(0, "rs_start_dump_thread() failed, can't alloc cmd mem");
        goto free;
    }

    rd->dump_tmp_file = (char *) rs_alloc_slab_chunk(sl, PATH_MAX + 1, id);

    if(rd->dump_tmp_file == NULL) {
        rs_log_err(0, "rs_start_dump_thread() failed, can't alloc cmd mem");
        goto free;
    }

    rs_memcpy(rd->dump_file, cbuf, p - cbuf);
    rd->dump_num = rs_estr_to_uint32(p - 1);
    rd->dump_pos = rs_str_to_uint32(p + 1);

    /* get filter tables */
    if((p = rs_strchr(p + 1, ',')) != NULL) {
        rd->filter_tables = p;
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
            rd->dump_file, 
            rd->dump_num, 
            rd->dump_pos, 
            rd->filter_tables);


    /* start io thread, read binlog */
    if((err = pthread_create(&(rd->io_thread), &(rd->rdi->thread_attr)
                    , rs_start_io_thread, (void *) rd)) != 0) 
    {
        rs_log_err(err, "pthread_create() in rs_start_dump_thread failed");
        goto free;
    }

    /* loop get ringbuffer data */
    for( ;; ) {

        err = rs_get_ring_buffer2(&(rd->ring_buf), &d);

        if(err == RS_ERR) {
            goto free;
        }

        if(err == RS_EMPTY) {

            /* send reserved buf, so won't hungry */
            if(rs_send_temp_buf(rd->cli_fd, send_buf) != RS_OK) {
                rs_log_err(0, "rs_send_temp_buf() in rs_start_dump_thread "
                        "failed()");
                goto free;
            }

            /* spin wait */
            err = rs_sleep_ring_buffer2(&(rd->ring_buf), &d);

            if(err != RS_OK) {
                /* sleep wait, release cpu */
                err = rs_timed_select(rd->cli_fd, 0, 
                        RS_RING_BUFFER_EMPTY_SLEEP_USEC);

                if(err == RS_ERR) {
                    goto free;
                } else if(err == RS_OK) {
                    /* test slave alive */
                    if(rs_read(rd->cli_fd, (void *) &pack_len, 1) <= 0) {
                        rs_log_err(rs_errno, "rs_read() in rs_start_dump_thrad "
                                "failed");
                        goto free;
                    }

                    /* won't happen */
                    goto free;
                }

                continue;
            }
        }

        // if have enough space buffer data
        if((uint32_t) (send_buf->end - send_buf->last) >= 4 + d->len) {

            send_buf->last = rs_cpymem(send_buf->last, &(d->len), 4);
            send_buf->last = rs_cpymem(send_buf->last, d->data, d->len);

            rs_get_ring_buffer2_advance(&(rd->ring_buf));

        } else {
            /* send buf */
            if(rs_send_temp_buf(rd->cli_fd, send_buf) != RS_OK) {
                rs_log_err(0, "rs_send_temp_buf() in rs_start_dump_thread "
                        "failed()");
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
    rs_request_dump_t   *rd;

    rd = (rs_request_dump_t *) data;

    pthread_cleanup_push(rs_free_io_thread, rd);

    if(rd == NULL) {
        goto free;
    }

    /* open binlog index file */
    if((rd->binlog_idx_fp = fopen(rd->binlog_idx_file, "r")) == NULL) {
        rs_log_err(rs_errno, "fopen(\"%s\") failed, binlog_idx_file");
        goto free;
    }

    /* read file, parse file, store sp */
    for( ;; ) {

        /* open new binlog */
        if((rd->binlog_fp = fopen(rd->dump_file, "r")) == NULL) {
            rs_log_err(rs_errno, "fopen(\"%s\", \"r\") failed", 
                    rd->dump_file);
            goto free;
        }

        rs_log_info("open a new binlog = %s", rd->dump_file);

        /* skip magic num */
        rd->dump_pos = rd->dump_pos == 0 ? RS_BINLOG_MAGIC_NUM_LEN : 
            rd->dump_pos;

        if(fseek(rd->binlog_fp, rd->dump_pos, SEEK_SET) == -1) {
            rs_log_err(rs_errno, "fseek() failed, seek_set pos = %u", 
                    rd->dump_pos);
            goto free;
        }

        /* read binlog */
        if(rs_read_binlog(rd) == RS_ERR) {
            goto free;
        }

        /* close old binlog */
        if(fclose(rd->binlog_fp) != RS_OK) {
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
    rs_request_dump_t           *rd;

    rd = (rs_request_dump_t *) data;

    if(rd != NULL) {
        rd->io_thread_exit = 1;
        rs_free_request_dump(rd->rdi, rd);
    }

    rs_log_debug(0, "io thread stoped");
}

void rs_free_dump_thread(void *data)
{
    rs_request_dump_t           *rd;

    rd = (rs_request_dump_t *) data;

    if(rd != NULL) {
        rd->dump_thread_exit = 1;
        rs_free_request_dump(rd->rdi, rd);
    }

    rs_log_debug(0, "dump thread stoped");
}

int rs_init_request_dump(rs_request_dump_info_t *rdi, uint32_t dump_num) 
{
    int                 err;
    uint32_t            i;
    rs_request_dump_t   *next;

    if(rdi == NULL) {
        return RS_ERR;
    }

    rdi->req_dumps = (rs_request_dump_t *) 
        malloc(sizeof(rs_request_dump_t) * dump_num);

    if(rdi->req_dumps == NULL) {
        rs_log_err(rs_errno, "malloc() failed, rs_request_dump_t * dump_num");
        return RS_ERR;
    }

    /* init thread mutex */
    if((err = pthread_mutex_init(&(rdi->req_dump_mutex), NULL)) != 0) {
        rs_log_err(err, "pthread_mutex_init() failed, req_dump_mutex");
        return RS_ERR;
    }

    /* init thread attr */
    if((err = pthread_attr_init(&(rdi->thread_attr))) != 0) {
        rs_log_err(err, "pthread_attr_init() failed, thread_attr");
        return RS_ERR;
    }

    /* set thread detached */
    if((err = pthread_attr_setdetachstate(&(rdi->thread_attr), 
                    PTHREAD_CREATE_DETACHED)) != 0) 
    {
        rs_log_err(err, "pthread_attr_setdetachstate() failed, DETACHED");
        return RS_ERR;
    }

    rdi->dump_num = dump_num;
    rdi->free_dump_num = dump_num;

    i = dump_num;
    next = NULL;

    do {

        i--;

        rdi->req_dumps[i].data = next;
        next = &(rdi->req_dumps[i]);
        rs_request_dump_t_init(next);

    } while(i);

    rdi->free_req_dump = next;

    return RS_OK;
}

rs_request_dump_t *rs_get_request_dump(rs_request_dump_info_t *rdi) 
{
    int                 err;
    rs_request_dump_t   *rd;

    if(rdi == NULL) {
        return NULL;
    }

    /* mutex_lock */
    if((err = pthread_mutex_lock(&(rdi->req_dump_mutex))) != 0) {
        rs_log_err(err, "pthread_mutex_lock() failed, req_dump_mutex");
        return NULL;
    }

    rd = rdi->free_req_dump;

    if(rd == NULL) {
        /* mutex_unlock */
        if((err = pthread_mutex_unlock(&(rdi->req_dump_mutex))) != 0) {
            rs_log_err(err, "pthread_mutex_unlock() failed, req_dump_mutex");
        }

        return NULL;
    }

    rdi->free_req_dump = rd->data;
    rdi->free_dump_num--;

    /* mutex_unlock */
    if((err = pthread_mutex_unlock(&(rdi->req_dump_mutex))) != 0) {
        rs_log_err(err, "pthread_mutex_unlock() failed, req_dump_mutex");
        return NULL;
    }

    rd->open = 1;
    return rd;
}

void rs_free_request_dump(rs_request_dump_info_t *rdi, rs_request_dump_t *rd) 
{
    int         err;

    if(rdi == NULL || rd == NULL || !rd->open) {
        return;
    }

    /* mutex_lock */
    if((err = pthread_mutex_lock(&(rdi->req_dump_mutex))) != 0) {
        rs_log_err(err, "pthread_mutex_lock() failed, req_dump_mutex");
        return;
    }

    if(!rd->open) {
        if((err = pthread_mutex_unlock(&(rdi->req_dump_mutex))) != 0) {
            rs_log_err(err, "pthread_mutex_unlock() failed, req_dump_mutex");
        }
        return;
    }

    rd->open = 0;

    /* free request dump */
    if(rd->io_thread != 0) {

        if(!rd->io_thread_exit) {
            rs_log_debug(0, "stop io thread");
            if((err = pthread_cancel(rd->io_thread)) != 0) {
                rs_log_err(err, "pthread_cancel() failed, io_thread");
            }
        }

        /*
           if((err = pthread_join(rd->io_thread, NULL)) != 0) {
           rs_log_err(err, "pthread_join() failed, io_thread");
           }
           */
    }

    if(rd->dump_thread != 0) {
        if(!rd->dump_thread_exit) {
            rs_log_debug(0, "stop dump thread");
            if((err = pthread_cancel(rd->dump_thread)) != 0) {
                rs_log_err(err, "pthread_cancel() failed, dump_thread");
            }
        }

        /*
           if((err = pthread_join(rd->dump_thread, NULL)) != 0) {
           rs_log_err(err, "pthread_join() failed, dump_thread");
           }
           */
    }

    if(rd->cli_fd != -1) {
        rs_close(rd->cli_fd);
    }

    if(rd->notify_fd != -1) {
        rs_close(rd->notify_fd);
    }

    sleep(10);

    /* free ring buffer */
    rs_free_ring_buffer2(&(rd->ring_buf));

    /* free slab */
    rs_free_slabs(&(rd->slab));

    /* free packbuf */
    rs_free_temp_buf(&(rd->send_buf));

    /* free iobuf */
    rs_free_temp_buf(&(rd->io_buf));

    /* close binlog fp */
    if(rd->binlog_fp != NULL) {
        if(fclose(rd->binlog_fp) != 0) {
            rs_log_err(rs_errno, "fclose() failed, binlog fp");
        }
    }

    if(rd->binlog_idx_fp != NULL) {
        if(fclose(rd->binlog_idx_fp) != 0) {
            rs_log_err(rs_errno, "fclose() failed, binlog file");
        }
    }


    /* update free thread p */
    rd->data = rdi->free_req_dump;
    rdi->free_req_dump = rd;
    rdi->free_dump_num++;

    rs_request_dump_t_init(rd);

    /* mutex_unlock */
    if((err = pthread_mutex_unlock(&(rdi->req_dump_mutex))) != 0) {
        rs_log_err(err, "pthread_mutex_unlock() failed, req_dump_mutex");
    }
}

void rs_free_request_dumps(rs_request_dump_info_t *rdi) 
{
    uint32_t i;

    if(rdi != NULL) {
        for(i = 0; i < rdi->dump_num; i++) {
            if(rdi->req_dumps[i].open) {
                rs_free_request_dump(rdi, &(rdi->req_dumps[i]));
            }
        }
    }
}

void rs_destroy_request_dumps(rs_request_dump_info_t *rdi)
{
    int err;

    if(rdi != NULL) {

        rs_free_request_dumps(rdi);

        if((err = pthread_mutex_destroy(&(rdi->req_dump_mutex))) != 0) {
            rs_log_err(err, "pthread_mutex_destroy() failed, "
                    "rdi->req_dump_mutex");
        }

        if((err = pthread_attr_destroy(&(rdi->thread_attr))) != 0) {
            rs_log_err(err, "pthread_attr_destroy() failed, rdi->thread_attr");
        }

        if(rdi->req_dumps != NULL) {
            free(rdi->req_dumps);
        }
    }
}
