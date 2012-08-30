
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>



static int  rs_create_dump_cmd(rs_slave_info_t *s, char *buf, uint32_t *len);
static void rs_free_io_thread(void *data);


/*
 *   rs_register_slave
 *   @s struct rs_slave_info_t
 *
 *   Connect to master, register a slave
 *
 *   No return value
 */
void *rs_start_io_thread(void *data) 
{
    int                     r, sock_err, i;
    socklen_t               len;
    uint32_t                l;
    int32_t                 pack_len, cp_len;
    ssize_t                 n;
    char                    cbuf[RS_REGISTER_SLAVE_CMD_LEN], istr[4], *p;
    struct                  sockaddr_in svr_addr;
    rs_slave_info_t         *si;
    rs_ring_buffer2_data_t  *d;
    rs_slab_t               *sl;
    rs_buf_t                *recv_buf;

    /* init var */
    i = 0;
    rs_memzero(&svr_addr, sizeof(svr_addr));
    len = sizeof(int);
    si = (rs_slave_info_t *) data;

    /* push cleanup handle */
    pthread_cleanup_push(rs_free_io_thread, si);

    if(si == NULL) {
        rs_log_err(0, "io thread can not get slave info struct"); 
        goto free;
    }

    sl = si->slab;
    recv_buf = si->recv_buf;

    /* connect to master */
    svr_addr.sin_family = AF_INET;

    if(si->listen_port <= 0) {
        rs_log_err(0, "listen_port must greate than zero"); 
        goto free;
    }

    svr_addr.sin_port = htons(si->listen_port);

    if(si->listen_addr == NULL) {
        rs_log_err(0, "listen_addr must not be null"); 
        goto free;
    }

    if (inet_pton(AF_INET, si->listen_addr, &(svr_addr.sin_addr)) != 1) {
        rs_log_err(rs_errno, "inet_pton(\"%s\") failed", si->listen_addr); 
        goto free;
    }

    for( ;; ) {

        si->svr_fd = socket(AF_INET, SOCK_STREAM, 0);

        if(si->svr_fd == -1) {
            rs_log_err(rs_errno, "socket() failed, svr_fd");
            goto free;
        }

        if(connect(si->svr_fd, (const struct sockaddr *) &svr_addr, 
                    sizeof(svr_addr)) == -1) 
        {
            if(i % 60 == 0) {
                i = 0;
                rs_log_err(rs_errno, "connect() failed, addr = %s:%d", 
                        si->listen_addr, si->listen_port);
            }

            i += RS_RETRY_CONNECT_SLEEP_SEC;

            goto retry;
        }

        if(rs_create_dump_cmd(si, cbuf, &l) != RS_OK) {
            goto free;
        }

        rs_log_info("send cmd = %s, dump_file = %s, dump_pos = %u, "
                "filter_tables = %s", 
                cbuf, si->dump_file, si->dump_pos, si->filter_tables);

        /* send command length */
        n = rs_write(si->svr_fd, &l, 4); 

        if(n != 4) {
            goto retry;
        }

        /* send command */
        n = rs_write(si->svr_fd, cbuf, l); 

        if(n != (ssize_t) l) {
            goto retry;
        }

        /* add to ring buffer */
        for( ;; ) {

            p = istr;
            pack_len = 0;
            cp_len = 0;

            r = rs_set_ring_buffer2(si->ring_buf, &d);

            if(r == RS_FULL) {
                if(i % 60 == 0) {
                    i = 0;
                    rs_log_info("io thread wait ring buffer consumed");
                }

                i += RS_RING_BUFFER_FULL_SLEEP_SEC;

                if(getsockopt(si->svr_fd, SOL_SOCKET, SO_ERROR, 
                            (void *) &sock_err, &len) == -1)
                {
                    sock_err = rs_errno;
                }

                if(sock_err) {
                    rs_log_info("server fd is closed");
                    goto retry;
                }

                sleep(RS_RING_BUFFER_FULL_SLEEP_SEC);
            }

            if(r == RS_ERR) {
                goto free;
            }

            if(r == RS_OK) {

                /* free slab chunk */
                if(d->data != NULL && d->id >= 0 && d->len > 0) {
                    rs_free_slab_chunk(sl, d->data, d->id); 
                }

                for( ;; ) {

                    if(recv_buf->pos == recv_buf->last) {

                        n = rs_read(si->svr_fd, recv_buf->start, 
                                recv_buf->size);

                        if(n <= 0) {
                            rs_log_err(rs_errno, "rs_read() failed, err or "
                                    "shutdown");
                            goto retry;
                        }

                        recv_buf->last = recv_buf->start + n;
                        recv_buf->pos = recv_buf->start;
                    }

                    /* get packet length */ 
                    if(pack_len == 0) {

                        cp_len = rs_min((recv_buf->last - recv_buf->pos), 
                                4 - (p - istr));

                        p = rs_cpymem(p, recv_buf->pos, cp_len);
                        recv_buf->pos += cp_len;

                        if((p - istr) != 4) {
                            continue;             
                        }

                        rs_memcpy(&pack_len, istr, 4);
                        rs_log_debug(0, "get cmd packet length = %u", pack_len);

                        /* alloc memory */
                        d->len = pack_len;
                        d->id = rs_slab_clsid(sl, pack_len);
                        d->data = rs_alloc_slab_chunk(sl, pack_len, d->id);

                        if(d->data == NULL) {
                            goto free;
                        }

                        p = d->data;
                    }

                    cp_len = rs_min((recv_buf->last - recv_buf->pos)
                            , pack_len - (p - (char *) d->data));

                    p = rs_cpymem(p, recv_buf->pos, cp_len);
                    recv_buf->pos += cp_len;

                    if((p - (char *) d->data) != pack_len) {
                        continue; 
                    }

                    rs_set_ring_buffer2_advance(si->ring_buf);
                    break;
                }
#if 0
                cur_len = 0;
                pack_len = 4;
                /* get packet length */ 
                while(cur_len != pack_len) {

                    n = rs_read(si->svr_fd, istr + cur_len, pack_len - cur_len);

                    if(n <= 0) {
                        rs_log_err(rs_errno, "rs_read() failed, err or "
                                "shutdown");
                        goto retry;
                    }

                    cur_len += (uint32_t) n;
                }

                rs_memcpy(&pack_len, istr, 4);

                rs_log_debug(0, "get cmd packet length = %u", pack_len);

                /* alloc memory */
                d->len = pack_len;
                d->id = rs_slab_clsid(sl, pack_len);
                d->data = rs_alloc_slab_chunk(sl, pack_len, d->id);

                if(d->data == NULL) {
                    goto free;
                }

                cur_len = 0;
                /* while get a full packet */
                while(cur_len != pack_len) {

                    n = rs_read(si->svr_fd, (char *) d->data + cur_len, 
                            pack_len - cur_len);

                    if(n <= 0) {
                        rs_log_err(rs_errno, "rs_read() failed, err or "
                                "shutdown");
                        goto retry;
                    }

                    cur_len += (uint32_t) n;
                }

                rs_set_ring_buffer2_advance(si->ring_buf);

                // rs_log_debug(0, "set data = %x, id = %d, len = %u", d->data, d->id, d->len);

                continue;
#endif
            }

        } // EXIT RING BUFFER 

retry:

        /* close svr_fd retry connect */
        (void) rs_close(si->svr_fd);

        si->svr_fd = -1;

        sleep(RS_RETRY_CONNECT_SLEEP_SEC);
    }


free:;

    pthread_cleanup_pop(1);

    pthread_exit(NULL);
}

/*
 *
 *  rs_create_dump_cmd
 *  @s struct rs_slave_infot
 *  @len store the length of command
 *
 *  Create request dump cmd
 *
 *  On success, RS_OK is returned. On error, RS_ERR is returned
 */
static int rs_create_dump_cmd(rs_slave_info_t *si, char *buf, uint32_t *len) 
{
    int l;

    l = snprintf(buf, RS_REGISTER_SLAVE_CMD_LEN, "%s,%u,%s,", si->dump_file, 
            si->dump_pos, si->filter_tables);

    if(l < 0) {
        rs_log_err(rs_errno, "snprintf() failed, dump_cmd"); 
        return RS_ERR;
    }

    *len = (uint32_t) l;

    return RS_OK;
}

static void rs_free_io_thread(void *data)
{
    rs_slave_info_t *si;

    si = (rs_slave_info_t *) data;

    if(si != NULL) {
        rs_log_info("set io thread exit state");
        si->io_thread_exit = 1;
        if(rs_quit == 0 && rs_reload == 0) {
            rs_log_info("io thread send SIGQUIT signal");
            kill(rs_pid, SIGQUIT);
        }
    }
}
