
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
    int                     i, r, sock_err;
    uint32_t                l;
    int32_t                 pack_len, cp_len;
    ssize_t                 n;
    char                    cbuf[RS_REGISTER_SLAVE_CMD_LEN];
    struct                  sockaddr_in svr_addr;
    rs_slave_info_t         *si;
    rs_ringbuf_data_t       *rbd;

    /* init var */
    i = 0;
    rs_memzero(&svr_addr, sizeof(svr_addr));
    si = (rs_slave_info_t *) data;

    /* push cleanup handle */
    pthread_cleanup_push(rs_free_io_thread, si);

    /* connect to master */
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_port = htons(si->listen_port);

    if (inet_pton(AF_INET, si->listen_addr, &(svr_addr.sin_addr)) != 1) {
        rs_log_err(rs_errno, "inet_pton(\"%s\") failed", si->listen_addr); 
        goto free;
    }

    for( ;; ) {

        si->svr_fd = socket(AF_INET, SOCK_STREAM, 0);

        if(si->svr_fd == -1) {
            rs_log_err(rs_errno, "socket() failed");
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

        if(rs_send_dumpcmd(si, cbuf, &l) != RS_OK) {
            goto retry;
        }

        /* add to ring buffer */
        for( ;; ) {

            r = rs_ringbuf_set(si->ringbuf, &rbd);

            if(r == RS_FULL) {
                sleep(RS_RING_BUFFER_FULL_SLEEP_SEC);
                continue;
            } else if(r == RS_ERR) {
                goto free;
            }

            /* free slab chunk */
            if(rbd->data != NULL) {
                rs_pfree(si->pool, rbd->data, rbd->id); 
            }


            if(rs_recv_tmpbuf(si->recv_buf, si->svr_fd, &pack_len, 4) 
                    != RS_OK) 
            {
                goto retry;
            }

            rs_log_debug(0, "get cmd packet length = %u", pack_len);

            /* alloc memory */
            rbd->len = pack_len;
            rbd->id = rs_palloc_id(si->pool, pack_len);
            rbd->data = rs_palloc(si->pool, pack_len, rbd->id);

            if(rbd->data == NULL) {
                goto free;
            }

            if(rs_recv_tmpbuf(si->recv_buf, si->svr_fd, rbd->data, rbd->len) 
                    != RS_OK) 
            {
                goto retry;
            }

            rs_ringbuf_set_advance(si->ringbuf);
        }

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
        si->io_thread_exit = 1;
        if(rs_quit == 0 && rs_reload == 0) {
            rs_log_info("io thread send SIGQUIT signal");
            kill(rs_pid, SIGQUIT);
        }
    }
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


/* slave info */
si->info_fd = open(si->slave_info, O_CREAT | O_RDWR, 00666);

if(si->info_fd == -1) {
    rs_log_err(rs_errno, "open(\"%s\") failed", si->slave_info);
    goto free;
}

/* parse slave.info */
if(rs_parse_slave_info(si) != RS_OK) {
    goto free;

    static int rs_parse_slave_info(rs_slave_info_t *si) 
    {
        char    *p, buf[RS_SLAVE_INFO_STR_LEN];
        ssize_t n;

        /* binlog path, binlog pos  */
        p = buf;

        n = rs_read(si->info_fd, buf, RS_SLAVE_INFO_STR_LEN);

        if(n < 0) {
            return RS_ERR;
        } else if (n > 0) { 
            buf[n] = '\0';
            p = rs_ncp_str_till(si->dump_file, p, ',', PATH_MAX);
            si->dump_pos = rs_str_to_uint32(p);
        } else {
            /* if a new slave.info file */
            rs_log_err(0, "slave.info is empty, format must \"file,pos\"");
            return RS_ERR;
        }

        rs_log_info("parse slave info file = %s, pos = %u", si->dump_file, 
                si->dump_pos);

        return RS_OK;
    }
}
