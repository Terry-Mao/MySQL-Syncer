
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

rs_master_info_t    *rs_master_info = NULL;

int rs_init_master() 
{
    rs_master_info_t    *mi, *om;

    /* init master info */
    mi = rs_init_master_info(rs_master_info);
    
    if(mi == NULL) {
        return RS_ERR;
    }

    /* free old master info */
    om = rs_master_info;
 
    if(om != NULL) {
        rs_free_master(om);
    }

    rs_master_info = mi;

    return RS_OK;
}


void rs_free_master(void *data) 
{
    int                 err;
    rs_master_info_t    *mi;
    rs_pool_t           *p;

    mi = (data == NULL ? rs_master_info : (rs_master_info_t *) data);

    /* exit accpet thread */
    if(mi->accept_thread != 0) {

        if(!mi->accept_thread_exit) {
            if((err = pthread_cancel(mi->accept_thread)) != 0) {
                rs_log_err(err, "pthread_cancel() failed");
            }
        }

        if((err = pthread_join(mi->accept_thread, NULL)) != 0) {
            rs_log_err(err, "pthread_join() failed");
        }
    }

    /* close listen fd */
    if(mi->svr_fd != -1) {
        rs_close(mi->svr_fd);
    }

    /* close all request_dump*/
    if(mi->req_dump != NULL) {
        rs_destroy_reqdump(mi->req_dump); 
    }

    p = mi->pool;

    /* free conf */
    if(mi->cf != NULL) {
        rs_destroy_conf(mi->cf);
    }

    /* free master info */
    rs_pfree(p, mi, mi->id);

    /* free pool */
    rs_destroy_pool(p);
}
