
#ifndef _RS_FILTER_BINLOG_H_INCLUDED_
#define _RS_FILTER_BINLOG_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

#define RS_RING_BUFFER_FULL_SLEEP_SEC 1

#define RS_FILTER_DB_NAME           "test"

#define RS_MYSQL_SKIP_DATA          0
#define RS_MYSQL_INSERT_TEST        1
#define RS_MYSQL_DELETE_TEST        2

#define RS_MYSQL_TEST_COLUMN_NUM     2
#define RS_MYSQL_TEST_VAR_COLUMN_NUM 1
#define RS_MYSQL_TEST_NULL_BIT_NUM                                           \
    ((RS_MYSQL_TEST_COLUMN_NUM - 1) / 8 + 1)

#define RS_SYNC_DATA_CMD_SIZE           (PATH_MAX + UINT32_LEN + 3)

/* nullbit + id(8) + msg_len(4) + msg_data */
#define RS_MYSQL_TEST_DATA_SIZE                                              \
    RS_MYSQL_TEST_NULL_BIT_NUM + 8 + 4 + 210

#define RS_SYNC_DATA_SIZE                                                    \
    (RS_SYNC_DATA_CMD_SIZE + rs_max(RS_MYSQL_TEST_DATA_SIZE, 0))


typedef struct {

    char        nullbit[RS_MYSQL_TEST_NULL_BIT_NUM]; 
    uint64_t    id;         /* NULL BIT = index : 0, pos = 0 */
    char        msg[210];   /* NULL BIT = index : 0, pos = 1 */

} rs_mysql_test_t;

#define rs_mysql_test_t_init(t)                                              \
    rs_memzero((t)->nullbit, RS_MYSQL_TEST_NULL_BIT_NUM);                    \
    (t)->id = 0;                                                             \
    rs_memzero((t)->msg, 210)


#define TEST_FILTER_KEY         "INSERT INTO test(msg) VALUES("
#define TEST_FILTER_KEY_LEN     (sizeof(TEST_FILTER_KEY) - 1)

#define TEST_COLUMN_MSG         "INSERT INTO test(msg) VALUES('"
#define TEST_COLUMN_MSG_LEN     (sizeof(TEST_COLUMN_MSG) - 1)


int rs_def_filter_data_handle(rs_request_dump_t *rd);
int rs_def_create_data_handle(rs_request_dump_t *rd);


#endif
