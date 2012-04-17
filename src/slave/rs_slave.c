
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>


int rs_init_slave() 
{
    rs_slave_info_t     *si;

    /* init slave info */
    si = rs_init_slave_info(rs_slave_info);

    if(si == NULL) {
        goto free;
    }

    rs_slave_info = si;

    return RS_OK;

free :

    rs_free_slave(si);
    return RS_ERR;
}

void rs_free_slave(void *data)
{
    int             err;
    rs_slave_info_t *si;

    si = data == NULL ? rs_slave_info : (rs_slave_info_t *) data;

    if(si != NULL) {

        if(si->io_thread != 0) {

            if((err = pthread_cancel(si->io_thread)) != 0) {
                rs_log_err(err, "pthread_cancel() failed, io_thread");
            }
        }

        if(si->svr_fd != -1) {
            rs_close(si->svr_fd);
        }

        if(si->redis_thread != 0) {

            if((err = pthread_cancel(si->redis_thread)) != 0) {
                rs_log_err(err, "pthread_cancel() failed, redis_thread");
            }
        }

        if(si->c != NULL) {
            redisFree(si->c); 
        }

        if(si->ring_buf != NULL) {
            rs_free_ring_buffer(si->ring_buf);
            free(si->ring_buf);
        }

        /* close slave info file */
        if(si->info_fd != -1) {
            rs_close(si->info_fd);
        }

        if((err = pthread_attr_destroy(&(si->thread_attr))) != 0) {
            rs_log_err(err, "pthread_attr_destroy() failed, thread_attr");
        }

        if(si->conf != NULL) {
            rs_free_conf(si->conf);
            free(si->conf);
        }

        free(si);
    }

    rs_slave_info = NULL;
}
