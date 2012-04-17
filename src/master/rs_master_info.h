
#ifndef _RS_MASTER_INFO_H_INCLUDED_
#define _RS_MASTER_INFO_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

#define RS_RING_BUFFER_NUM              50000
#define RS_SLAB_GROW_FACTOR             1.5
#define RS_SLAB_INIT_SIZE               100
#define RS_SLAB_MEM_SIZE                (100 * 1024 * 1024)

struct rs_master_info_s {
    char                    *listen_addr;   /* dump listen addr */
    int                     listen_port;    /* dump listen port */
    int                     svr_fd;         /* dump listen fd */

    char                    *binlog_idx_file;   /* binlog idx file path  */

    pthread_t               accept_thread;
    rs_request_dump_info_t  *req_dump_info;

    double                  slab_factor;        /* slab grow factor */
    uint32_t                slab_mem_size;
    uint32_t                slab_init_size;

    rs_conf_t               conf;
};

#define rs_master_info_t_init(mi)                                            \
    mi->listen_addr = NULL;                                                  \
    mi->listen_port = 0;                                                     \
    mi->svr_fd = -1;                                                         \
    mi->binlog_idx_file = NULL;                                              \
    mi->accept_thread = 0;                                                   \
    mi->slab_factor = 0;                                                     \
    mi->slab_mem_size = 0;                                                   \
    mi->slab_init_size = 0;                                                  \
    rs_conf_t_init(&(mi->conf))

rs_master_info_t *rs_init_master_info(rs_master_info_t *om); 

#endif
