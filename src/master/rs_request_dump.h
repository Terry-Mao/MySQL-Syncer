

#ifndef _RS_REQUEST_DUMP_H_INCLUDED_
#define _RS_REQUEST_DUMP_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

#define RS_SLAVE_CMD_PACK_LEN               4
#define RS_CMD_RETRY_TIMES                  600


struct rs_reqdump_data_s {

    int                 cli_fd;
    int                 notify_fd;

    uint32_t            dump_pos;
    uint32_t            dump_num;
    char                dump_file[PATH_MAX + 1];
    char                *binlog_idx_file;
    char                *cbuf;
    uint32_t            rb_esusec;
    
    char                *filter_tables;

    uint32_t            server_id;          /* localhost mysql server-id */

    FILE                *binlog_fp;         /* binlog fp */
    FILE                *binlog_idx_fp;     /* binlog idx fp */

    rs_ringbuf_t        *ringbuf;
    rs_binlog_info_t    binlog_info;
    rs_pool_t           *pool;

    pthread_t           io_thread;
    pthread_t           dump_thread;

    rs_buf_t            *send_buf;
    rs_buf_t            *io_buf;

    rs_reqdump_data_t   *data;
    rs_reqdump_t        *req_dump;
    
    unsigned            open:1;
    unsigned            io_thread_exit:1;
    unsigned            dump_thread_exit:1;
}; 

#define rs_reqdump_data_t_init(rd)                                           \
    rs_binlog_info_t *bi;                                                    \
    (rd)->dump_pos = 0;                                                      \
    (rd)->cli_fd = -1;                                                       \
    (rd)->notify_fd = -1;                                                    \
    (rd)->open = 0;                                                          \
    (rd)->io_thread_exit = 0;                                                \
    (rd)->dump_thread_exit = 0;                                              \
    (rd)->dump_num = 0;                                                      \
    (rd)->binlog_fp = NULL;                                                  \
    (rd)->binlog_idx_fp = NULL;                                              \
    (rd)->binlog_idx_file = NULL;                                            \
    (rd)->rb_esusec = RS_RINGBUF_ESUSEC;                                     \
    (rd)->cbuf = NULL;                                                       \
    (rd)->filter_tables = NULL;                                              \
    (rd)->server_id = 0;                                                     \
    (rd)->dump_thread = 0;                                                   \
    (rd)->req_dump = NULL;                                                   \
    (rd)->io_thread = 0;                                                     \
    rs_memzero((rd)->dump_file, PATH_MAX + 1);                               \
    (rd)->ringbuf = NULL;                                                    \
    (rd)->io_buf = NULL;                                                     \
    (rd)->send_buf = NULL;                                                   \
    (rd)->pool = NULL;                                                       \
    bi = &(rd->binlog_info);                                                 \
    rs_binlog_info_t_init(bi)                                               

struct rs_reqdump_s {

    uint32_t            num;  /* dump threads num */
    uint32_t            free_num;

    rs_reqdump_data_t   *data;  /* dump threas pointer */
    rs_reqdump_data_t   *free;

    pthread_mutex_t     thr_mutex;
    pthread_attr_t      thr_attr;

    rs_pool_t           *pool;
    int32_t             id;
};

void *rs_start_dump_thread(void *data); 
void rs_free_dump_thread(void *data);
void rs_free_io_thread(void *data);


rs_reqdump_t *rs_create_reqdump(rs_pool_t *p, uint32_t num);
rs_reqdump_data_t *rs_get_reqdump_data(rs_reqdump_t *rd);
void rs_free_reqdump_data(rs_reqdump_t *rd, rs_reqdump_data_t *d);
void rs_freeall_reqdump_data(rs_reqdump_t *rd);
void rs_destroy_reqdump(rs_reqdump_t *rd);

#endif
