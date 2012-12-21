
#ifndef _RS_MASTER_H_INCLUDED_
#define _RS_MASTER_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>

typedef struct rs_master_info_s     rs_master_info_t;
typedef struct rs_reqdump_s         rs_reqdump_t;
typedef struct rs_reqdump_data_s    rs_reqdump_data_t;
typedef struct rs_binlog_info_s     rs_binlog_info_t;

extern rs_master_info_t             *rs_master_info;

#include <rs_master_info.h>
#include <rs_dump_listen.h>
#include <rs_read_binlog.h>
#include <rs_parse_binlog.h>
#include <rs_request_dump.h>
#include <rs_filter_binlog.h>

#endif
