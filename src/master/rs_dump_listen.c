
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
        rs_log_error(RS_LOG_ERR, rs_errno, "inet_pton() failed, %s", 
                mi->listen_addr);
        goto free;
    }

    for(tries = RS_RETRY_BIND_TIMES; tries; tries--) {
        mi->svr_fd = socket(AF_INET, SOCK_STREAM, 0);

        if(mi->svr_fd == -1) {
            rs_log_error(RS_LOG_ERR, rs_errno, "socket() failed");
            goto free;
        }

        if(setsockopt(mi->svr_fd, SOL_SOCKET, SO_REUSEADDR, 
                    (const void *) &reuseaddr, sizeof(int)) == -1)
        {
            rs_log_error(RS_LOG_ERR, rs_errno, "setsockopt(REUSEADDR) failed");
            goto free;
        }

        if(bind(mi->svr_fd, (const struct sockaddr *) &svr_addr, 
                    sizeof(svr_addr)) == -1) 
        {
            err = errno;
            mi->svr_fd = -1;

            if(err != EADDRINUSE) {
                rs_log_error(RS_LOG_ERR, err, "bind() failed");
                goto free;
            }

            rs_log_error(RS_LOG_INFO, 0, "try again to bind() after %ums"
                    , RS_RETRY_BIND_SLEEP_MSC);

            usleep(RS_RETRY_BIND_SLEEP_MSC * 1000);
            continue;
        }

        if(listen(mi->svr_fd, RS_BACKLOG) == -1) {
            rs_log_error(RS_LOG_ERR, rs_errno, "listen() failed");
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
    rs_reqdump_data_t       *rd;

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

            rs_log_error(RS_LOG_ERR, rs_errno, "accept() failed");
            goto free;
        }

        /* register slave */
        rd = rs_get_reqdump_data(mi->req_dump);

        if(rd == NULL) {
            goto free;
        }

        rd->pool = rs_create_pool(mi->pool_initsize, mi->pool_memsize, 
                1 * 1024 * 1024, RS_POOL_CLASS_IDX, mi->pool_factor, 
                RS_POOL_PREALLOC);

        if(rd->pool == NULL) {
            goto free;
        }

        /* init ring buffer */
        if((rd->ringbuf = rs_create_ringbuf(rd->pool, mi->ringbuf_num)) 
                == NULL)
        {
            goto free;
        }

        /* init packbuf */
        if((rd->send_buf = rs_create_tmpbuf(mi->sendbuf_size)) == NULL) {
            goto free; 
        }

        /* init iobuf */
        if((rd->io_buf = rs_create_tmpbuf(mi->iobuf_size)) == NULL) {
            goto free;
        }

        rd->cli_fd = cli_fd;
        rd->binlog_idx_file = mi->binlog_idx_file;
        rd->req_dump = mi->req_dump;
        rd->server_id = mi->server_id;

        /* create dump thread */
        if((err = pthread_create(&(rd->dump_thread), 
                        &(rd->req_dump->thr_attr), 
                        rs_start_dump_thread, (void *) rd)) != 0) 
        {
            rs_log_error(RS_LOG_ERR, err, "pthread_create() failed");
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
