

#ifndef _RS_REDIS_THREAD_H_INCLUDED_
#define _RS_REDIS_THREAD_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>


#define RS_REDIS_CONNECT_RETRY_SLEEP_SEC    10
#define RS_RING_BUFFER_EMPTY_SLEEP_USEC     (1000 * 10)
#define RS_REDIS_CMD_COMMIT_NUM             36

#define RS_MYSQL_SKIP_DATA          0

#define RS_XID_EVENT                 16
#define RS_WRITE_ROWS_EVENT          23
#define RS_UPDATE_ROWS_EVENT         24
#define RS_DELETE_ROWS_EVENT         25



#define RS_FILTER_TABLE_TEST_TEST   "test.test"


typedef int (*rs_redis_dml_row_based_pt)(rs_slave_info_t *si, void *obj);

typedef char *(*rs_redis_parse_row_based_pt)(char *p, u_char *cm, uint32_t i, 
        void *obj);


void *rs_start_redis_thread(void *data);


typedef struct {
    uint32_t    id;
    char        col[30];
} rs_mysql_test_t;

#define rs_mysql_test_t_init(test)                                           \
    (test)->id = 0;                                                          \
    rs_memzero((test)->col, 30)

#endif
