
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

    if(mi->listen_port <= 0) {
        rs_log_err(0, "listen_port is invalid");
        goto free;
    }
 
    svr_addr.sin_port = htons(mi->listen_port);

    if(mi->listen_addr == NULL) {
        rs_log_err(0, "listen_addr must not be null");
        goto free;
    }

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
    rs_request_dump_t       *rd;

    mi = (rs_master_info_t *) data;

    pthread_cleanup_push(rs_free_accept_thread, mi);

    if(mi == NULL) {
        rs_log_err(0, "accept thread can not get master info struct");
        goto free;
    }

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
        rd = rs_get_request_dump(mi->req_dump_info);

        if(rd == NULL) {
            rs_log_err(0, "no more free request_dump struct");
            rs_close(cli_fd);
            cli_fd = -1;
            continue;
        } 

        rd->cli_fd = cli_fd;

        /* init ring buffer */
        if(rs_init_ring_buffer2(&(rd->ring_buf), RS_RING_BUFFER_NUM) != RS_OK) 
        {
            goto free;
        }

        /* init slab */
        if(rs_init_slab(&(rd->slab), NULL, mi->slab_init_size, mi->slab_factor
                    , mi->slab_mem_size, RS_SLAB_PREALLOC) != RS_OK) 
        {
            goto free;
        }

        /* create dump thread */
        if((err = pthread_create(&(rd->dump_thread), 
                        &(mi->req_dump_info->thread_attr), 
                        rs_start_dump_thread, (void *) rd)) != 0) 
        {
            rs_log_err(err, "pthread_create() failed, req_dump thread");
            goto free;
        }
    }

free:

    pthread_cleanup_pop(1);

    return NULL;
}

static void rs_free_accept_thread(void *data)
{
    rs_master_info_t    *mi;

    mi = (rs_master_info_t *) data;

    /* NOTICE : if reload signal, must skip send SIGQUIT */
    if(mi != NULL) {
        rs_log_info("free accept thread");
        // kill(rs_pid, SIGQUIT);
        rs_close(mi->svr_fd);
        mi->svr_fd = -1;
    }
}
