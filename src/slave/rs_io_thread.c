
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>



static int rs_send_dumpcmd(rs_slave_info_t *si);
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
    int                     r;
    int32_t                 pack_len;
    struct                  sockaddr_in svr_addr;
    rs_slave_info_t         *si;
    rs_ringbuf_data_t       *rbd;

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
            goto retry;
        }

        if(rs_send_dumpcmd(si) != RS_OK) {
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

            rs_log_slave(0, "get cmd packet length = %u", pack_len);

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

static int rs_send_dumpcmd(rs_slave_info_t *si)
{
    int32_t l;
    ssize_t n;
    l = 2 + rs_strlen(si->dump_info) + rs_strlen(si->filter_tables);
    char buf[4 + l], *p;
    
    p = buf;

    p = rs_cpymem(buf, &l, 4);
    if(snprintf(p, l + 1, "%s,%s,", si->dump_info, si->filter_tables) < 0) {
        rs_log_err(rs_errno, "snprintf() failed");
        return RS_ERR;
    }

    n = rs_write(si->svr_fd, buf, 4 + l);

    if(n != 4 + l) {
        return RS_ERR;
    }

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
