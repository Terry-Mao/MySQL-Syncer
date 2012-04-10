
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

#define RS_MYSQL_TEST_COLUMN_NUM     3
#define RS_MYSQL_TEST_VAR_COLUMN_NUM 1
#define RS_MYSQL_TEST_NULL_BIT_NUM                                           \
    rs_align((RS_MYSQL_TEST_COLUMN_NUM / 8), 1)

#define RS_SYNC_DATA_CMD_SIZE           (PATH_MAX + UINT32_LEN + 3)

#define RS_MYSQL_TEST_DATA_SIZE                                           \
    1

#define RS_SYNC_DATA_SIZE                                                    \
    (RS_SYNC_DATA_CMD_SIZE + rs_max(RS_MYSQL_TEST_DATA_SIZE, 0))

int rs_def_filter_data_handle(rs_request_dump_t *rd);
int rs_def_create_data_handle(rs_request_dump_t *rd);


#endif
