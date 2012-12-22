
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
    rs_pool_t       *p;

    si = (data == NULL ? rs_slave_info : (rs_slave_info_t *) data);

    if(si == NULL) {
        return;
    }

    if(si->io_thread != 0 && !si->io_thread_exit) {

        rs_log_info("cancel io thread");
        if((err = pthread_cancel(si->io_thread)) != 0) {
            rs_log_err(err, "pthread_cancel() failed");
        }

        rs_log_info("join io thread");
        if((err = pthread_join(si->io_thread, NULL)) != 0) {
            rs_log_err(err, "pthread_join() failed");
        }
    }

    if(si->redis_thread != 0 && !si->redis_thread_exit) {

        rs_log_info("cancel redis thread");
        if((err = pthread_cancel(si->redis_thread)) != 0) {
            rs_log_err(err, "pthread_cancel() failed");
        }

        rs_log_info("join redis thread");
        if((err = pthread_join(si->redis_thread, NULL)) != 0) {
            rs_log_err(err, "pthread_join() failed");
        }
    }

    if(si->svr_fd != -1) {
        rs_close(si->svr_fd);
    }

    if(si->c != NULL) {
        redisFree(si->c); 
    }

    /* free ring buffer2 */
    if(si->ringbuf != NULL) {
        rs_destroy_ringbuf(si->ringbuf);
    }

    /* free packbuf */
    if(si->recv_buf != NULL) {
        rs_destroy_tmpbuf(si->recv_buf); 
    }

    /* close slave info file */
    if(si->info_fd != -1) {
        rs_close(si->info_fd);
    }

    if(si->table_func != NULL) {
        rs_destroy_shash(si->table_func);
    }

    p = si->pool;

    /* free conf */
    if(si->cf != NULL) {
        rs_destroy_conf(si->cf);
    }

    rs_pfree(p, si, si->id);

    rs_destroy_pool(p);
}
