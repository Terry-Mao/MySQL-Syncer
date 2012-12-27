
#ifndef _RS_BINLOG_ROW_H_INCLUDED_
#define _RS_BINLOG_ROW_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>


#define RS_SKIP_COPY_COLUMN_VAL     -1

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

#define RS_DM_DATA_STACK    0
#define RS_DM_DATA_POOL     1

#define RS_DM_TYPE_DEF      0
#define RS_DM_TYPE_HEX      1

typedef char *(*rs_binlog_parse_func) (char *, u_char *, uint32_t, uint32_t, 
        uint32_t *);
typedef void (*rs_binlog_obj_func) (void *);
typedef int (*rs_binlog_dm_func)(rs_slave_info_t *si, void *obj);

typedef struct {
    uint32_t                meta_len;
    uint32_t                fixed_len;    
    rs_binlog_parse_func    parse_handle;
} rs_binlog_column_meta_t;


typedef struct {
    int pos;
    int alloc;
    int type;
} rs_dm_pos_alloc_t;


int rs_dm_binlog_row(rs_slave_info_t *si, void *data, uint32_t len, char type, 
        rs_binlog_obj_func before_parse_handle,        
        rs_binlog_obj_func after_parse_handle,        
        rs_binlog_dm_func write_handle, 
        rs_binlog_dm_func before_update_handle,
        rs_binlog_dm_func update_handle,
        rs_binlog_dm_func delete_handle,
        rs_dm_pos_alloc_t *pas_arr, void *obj);

#endif
