
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>


static void rs_free_accept_thread(void *data);

/*
 *  Description
 *      Create listen fd for registering slaves.
 *      
 *  
 *  Return Value
 *      On success, RS_OK is returned. On error, RS_ERR is returned
 *
 */
int rs_dump_listen(rs_master_info_t *mi) 
{
    int     err, tries, reuseaddr;
    struct  sockaddr_in svr_addr;

    /* init var */
    reuseaddr = 1;
    rs_memzero(&svr_addr, sizeof(svr_addr));

    svr_addr.sin_family = AF_INET;
    svr_addr.sin_port = htons(mi->listen_port);

    if (inet_pton(AF_INET, mi->listen_addr, &(svr_addr.sin_addr)) != 1) {
        rs_log_err(rs_errno, "inet_pton() failed, %s", mi->listen_addr);
        goto free;
    }

    for(tries = RS_RETRY_BIND_TIMES; tries; tries--) {

        mi->svr_fd = socket(AF_INET, SOCK_STREAM, 0);

        if(mi->svr_fd == -1) {
            rs_log_err(rs_errno, "socket() failed");
            goto free;
        }

        if(setsockopt(mi->svr_fd, SOL_SOCKET, SO_REUSEADDR, 
                    (const void *) &reuseaddr, sizeof(int)) == -1)
        {
            rs_log_err(rs_errno, "setsockopt(SO_REUSEADDR) failed, %s", 
                    mi->listen_addr);
            goto free;
        }

        if(bind(mi->svr_fd, (const struct sockaddr *) &svr_addr, 
                    sizeof(svr_addr)) == -1) 
        {
            err = errno;

            rs_close(mi->svr_fd);
            mi->svr_fd = -1;

            if(err != EADDRINUSE) {
                rs_log_err(err, "bind() failed, %s", mi->listen_addr);
                goto free;
            }

            rs_log_info(0, "try again to bind() after %ums"
                    , RS_RETRY_BIND_SLEEP_MSC);

            usleep(RS_RETRY_BIND_SLEEP_MSC * 1000);

            continue;
        }

        if(listen(mi->svr_fd, RS_BACKLOG) == -1) {
            rs_log_err(rs_errno, "listen() failed, %s", mi->listen_addr);
            goto free;
        }

        break;
    }

    if(!tries) {
        goto free;
    }

    return RS_OK;

free : 
    return RS_ERR;
}

void *rs_start_accept_thread(void *data) 
{
    int                     err, cli_fd;
    socklen_t               socklen;
    rs_master_info_t        *mi;
    struct                  sockaddr_in cli_addr;
    rs_reqdump_data_t       *d;

    mi = (rs_master_info_t *) data;

    pthread_cleanup_push(rs_free_accept_thread, mi);

    for( ;; ) {

        socklen = 0;
        rs_memzero(&cli_addr, sizeof(cli_addr));

        cli_fd = accept(mi->svr_fd, (struct sockaddr *) &cli_addr, &socklen);

        if(cli_fd == -1) {
        
            if(rs_errno == EINTR) {
                continue;
            }

            rs_log_err(rs_errno, "accept() failed");
            goto free;
        }

        /* register slave */
        d = rs_get_reqdump_data(mi->req_dump);

        if(d == NULL) {
            rs_close(cli_fd);
            cli_fd = -1;
            continue;
        }

        /* init ring buffer */
        if((d->ringbuf = rs_create_ringbuf(mi->pool, mi->ringbuf_num)) 
                == NULL)
        {
            goto free;
        }

        /* init packbuf */
        if((d->send_buf = rs_create_tmpbuf(mi->sendbuf_size)) == NULL) {
            goto free; 
        }

        /* init iobuf */
        if((d->io_buf = rs_create_tmpbuf(mi->iobuf_size)) == NULL) {
            goto free;
        }

        d->binlog_func = mi->binlog_func;
        d->pool = mi->pool;
        d->cli_fd = cli_fd;
        d->binlog_idx_file = mi->binlog_idx_file;
        d->req_dump = mi->req_dump;
        d->server_id = mi->server_id;

        /* create dump thread */
        if((err = pthread_create(&(d->dump_thread), 
                        &(d->req_dump->thr_attr), 
                        rs_start_dump_thread, (void *) d)) != 0) 
        {
            rs_log_err(err, "pthread_create() failed");
            goto free;
        }
    }

free:;

    pthread_cleanup_pop(1);
    pthread_exit(NULL);
}

static void rs_free_accept_thread(void *data)
{
    rs_master_info_t    *mi;

    mi = (rs_master_info_t *) data;
    mi->accept_thread_exit = 1;
}
