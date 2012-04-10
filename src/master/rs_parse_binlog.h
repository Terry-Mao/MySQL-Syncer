
#ifndef _RS_PARSE_BINLOG_H_INCLUDED_
#define _RS_PARSE_BINLOG_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

#define RS_RING_BUFFER_FULL_SLEEP_SEC 1

typedef struct {
    int (*header)(rs_request_dump_t *rd);
    int (*query)(rs_request_dump_t *rd);
    int (*filter_data)(rs_request_dump_t *rd);
    int (*intvar)(rs_request_dump_t *rd);
    int (*xid)(rs_request_dump_t *rd);
    int (*finish)(rs_request_dump_t *rd);
} rs_binlog_action_t;

#define rs_binlog_header_event          rs_binlog_actions.header 
#define rs_binlog_query_event           rs_binlog_actions.query 
#define rs_binlog_filter_data           rs_binlog_actions.filter_data
#define rs_binlog_intvar_event          rs_binlog_actions.intvar
#define rs_binlog_xid_event             rs_binlog_actions.xid
#define rs_binlog_flush_event           rs_binlog_actions.flush_event
#define rs_binlog_finish_event          rs_binlog_actions.finish

extern rs_binlog_action_t rs_binlog_actions;

int rs_redis_header_handle(rs_request_dump_t *rd);
int rs_redis_query_handle(rs_request_dump_t *rd);
int rs_redis_intvar_handle(rs_request_dump_t *rd);
int rs_redis_xid_handle(rs_request_dump_t *rd);
int rs_redis_finish_handle(rs_request_dump_t *rd);

#endif
