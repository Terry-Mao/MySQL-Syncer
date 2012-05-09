
#ifndef _RS_SLAVE_INFO_H_INCLUDED_
#define _RS_SLAVE_INFO_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

#define RS_SLAVE_MODULE_NAME            "slave"
#define RS_SLAVE_INFO_STR_LEN           (PATH_MAX + 1 + UINT32_LEN)
#define RS_RING_BUFFER_NUM              50000
#define RS_SLAB_GROW_FACTOR             1.5
#define RS_SLAB_INIT_SIZE               100
#define RS_SLAB_MEM_SIZE                (100 * 1024 * 1024)

typedef struct rs_slave_info_s rs_slave_info_t;

struct rs_slave_info_s {
    int                 info_fd;
    int                 svr_fd;
    redisContext        *c;

    int                 listen_port;
    char                *listen_addr;

    int                 redis_port;
    char                *redis_addr;

    char                *slave_info;

    char                dump_file[PATH_MAX + 1];
    uint32_t            dump_pos;          

    rs_ring_buffer2_t   *ring_buf;
    uint32_t            ring_buf_num;

    rs_slab_t           *slab;

    pthread_t           io_thread;
    pthread_t           redis_thread;

    double              slab_factor;        /* slab grow factor */
    uint32_t            slab_mem_size;
    uint32_t            slab_init_size;

    char                *filter_tables;

    rs_conf_t           conf;
};

#define rs_slave_info_t_init(si)                                             \
    (si)->info_fd = -1;                                                      \
    (si)->svr_fd = -1;                                                       \
    (si)->c = NULL;                                                          \
    (si)->listen_port = -1;                                                  \
    (si)->listen_addr = NULL;                                                \
    (si)->redis_port = -1;                                                   \
    (si)->redis_addr = NULL;                                                 \
    (si)->slave_info = NULL;                                                 \
    rs_memzero((si)->dump_file, PATH_MAX + 1);                               \
    (si)->dump_pos = 0;                                                      \
    (si)->io_thread = 0;                                                     \
    (si)->redis_thread = 0;                                                  \
    (si)->slab_factor = 0;                                                   \
    (si)->slab_mem_size = 0;                                                 \
    (si)->slab_init_size = 0;                                                \
    rs_conf_t_init(&((si)->conf));                                           \
    (si)->slab = NULL;                                                       \
    (si)->ring_buf = NULL;                                                   \
    (si)->filter_tables = NULL;                                              \
    (si)->ring_buf_num = 0


rs_slave_info_t *rs_init_slave_info(rs_slave_info_t *os); 
int rs_flush_slave_info(rs_slave_info_t *si, char *buf, size_t len);

extern rs_slave_info_t                  *rs_slave_info; 

#endif

