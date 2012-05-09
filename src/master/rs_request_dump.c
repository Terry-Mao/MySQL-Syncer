
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
    int                     err, i, ready, mfd, s, id;
    uint32_t                pack_len, cur_len;
    ssize_t                 n;
    char                    *cbuf, *p, pack_buf[4];
    rs_request_dump_t       *rd;
    rs_ring_buffer2_data_t  *d;
    rs_slab_t               *sl;
    fd_set                  rset, tset;
    struct timeval          tv;

    s = 0;
    cbuf = NULL;
    rd = (rs_request_dump_t *) data;
    FD_ZERO(&rset);
    FD_ZERO(&tset);
    mfd = 0;
    ready = 0;
    pthread_cleanup_push(rs_free_dump_thread, (void *) rd);

    if(rd == NULL || rd->rdi == NULL) {
        rs_log_err(rs_errno, "rd or rdi is null");
        goto free;
    }

    sl = &(rd->slab);

    /* get packet length (litte endian) */
    cur_len = 0;
    while(cur_len != RS_SLAVE_CMD_PACK_HEADER_LEN) {
        n = rs_read(rd->cli_fd, pack_buf + cur_len, 
                RS_SLAVE_CMD_PACK_HEADER_LEN - cur_len);

        if(n <= 0) {
            rs_log_err(rs_errno, "rs_read() failed, error or shutdown");
            goto free;
        }

        cur_len += (uint32_t) n;
    }

    rs_memcpy(&pack_len, pack_buf, RS_SLAVE_CMD_PACK_HEADER_LEN);

    if(n <= 0) {
        rs_log_err(rs_errno, "rs_read() failed, error or shutdown");
        goto free;
    }

    /* alloc cmd buf from mempool */
    id = rs_slab_clsid(sl, pack_len);
    cbuf = (char *) rs_alloc_slab_chunk(sl, pack_len, id);

    if(cbuf == NULL) {
        rs_log_err(rs_errno, "rs_alloc_slab failed(), register_slave_cmd");
        goto free;
    } 

    /* while get a full packet */
    cur_len = 0;
    while(cur_len != pack_len) {
        n = rs_read(rd->cli_fd, cbuf + cur_len, pack_len - cur_len);

        if(n <= 0) {
            rs_log_err(rs_errno, "rs_read() failed, error or shutdown");
            goto free;
        }

        cur_len += (uint32_t) n;
    }

    cbuf[n] = '\0';

    /* parse command then open and seek file */
    if((p = rs_strchr(cbuf, ',')) == NULL) {
        rs_log_err(0, "slave cmd format error");
        goto free;
    }

    rs_memcpy(rd->dump_file, cbuf, p - cbuf);
    rd->dump_num = rs_estr_to_uint32(p - 1);
    rd->dump_pos = rs_str_to_uint32(p + 1);

    /* get filter tables */
    if((p = rs_strchr(p + 1, ',')) != NULL) {
        rd->filter_tables = p;
    }


#if 0
    /* free cmd buffer */
    rs_free_slab_chunk(sl, cbuf, id);
#endif

    rs_log_info("cmd = %s, dump_file = %s, dump_num = %u, dump_pos = %u, "
            "filter_tables = %s", cbuf, rd->dump_file, rd->dump_num, 
            rd->dump_pos, rd->filter_tables);

    if((err = pthread_create(&(rd->io_thread), &(rd->rdi->thread_attr)
                    , rs_start_io_thread, (void *) rd)) != 0) 
    {
        rs_log_err(err, "pthread_create() failed, rs_dump_io");
        goto free;
    }

    for( ;; ) {

        err = rs_get_ring_buffer2(&(rd->ring_buf), &d);

        if(err == RS_ERR) {
            rs_log_err(0, "rs_get_ring_buffer() failed, dump data");
            goto free;
        }

        if(err == RS_EMPTY) {

            for (n = 1; n < RS_RING_BUFFER_SPIN; n <<= 1) {

                for (i = 0; i < n; i++) {
                    rs_cpu_pause();
                }

                err = rs_get_ring_buffer2(&(rd->ring_buf), &d);

                if(err == RS_OK) {
                    break;
                }
            }

            if(err != RS_OK) {

                FD_SET(rd->cli_fd, &rset);
                tset = rset;
                mfd = rs_max(rd->cli_fd, mfd);

                tv.tv_sec = 0;
                tv.tv_usec = RS_RING_BUFFER_EMPTY_SLEEP_USEC;

                ready = select(mfd + 1, &tset, NULL, NULL, &tv);

                if(ready == 0) {
                    if(s % 60000000 == 0) {
                        s = 0;
                        rs_log_info("dump thread wait ring buf fill data");
                    }

                    s += RS_RING_BUFFER_EMPTY_SLEEP_USEC;

                    continue;
                }

                if(ready == -1) {
                    if(rs_errno == EINTR) {
                        continue;
                    }

                    rs_log_err(rs_errno, "select failed(), rd->cli_fd");
                    goto free;
                }

                if(!FD_ISSET(rd->cli_fd, &tset)) {
                    goto free;
                }

                /* test slave alive */
                n = rs_read(rd->cli_fd, cbuf, 1);

                if(n <= 0) {
                    rs_log_err(rs_errno, "rs_read() failed, err or shutdown");
                    goto free;
                }

                FD_CLR(rd->cli_fd, &rset);
                continue;
            }
        }

        /* send packet length */
        n = rs_write(rd->cli_fd, &(d->len), 4);

        if((size_t) n != 4) {
            goto free;
        }

        /* send packet data */
        n = rs_write(rd->cli_fd, d->data, d->len);

        if((size_t) n != d->len) {
            goto free;
        }

        rs_get_ring_buffer2_advance(&(rd->ring_buf));

    } // END FOR

free:

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
    if((rd->binlog_idx_fp = fopen(rd->binlog_idx_file, "r"))
            == NULL) 
    {
        rs_log_err(rs_errno, "fopen(\"%s\") failed, binlog_idx_file");
        goto free;
    }

    /* read file, parse file, store sp */
    for( ;; ) {

        /* open new binlog */
        rs_log_info("open a new binlog, %s", rd->dump_file);

        if((rd->binlog_fp = fopen(rd->dump_file, "r")) == NULL) {
            rs_log_err(rs_errno, "fopen(\"%s\", \"r\") failed", 
                    rd->dump_file);
            goto free;
        }

        /* read binlog */
        if(rs_read_binlog(rd) == RS_ERR) {
            goto free;
        }

        /* close old binlog */
        if(fclose(rd->binlog_fp) != 0) {
            rs_log_err(rs_errno, "fclose() failed");
            goto free;
        }
    }

free:

    pthread_cleanup_pop(1);
    pthread_exit(NULL);
}

void rs_free_io_thread(void *data)
{
    rs_request_dump_t           *rd;

    rd = (rs_request_dump_t *) data;

    if(rd != NULL) {
        rd->io_thread = 0;
        rs_free_request_dump(rd->rdi, rd);
    }
}

void rs_free_dump_thread(void *data)
{
    rs_request_dump_t           *rd;

    rd = (rs_request_dump_t *) data;

    if(rd != NULL) {
        rd->dump_thread = 0;
        rs_free_request_dump(rd->rdi, rd);
    }
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

    if(rd->cli_fd != -1) {
        rs_close(rd->cli_fd);
    }

    if(rd->notify_fd != -1) {
        rs_close(rd->notify_fd);
    }

    /* free request dump */
    if(rd->io_thread != 0) {
        // tid = rd->io_thread;
        if((err = pthread_cancel(rd->io_thread)) != 0) {
            rs_log_err(err, "pthread_cancel() failed, io_thread");
        } 
    }

    if(rd->dump_thread != 0) {
        // tid = rd->dump_thread;
        if((err = pthread_cancel(rd->dump_thread)) != 0) {
            rs_log_err(err, "pthread_cancel() failed, dump_thread");
        }
    }

    /* free ring buffer */
    rs_free_ring_buffer2(&(rd->ring_buf));

    /* free slab */
    rs_free_slabs(&(rd->slab));

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
            rs_log_err(err, "pthread_mutex_destroy() failed, dump_mutex");
        }

        if((err = pthread_attr_destroy(&(rdi->thread_attr))) != 0) {
            rs_log_err(err, "pthread_attr_destroy() failed, thread_attr");
        }

        if(rdi->req_dumps != NULL) {
            free(rdi->req_dumps);
        }
    }
}
