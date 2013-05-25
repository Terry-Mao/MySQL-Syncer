
#ifndef _RS_PARSE_BINLOG_H_INCLUDED_
#define _RS_PARSE_BINLOG_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_binlog.h>

typedef int (*rs_binlog_event_handler)(rs_binlog_info_t *);

extern rs_binlog_event_handler rs_binlog_event_handlers[];

#endif
