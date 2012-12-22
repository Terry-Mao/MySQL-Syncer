
#ifndef _RS_MASTER_INFO_H_INCLUDED_
#define _RS_MASTER_INFO_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

/* conf */
#define RS_MASTER_MODULE_NAME   "master"
#define RS_MASTER_CONF_NUM      10 

/* master public */
#define RS_MASTER_DUMP_THREAD_NUM       36
#define RS_MASTER_LISTEN_ADDR           "127.0.0.1"
#define RS_MASTER_LISTEN_PORT           1919

#define RS_MASTER_RINGBUF_NUM           5000

#define RS_MASTER_POOL_FACTOR           1.5
#define RS_MASTER_POOL_INITSIZE         100
#define RS_MASTER_POOL_MEMSIZE          (100 * 1024 * 1024)
#define RS_MASTER_SENDBUF_SIZE          (1 * 1024)
#define RS_MASTER_IOBUF_SIZE            (1 * 1024)
#define RS_MASTER_SERVER_ID             1

struct rs_master_info_s {
    char                    *listen_addr;   /* dump listen addr */
    int                     listen_port;    /* dump listen port */
    char                    *binlog_idx_file;   /* binlog idx file path  */
    uint32_t                server_id;
    uint32_t                max_dump_thread;
    double                  pool_factor;        /* slab grow factor */
    uint32_t                pool_memsize;
    uint32_t                pool_initsize;
    uint32_t                ringbuf_num;
    uint32_t                sendbuf_size;
    uint32_t                iobuf_size;

    pthread_t               accept_thread;
    unsigned                accept_thread_exit;
    int                     svr_fd;         /* dump listen fd */

    rs_conf_t               *cf;
    rs_reqdump_t            *req_dump;
    rs_pool_t               *pool;
    int32_t                 id;
};

#define rs_master_info_t_init(mi)                                            \
    (mi)->listen_addr = RS_MASTER_LISTEN_ADDR;                               \
    (mi)->listen_port = RS_MASTER_LISTEN_PORT;                               \
    (mi)->svr_fd = -1;                                                       \
    (mi)->binlog_idx_file = NULL;                                            \
    (mi)->accept_thread = 0;                                                 \
    (mi)->accept_thread_exit = 0;                                            \
    (mi)->pool_factor = RS_MASTER_POOL_FACTOR;                               \
    (mi)->pool_memsize = RS_MASTER_POOL_MEMSIZE;                             \
    (mi)->pool_initsize = RS_MASTER_POOL_INITSIZE;                           \
    (mi)->sendbuf_size = RS_MASTER_SENDBUF_SIZE;                             \
    (mi)->iobuf_size = RS_MASTER_IOBUF_SIZE;                                 \
    (mi)->server_id = RS_MASTER_SERVER_ID;                                   \
    (mi)->max_dump_thread = RS_MASTER_DUMP_THREAD_NUM;                       \
    (mi)->req_dump = NULL;                                                   \
    (mi)->pool = NULL;                                                       \
    (mi)->cf = NULL

rs_master_info_t *rs_init_master_info(rs_master_info_t *om); 

#endif
