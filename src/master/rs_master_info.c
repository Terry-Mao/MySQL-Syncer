
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>


static int rs_init_master_conf(rs_master_info_t *mi);

rs_master_info_t *rs_init_master_info(rs_master_info_t *om) 
{
    int                 nl, nd, id, err;
    rs_master_info_t    *mi;
    rs_pool_t           *p;

    mi = NULL;
    p = NULL;
    nl = 1;
    nd = 1;

    p = rs_create_pool(200, 1024 * 1024 * 10, 1 * 1024 * 1024, 
            RS_POOL_CLASS_IDX, 1.5, RS_POOL_PREALLOC);

    if(p == NULL) {
        return NULL;
    }

    id = rs_palloc_id(p, sizeof(rs_master_info_t));
    mi = rs_palloc(p, sizeof(rs_master_info_t), id);

    if(mi == NULL) {
        rs_destroy_pool(p);
        return NULL;
    }

    rs_master_info_t_init(mi);

    mi->pool = p;
    mi->id = id;
    mi->cf = rs_create_conf(p, RS_MASTER_CONF_NUM);

    if(mi->cf == NULL) {
        goto free;
    }

    /* init master conf */
    if(rs_init_master_conf(mi) != RS_OK) {
        goto free;
    }

    /* recreate pool */
    p = rs_create_pool(mi->pool_initsize, mi->pool_memsize,  1 * 1024 * 1024, 
            RS_POOL_CLASS_IDX, mi->pool_factor, RS_POOL_PREALLOC);

    if(p == NULL) {
        goto free; 
    }

    rs_free_master(mi);

    id = rs_palloc_id(p, sizeof(rs_master_info_t));
    mi = rs_palloc(p, sizeof(rs_master_info_t), id);

    if(mi == NULL) {
        rs_destroy_pool(p);
        return NULL;
    }

    rs_master_info_t_init(mi);

    mi->pool = p;
    mi->id = id;
    mi->cf = rs_create_conf(p, RS_MASTER_CONF_NUM);

    if(mi->cf == NULL) {
        goto free;
    }

    /* init conf */
    if(rs_init_master_conf(mi) != RS_OK) {
        goto free;
    }

    if(om != NULL) {

        /* exit accpet thread */
        if(om->accept_thread != 0) {
            if((err = pthread_cancel(om->accept_thread)) != 0) {
                rs_log_err(err, "pthread_cancel() failed");
            } else {
                if((err = pthread_join(om->accept_thread, NULL)) != 0) {
                    rs_log_err(err, "pthread_join() failed");
                }
            }
        }

        /* init dump listen */
        if((nl = ((rs_strcmp(mi->listen_addr, om->listen_addr) != 0) || 
                        (mi->listen_port != om->listen_port))))
        {
            rs_log_info("old addr = %s, old port = %d, new addr = %s, "
                    "old port = %d", om->listen_addr, om->listen_port, 
                    mi->listen_addr, mi->listen_port);
        } else {
            rs_log_info("reuse svr_fd");

            mi->svr_fd = om->svr_fd;
            om->svr_fd = -1; /* NOTICE : reuse fd, don't reclose svr_fd */
        }


        nd = (rs_strcmp(mi->binlog_idx_file, om->binlog_idx_file) != 0) && 
            (om->ringbuf_num != mi->ringbuf_num);

        if(!nd) {
            if(nl) {
                rs_log_info("free dump threads");
                /* free dump threads */
                rs_freeall_reqdump_data(om->req_dump);
            }

            rs_log_info("reuse dump threads");

            mi->req_dump = om->req_dump;
            om->req_dump = NULL; /* NOTICE: don't refree req_dump_info */
        }
    } /* end om */

    if(nl) {
        rs_log_info("start listening on %s:%d", mi->listen_addr, 
                mi->listen_port);

        /* init dump listen */
        if(rs_dump_listen(mi) != RS_OK) {
            goto free;
        }
    }

    if(nd) {
        /* init dump threads */
        rs_log_info("start initing dump threads");
        mi->req_dump = rs_create_reqdump(p, mi->max_dump_thread);

        if(mi->req_dump == NULL) {
            goto free;
        }
    }

    /* start accept thread */
    rs_log_info("start accepting client");
    if((err = pthread_create(&(mi->accept_thread), NULL, 
                    rs_start_accept_thread, mi)) != 0) 
    {
        rs_log_err(err, "pthread_create() failed");
        goto free;
    }

    return mi;

free:

    rs_free_master(mi);
    return NULL;
}

static int rs_init_master_conf(rs_master_info_t *mi)
{
    if(rs_conf_register(mi->cf, "listen.addr", &(mi->listen_addr), 
                RS_CONF_STR) != RS_OK)  
    {
        return RS_ERR;
    }

    if(rs_conf_register(mi->cf, "listen.port", &(mi->listen_port), 
                RS_CONF_INT32) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(mi->cf, "binlog.index", &(mi->binlog_idx_file), 
                RS_CONF_STR) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(mi->cf, "pool.factor", &(mi->pool_factor), 
                RS_CONF_DOUBLE) != RS_OK)
    {
        return RS_ERR;    
    }

    if(rs_conf_register(mi->cf, "pool.memsize", &(mi->pool_memsize), 
                RS_CONF_UINT32) != RS_OK)
    {
        return RS_ERR; 
    }

    if(rs_conf_register(mi->cf, "pool.initsize", &(mi->pool_initsize), 
                RS_CONF_UINT32) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(mi->cf, "ringbuf.num", &(mi->ringbuf_num), 
                RS_CONF_UINT32) != RS_OK) 
    {
        return RS_ERR;                         
    }
            
    if(rs_conf_register(mi->cf, "sendbuf.size", &(mi->sendbuf_size), 
                RS_CONF_UINT32) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(mi->cf, "server.id", &(mi->server_id), 
                RS_CONF_UINT32) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(mi->cf, "iobuf.size", &(mi->iobuf_size), 
                RS_CONF_UINT32) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(mi->cf, "dump.thread", &(mi->max_dump_thread), 
                RS_CONF_UINT32) != RS_OK)
    {
        return RS_ERR;
    }

    /* init master conf */
    return rs_init_conf(mi->cf, rs_conf_path, RS_MASTER_MODULE_NAME);
}
