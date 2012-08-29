

#ifndef _RS_REQUEST_DUMP_H_INCLUDED_
#define _RS_REQUEST_DUMP_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

#define RS_SLAVE_CMD_PACK_HEADER_LEN        4
#define RS_REGISTER_SLAVE_CMD_LEN           (1 + PATH_MAX + UINT32_LEN)
#define RS_CMD_RETRY_TIMES                  600

#define RS_RING_BUFFER_EMPTY_SLEEP_USEC     (1000 * 10)


struct rs_request_dump_s {

    int                 cli_fd;
    int                 notify_fd;

    uint32_t            dump_pos;
    uint32_t            dump_num;
    char                dump_file[PATH_MAX + 1];
    char                *binlog_idx_file;
    
    char                *filter_tables;

    uint32_t            server_id;          /* localhost mysql server-id */

    FILE                *binlog_fp;         /* binlog fp */
    FILE                *binlog_idx_fp;     /* binlog idx fp */

    rs_ring_buffer2_t   ring_buf;
    rs_binlog_info_t    binlog_info;
    rs_slab_t           slab;

    pthread_t           io_thread;
    pthread_t           dump_thread;
    rs_buf_t            send_buf;
    
    rs_buf_t            io_buf;

    rs_request_dump_t       *data;
    rs_request_dump_info_t  *rdi;
    
    unsigned            open:1;
    unsigned            io_thread_exit;
    unsigned            dump_thread_exit;
}; 

#define rs_request_dump_t_init(rd)                                           \
    rs_ring_buffer2_t *rb;                                                   \
    rs_binlog_info_t *bi;                                                    \
    rs_buf_t *sb;                                                            \
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
    (rd)->filter_tables = NULL;                                              \
    (rd)->server_id = 0;                                                     \
    (rd)->dump_thread = 0;                                                   \
    (rd)->rdi = NULL;                                                        \
    (rd)->io_thread = 0;                                                     \
    rs_memzero((rd)->dump_file, PATH_MAX + 1);                               \
    rb = &(rd->ring_buf);                                                    \
    bi = &(rd->binlog_info);                                                 \
    sb = &(rd->send_buf);                                                    \
    rs_ring_buffer2_t_init(rb);                                              \
    rs_buf_t_init(sb);                                                       \
    sb = &(rd->io_buf);                                                      \
    rs_buf_t_init(sb);                                                       \
    rs_binlog_info_t_init(bi)                                               

struct rs_request_dump_info_s {

    uint32_t            dump_num;  /* dump threads num */
    uint32_t            free_dump_num;
    rs_request_dump_t   *req_dumps;  /* dump threas pointer */
    rs_request_dump_t   *free_req_dump;

    pthread_mutex_t     req_dump_mutex;
    pthread_attr_t      thread_attr;
};

void *rs_start_dump_thread(void *data); 
void rs_free_dump_thread(void *data);
void rs_free_io_thread(void *data);


int rs_init_request_dump(rs_request_dump_info_t *rdi, uint32_t dump_num); 
void rs_free_request_dump(rs_request_dump_info_t *rdi, rs_request_dump_t *rd); 
rs_request_dump_t *rs_get_request_dump(rs_request_dump_info_t *rdi); 

void rs_free_request_dumps(rs_request_dump_info_t *rdi); 
void rs_destroy_request_dumps(rs_request_dump_info_t *rdi);

#endif
