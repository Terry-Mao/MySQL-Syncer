
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

rs_master_info_t    *rs_master_info = NULL;
rs_binlog_action_t  rs_binlog_actions;

static rs_binlog_action_t rs_def_binlog_actions = {
    rs_def_header_handle,
    rs_def_query_handle,
    rs_def_filter_data_handle,
    rs_def_create_data_handle,
    rs_def_intvar_handle,
    rs_def_xid_handle,
    rs_def_finish_handle
};


int rs_init_master() 
{
    rs_master_info_t    *mi, *om;

    /* set binlog actions */
    rs_binlog_actions = rs_def_binlog_actions;

    /* init master info */
    mi = rs_init_master_info(rs_master_info);
    
    if(mi == NULL) {
        mi = rs_master_info;
        goto free;
    }

    /* free old master info */
    om = rs_master_info;
    rs_master_info = mi;

    if(om != NULL) {
        rs_free_master(om);
    }

    return RS_OK;

free:

    rs_free_master(mi);

    return RS_ERR;
}


void rs_free_master(void *data) 
{
    int                 err;
    rs_master_info_t    *mi;

    mi = (data == NULL ? rs_master_info : (rs_master_info_t *) data);

    if(mi != NULL) {

        rs_log_info("free master");

        if(mi->accept_thread != 0) {
            if((err = pthread_cancel(mi->accept_thread)) != 0) {
                rs_log_err(err, "pthread_cancel() failed, accept_thread");
            } else {
                if((err = pthread_join(mi->accept_thread, NULL)) != 0) {
                    rs_log_err(err, "pthread_join() failed, "
                            "accept_thread");
                }
            }
        }

        if(mi->svr_fd != -1) {
            rs_close(mi->svr_fd);
        }

        if(mi->req_dump_info != NULL) {
            /* close all request_dump*/
            rs_destroy_request_dumps(mi->req_dump_info); 
        }

        /* free conf */
        rs_free_conf(&(mi->conf));

        if(mi->conf.kv != NULL) {
            free(mi->conf.kv);
        }

        free(mi);
    }
}
