
#ifndef _RS_PARSE_BINLOG_H_INCLUDED_
#define _RS_PARSE_BINLOG_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>


typedef int (*rs_event_handler)(rs_request_dump_t);

typedef struct {
    char                *key;
    rs_event_handler    *handler;
} rs_binlog_event_map_t;

int rs_header_handler(rs_request_dump_t *rd);
int rs_query_handler(rs_request_dump_t *rd);
int rs_intvar_handler(rs_request_dump_t *rd);
int rs_xid_handler(rs_request_dump_t *rd);
int rs_table_map_handler(rs_request_dump_t *rd);
int rs_write_rows_handler(rs_request_dump_t *rd);
int rs_update_rows_handler(rs_request_dump_t *rd);
int rs_delete_rows_handler(rs_request_dump_t *rd);
int rs_finish_handler(rs_request_dump_t *rd);
int rs_skip_handler(rs_request_dump_t *rd);
int rs_stop_handler(rs_request_dump_t *rd);


extern rs_binlog_event_map_t rs_binlog_event_map[];

#endif
