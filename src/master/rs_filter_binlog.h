
#ifndef _RS_FILTER_BINLOG_H_INCLUDED_
#define _RS_FILTER_BINLOG_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

#define RS_SKIP_DATA_FLUSH_NUM          900
#define RS_MYSQL_SKIP_DATA              0

int rs_binlog_filter_data(rs_reqdump_data_t *d);
int rs_binlog_create_data(rs_reqdump_data_t *d);


#endif
