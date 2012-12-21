
#ifndef _RS_PARSE_BINLOG_H_INCLUDED_
#define _RS_PARSE_BINLOG_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>


typedef int (*rs_binlog_func)(rs_reqdump_data_t *);

typedef struct {
    char            *ev;
    rs_binlog_func  func;
} rs_binlog_func_t;

int rs_binlog_header_handler(rs_reqdump_data_t *rd);
int rs_binlog_query_handler(rs_reqdump_data_t *rd);
int rs_binlog_intvar_handler(rs_reqdump_data_t *rd);
int rs_binlog_xid_handler(rs_reqdump_data_t *rd);
int rs_binlog_table_map_handler(rs_reqdump_data_t *rd);
int rs_binlog_write_rows_handler(rs_reqdump_data_t *rd);
int rs_binlog_update_rows_handler(rs_reqdump_data_t *rd);
int rs_binlog_delete_rows_handler(rs_reqdump_data_t *rd);
int rs_binlog_finish_handler(rs_reqdump_data_t *rd);
int rs_binlog_skip_handler(rs_reqdump_data_t *rd);
int rs_binlog_stop_handler(rs_reqdump_data_t *rd);



extern rs_binlog_func_t rs_binlog_funcs[];

#endif
