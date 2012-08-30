
#ifndef _RS_BINLOG_ROW_H_INCLUDED_
#define _RS_BINLOG_ROW_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>


#define RS_MYSQL_SKIP_DATA           0
#define RS_XID_EVENT                 16
#define RS_WRITE_ROWS_EVENT          23
#define RS_UPDATE_ROWS_EVENT         24
#define RS_DELETE_ROWS_EVENT         25

typedef int (*rs_dml_binlog_row_pt)(rs_slave_info_t *si, void *obj);

typedef char *(*rs_parse_binlog_row_pt)(char *p, u_char *ct, u_char *cm, 
        uint32_t i, void *obj);

int rs_dml_binlog_row(rs_slave_info_t *si, void *data, 
        uint32_t len, char type, rs_dml_binlog_row_pt write_handle, 
        rs_dml_binlog_row_pt before_update_handle,
        rs_dml_binlog_row_pt update_handle,
        rs_dml_binlog_row_pt  delete_handle,
        rs_parse_binlog_row_pt parse_handle, void *obj);

#endif
