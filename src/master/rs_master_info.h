
#ifndef _RS_MASTER_INFO_H_INCLUDED_
#define _RS_MASTER_INFO_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>


#define RS_MASTER_CONF_NUM  5

struct rs_master_info_s {

    rs_conf_kv_t            *conf;
    char                    *listen_addr;   /* dump listen addr */
    int                     listen_port;    /* dump listen port */
    int                     svr_fd;         /* dump listen fd */

    char                    *binlog_idx_file;   /* binlog idx file path  */

    pthread_t               accept_thread;
    rs_request_dump_info_t  *req_dump_info;
};

#define rs_master_info_t_init(mi)                                            \
    mi->conf = NULL;                                                         \
    mi->listen_addr = NULL;                                                  \
    mi->listen_port = 0;                                                     \
    mi->svr_fd = -1;                                                         \
    mi->binlog_idx_file = NULL;                                              \
    mi->accept_thread = 0 

rs_master_info_t *rs_init_master_info(rs_master_info_t *om); 

#endif
