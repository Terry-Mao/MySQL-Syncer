
#ifndef _RS_SLAVE_INFO_H_INCLUDED_
#define _RS_SLAVE_INFO_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

#define SLAVE_MODULE_NAME       "slave"
#define RS_SLAVE_CONF_NUM       6
#define RS_SLAVE_INFO_STR_LEN   PATH_MAX + 1 + UINT32_LEN

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

    rs_ring_buffer_t    *ring_buf;

    pthread_t           io_thread;
    pthread_t           redis_thread;
    pthread_attr_t      thread_attr;

    rs_conf_kv_t        *conf;
};

#define rs_slave_info_t_init(si)                                             \
    si->info_fd = -1;                                                        \
    si->svr_fd = -1;                                                         \
    si->c = NULL;                                                            \
    si->listen_port = -1;                                                    \
    si->listen_addr = NULL;                                                  \
    si->redis_port = -1;                                                     \
    si->redis_addr = NULL;                                                   \
    si->slave_info = NULL;                                                   \
    rs_memzero(si->dump_file, PATH_MAX + 1);                                 \
    si->dump_pos = 0;                                                        \
    si->io_thread = 0;                                                       \
    si->redis_thread = 0;                                                    \
    si->ring_buf = NULL


rs_slave_info_t *rs_init_slave_info(rs_slave_info_t *os); 
int rs_flush_slave_info(rs_slave_info_t *si, char *buf, size_t len);

#endif

