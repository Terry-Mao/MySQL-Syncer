
#ifndef _RS_FILTER_BINLOG_H_INCLUDED_
#define _RS_FILTER_BINLOG_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

#define RS_RING_BUFFER_FULL_SLEEP_SEC   1
#define RS_SKIP_DATA_FLUSH_NUM          900


#define RS_MYSQL_SKIP_DATA          0

int rs_def_filter_data_handle(rs_request_dump_t *rd);
int rs_def_create_data_handle(rs_request_dump_t *rd);


#endif
