
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>


int rs_init_slave() 
{
    rs_slave_info_t     *si, *os;

    /* init slave info */
    si = rs_init_slave_info(rs_slave_info);

    if(si == NULL) {
        return RS_ERR;
    }

    /* free old slave inof */
    os = rs_slave_info;

    rs_slave_info = si;

    if(os != NULL) {
        rs_free_slave(os);
    }

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

    /* free ring buffer2 */
    if(si->ring_buf != NULL) {
        rs_free_ring_buffer2(si->ring_buf);
        free(si->ring_buf);
    }

    /* free slab */
    if(si->slab != NULL) {
        rs_free_slabs(si->slab);
        free(si->slab);
    }

    /* close slave info file */
    if(si->info_fd != -1) {
        rs_close(si->info_fd);
    }

    if((err = pthread_attr_destroy(&(si->thread_attr))) != 0) {
        rs_log_err(err, "pthread_attr_destroy() failed, thread_attr");
    }

    /* free conf */
    rs_free_conf(&(si->conf));

    if(si->conf.kv != NULL) {
        free(si->conf.kv);
    }

    /* free slave info */
    free(si);
}
