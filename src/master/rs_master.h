
#ifndef _RS_MASTER_H_INCLUDED_
#define _RS_MASTER_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>

#define RS_MASTER_MODULE_NAME       "master"
#define RS_DUMP_THREADS_NUM         36

typedef struct rs_master_info_s         rs_master_info_t;
typedef struct rs_request_dump_s        rs_request_dump_t;
typedef struct rs_request_dump_info_s   rs_request_dump_info_t;
typedef struct rs_binlog_info_s         rs_binlog_info_t;

extern rs_master_info_t             *rs_master_info;

#include <rs_master_info.h>
#include <rs_dump_listen.h>
#include <rs_read_binlog.h>
#include <rs_parse_binlog.h>
#include <rs_request_dump.h>
#include <rs_filter_binlog.h>
#include <message.pb-c.h>

#endif
