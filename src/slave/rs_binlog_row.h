
#ifndef _RS_BINLOG_ROW_H_INCLUDED_
#define _RS_BINLOG_ROW_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

/* BINLOG EVENT TYPE */
#define RS_MYSQL_SKIP_DATA          0
#define RS_XID_EVENT                16
#define RS_WRITE_ROWS_EVENT         23
#define RS_UPDATE_ROWS_EVENT        24
#define RS_DELETE_ROWS_EVENT        25


/* MYSQL COLUMN TYPE  */
#define RS_MYSQL_TYPE_DECIMAL       0
#define RS_MYSQL_TYPE_TINYINT       1
#define RS_MYSQL_TYPE_SHORT         2
#define RS_MYSQL_TYPE_LONG          3
#define RS_MYSQL_TYPE_FLOAT         4
#define RS_MYSQL_TYPE_DOUBLE        5
#define RS_MYSQL_TYPE_NULL          6
#define RS_MYSQL_TYPE_TIMESTAMP     7
#define RS_MYSQL_TYPE_LONGLONG      8
#define RS_MYSQL_TYPE_INT24         9
#define RS_MYSQL_TYPE_DATE          10
#define RS_MYSQL_TYPE_TIME          11
#define RS_MYSQL_TYPE_DATETIME      12
#define RS_MYSQL_TYPE_YEAR          13
#define RS_MYSQL_TYPE_VARCHAR       15
#define RS_MYSQL_TYPE_BIT           16
#define RS_MYSQL_TYPE_TINY_BLOB     249
#define RS_MYSQL_TYPE_MEDIUM_BLOB   250
#define RS_MYSQL_TYPE_LONG_BLOB     251
#define RS_MYSQL_TYPE_BLOB          252
#define RS_MYSQL_TYPE_VAR_STRING    253
#define RS_MYSQL_TYPE_STRING        254
#define RS_MYSQL_TYPE_GEOMETRY      255

typedef char *(*rs_binlog_parse_column_pt)(char *p, u_char *cm, uint32_t ml, 
        uint32_t fl, uint32_t *dl);

typedef struct {
    uint32_t                    meta_len;
    uint32_t                    fixed_len;    
    rs_binlog_parse_column_pt   parse_column_handle;
} rs_binlog_column_meta_t;



typedef int (*rs_dml_binlog_row_pt)(rs_slave_info_t *si, void *obj);

typedef void (*rs_parse_binlog_row_pt)(char *p, uint32_t dl, uint32_t i, 
        void *obj);

int rs_dml_binlog_row(rs_slave_info_t *si, void *data, 
        uint32_t len, char type, rs_dml_binlog_row_pt write_handle, 
        rs_dml_binlog_row_pt before_update_handle,
        rs_dml_binlog_row_pt update_handle,
        rs_dml_binlog_row_pt  delete_handle,
        rs_parse_binlog_row_pt parse_handle, void *obj);

#endif
