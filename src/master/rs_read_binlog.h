
#ifndef _RS_READ_BINLOG_H_INCLUDED_
#define _RS_READ_BINLOG_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

#define RS_DATABASE_NAME_MAX_LEN       64
#define RS_TABLE_NAME_MAX_LEN          64
#define RS_SQL_MAX_LEN                 1024

#define RS_TRAN_KEYWORD                "BEGIN"
#define RS_TRAN_KEYWORD_LEN            sizeof("BEGIN") - 1

#define RS_TRAN_END_KEYWORD            "COMMIT"
#define RS_TRAN_END_KEYWORD_LEN        sizeof("COMMIT") - 1

#define RS_BINLOG_EOF_WAIT_SEC      1


struct rs_binlog_info_s {

    uint32_t    np;
    uint32_t    el;
    uint32_t    sbl;
    uint32_t    tbl;
    uint32_t    sl;
    uint32_t    dbl;
    uint64_t    ai;

    uint32_t    skip_n;

    /* char        eh[RS_BINLOG_EVENT_HEADER_LEN]; */
    char        sql[RS_SQL_MAX_LEN + 1];
    char        db[RS_DATABASE_NAME_MAX_LEN + 1];
    char        tb[RS_TABLE_NAME_MAX_LEN + 1];
    void        *data;

    char        t;
    char        intvar_t;

    char        mev; // Mission Event type

    unsigned    flush      :1;
    unsigned    sent       :1;
    unsigned    tran       :1;
    unsigned    auto_incr  :1;

};

#define rs_binlog_info_t_init(bi)                                            \
        (bi)->np = 0;                                                        \
        (bi)->el = 0;                                                        \
        (bi)->sbl = 0;                                                       \
        (bi)->tbl = 0;                                                       \
        (bi)->sl = 0;                                                        \
        (bi)->dbl = 0;                                                       \
        (bi)->ai = 0;                                                        \
        (bi)->skip_n = 0;                                                    \
        rs_memzero((bi)->sql, RS_SQL_MAX_LEN + 1);                           \
        rs_memzero((bi)->db, RS_DATABASE_NAME_MAX_LEN + 1);                  \
        rs_memzero((bi)->tb, RS_TABLE_NAME_MAX_LEN + 1);                     \
        (bi)->data = NULL;                                                   \
        (bi)->t = 0;                                                         \
        (bi)->intvar_t = 0;                                                  \
        (bi)->mev = 0;                                                       \
        (bi)->flush = 0;                                                     \
        (bi)->tran = 0;                                                      \
        (bi)->sent = 0;                                                      \
        (bi)->auto_incr = 0

/* binlog event type */
#define RS_BINLOG_MAGIC_NUM_LEN        4
#define RS_QUERY_EVENT                 2
#define RS_STOP_EVENT                  3
#define RS_ROTATE_EVENT                4
#define RS_INTVAR_EVENT                5
#define RS_XID_EVENT                   16
#define RS_FORMAT_DESCRIPTION_EVENT    15


/* binlog event header field */
#define RS_BINLOG_TIMESTAMP_LEN        4
#define RS_BINLOG_TYPE_CODE_LEN        1   
#define RS_BINLOG_SERVER_ID_LEN        4
#define RS_BINLOG_EVENT_LENGTH_LEN     4
#define RS_BINLOG_NEXT_POSITION_LEN    4
#define RS_BINLOG_FLAGS_LEN            2
#define RS_BINLOG_EVENT_HEADER_LEN     19

/* binlog query event */
#define RS_BINLOG_QUERY_THREAD_ID_LEN  4
#define RS_BINLOG_QUERY_EXEC_SEC_LEN   4
#define RS_BINLOG_QUERY_DB_NAME_LEN    1
#define RS_BINLOG_QUERY_ERR_CODE_LEN   2
#define RS_BINLOG_QUERY_STAT_BLOCK_LEN 2
#define RS_BINLOG_TABLE_NAME_LEN       1
#define RS_BINLOG_QUERY_FIXED_DATA_LEN 13

/* intvar*/
#define RS_BINLOG_INTVAR_TYPE_LEN      1
#define RS_BINLOG_INTVAR_TYPE_INCR     2
#define RS_BINLOG_INTVAR_INSERT_ID_LEN 8
#define RS_BINLOG_INTVAR_EVENT_LEN     9

/* table_map */
#define RS_BINLOG_TABLE_MAP_TABLE_ID_LEN  6
#define RS_BINLOG_TABLE_MAP_RESERVED_LEN   2

int rs_read_binlog(rs_request_dump_t *rd);
int rs_has_next_binlog(rs_request_dump_t *rd);
int rs_eof_read_binlog(rs_request_dump_t *rd, void *buf, size_t size);

#endif
