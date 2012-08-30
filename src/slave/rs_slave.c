
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>


int rs_init_slave() 
{
    rs_slave_info_t     *si;

    /* init slave info */
    si = rs_init_slave_info(NULL);

    if(si == NULL) {
        return RS_ERR;
    }

#if 0
    /* free old slave inof */
    os = rs_slave_info;

    if(os != NULL) {
        rs_free_slave(os);
    }
#endif

    rs_slave_info = si;

    return RS_OK;
}

void rs_free_slave(void *data)
{
    int             err;
    rs_slave_info_t *si;

    si = (data == NULL ? rs_slave_info : (rs_slave_info_t *) data);

    if(si == NULL) {
        return;
    }

    if(si->io_thread != 0) {

        if(!si->io_thread_exit) {

            rs_log_info("cancel io thread");

            if((err = pthread_cancel(si->io_thread)) != 0) {
                rs_log_err(err, "pthread_cancel() failed, io_thread");
            }

            rs_log_info("join io thread");

            if((err = pthread_join(si->io_thread, NULL)) != 0) {
                rs_log_err(err, "pthread_join() failed, io_thread");
            }
        }
    }

    if(si->redis_thread != 0) {

        if(!si->redis_thread_exit) {

            rs_log_info("cancel redis thread");

            if((err = pthread_cancel(si->redis_thread)) != 0) {
                rs_log_err(err, "pthread_cancel() failed, redis_thread");
            }

            rs_log_info("join redis thread");

            if((err = pthread_join(si->redis_thread, NULL)) != 0) {
                rs_log_err(err, "pthread_join() failed, redis_thread");
            }
        }
    }

    if(si->svr_fd != -1) {
        rs_log_info("close master fd");
        rs_close(si->svr_fd);
    }

    if(si->c != NULL) {
        rs_log_info("free redis");
        redisFree(si->c); 
    }

    /* free ring buffer2 */
    if(si->ring_buf != NULL) {
        rs_log_info("free ring buffer");
        rs_free_ring_buffer2(si->ring_buf);
        free(si->ring_buf);
    }

    /* free slab */
    if(si->slab != NULL) {
        rs_log_info("free memory slabs");
        rs_free_slabs(si->slab);
        free(si->slab);
    }

    /* free packbuf */
    if(si->recv_buf != NULL) {
        rs_log_info("free recv buf");
        rs_free_temp_buf(si->recv_buf); 
        free(si->recv_buf);
    }

    /* close slave info file */
    if(si->info_fd != -1) {
        rs_log_info("close slave info file fd");
        rs_close(si->info_fd);
    }

    /* free conf */
    rs_log_info("free slave conf");
    rs_free_conf(&(si->conf));

    if(si->conf.kv != NULL) {
        free(si->conf.kv);
    }

    /* free slave info */
    rs_log_info("free slave info");
    free(si);
}
