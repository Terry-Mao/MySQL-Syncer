
#ifndef _RS_SLAVE_INFO_H_INCLUDED_
#define _RS_SLAVE_INFO_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

#define RS_SLAVE_MODULE_NAME            "slave"
#define RS_SLAVE_CONF_NUM               13

#define RS_SLAVE_INFO_STR_LEN           (PATH_MAX + 1 + UINT32_LEN + 1)

#define RS_SLAVE_LISTEN_ADDR            "localhost"
#define RS_SLAVE_LISTEN_PORT            1919 
#define RS_SLAVE_REDIS_ADDR             "localhost"
#define RS_SLAVE_REDIS_PORT             6379
#define RS_RING_BUFFER_NUM              50000
#define RS_SLAVE_POOL_FACTOR            1.5
#define RS_SLAVE_POOL_INITSIZE          100
#define RS_SLAVE_POOL_MEMSIZE           (100 * 1024 * 1024)
#define RS_BINLOG_SAVE                  900
#define RS_BINLOG_SAVESEC               60
#define RS_SLAB_MEM_SIZE                (100 * 1024 * 1024)
#define RS_PACKBUF_LEN                  (1 * 1024)

typedef struct rs_slave_info_s rs_slave_info_t;

struct rs_slave_info_s {
    int                 listen_port;
    char                *listen_addr;
    int                 redis_port;
    char                *redis_addr;
    char                *slave_info;
    uint32_t            ringbuf_num;
    double              pool_factor;        /* slab grow factor */
    uint32_t            pool_memsize;
    uint32_t            pool_initsize;
    uint32_t            binlog_save;
    uint32_t            cur_binlog_save;
    uint32_t            binlog_savesec;
    uint64_t            cur_binlog_savesec;
    char                *filter_tables;
    uint32_t            recvbuf_size;

    int                 info_fd;
    int                 svr_fd;
    uint32_t            cmdn;
    char                dump_info[RS_SLAVE_INFO_STR_LEN];

    redisContext        *c;
    rs_ringbuf_t        *ringbuf;
    rs_buf_t            *recv_buf;
    rs_conf_t           *cf;
    rs_shash_t          *table_func;

    rs_pool_t           *pool;
    int32_t             id;

    pthread_t           io_thread;
    pthread_t           redis_thread;
    unsigned            io_thread_exit:1;
    unsigned            redis_thread_exit:1;
};

#define rs_slave_info_t_init(si)                                             \
    (si)->listen_addr = RS_SLAVE_LISTEN_ADDR;                                \
    (si)->listen_port = RS_SLAVE_LISTEN_PORT;                                \
    (si)->redis_addr = RS_SLAVE_REDIS_ADDR;                                  \
    (si)->redis_port = RS_SLAVE_REDIS_PORT;                                  \
    (si)->slave_info = NULL;                                                 \
    (si)->pool_factor = RS_SLAVE_POOL_FACTOR;                                \
    (si)->pool_initsize = RS_SLAVE_POOL_INITSIZE;                            \
    (si)->pool_memsize = RS_SLAVE_POOL_MEMSIZE;                              \
    (si)->binlog_save = 0;                                                   \
    (si)->cur_binlog_save = 0;                                               \
    (si)->binlog_savesec = 0;                                                \
    (si)->cur_binlog_savesec = 0;                                            \
    (si)->filter_tables = NULL;                                              \
    (si)->ringbuf_num = 0;                                                   \
    (si)->recvbuf_size = 0;                                                  \
    (si)->info_fd = -1;                                                      \
    (si)->svr_fd = -1;                                                       \
    (si)->c = NULL;                                                          \
    (si)->cf = NULL;                                                         \
    (si)->ringbuf = NULL;                                                    \
    (si)->cmdn = 0;                                                          \
    (si)->recv_buf = NULL;                                                   \
    (si)->table_func = NULL;                                                 \
    (si)->io_thread = 0;                                                     \
    (si)->redis_thread = 0;                                                  \
    (si)->io_thread_exit = 0;                                                \
    (si)->redis_thread_exit = 0;                                             \
    (si)->pool = NULL 


rs_slave_info_t *rs_init_slave_info(rs_slave_info_t *os); 

extern rs_slave_info_t                  *rs_slave_info; 

#endif

