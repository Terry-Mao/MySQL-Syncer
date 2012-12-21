
#ifndef _RS_READ_BINLOG_H_INCLUDED_
#define _RS_READ_BINLOG_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

#define RS_DATABASE_NAME_MAX_LEN       64
#define RS_TABLE_NAME_MAX_LEN          64
#define RS_SQL_MAX_LEN                 1024
#define RS_COLUMN_MAX_LEN              4096
#define RS_COLUMN_META_MAX_LEN         (4096 * 2)

#define RS_BINLOG_FORMAT_SQL_STATEMENT 0
#define RS_BINLOG_FORMAT_ROW_BASED     1

#define RS_TRAN_KEYWORD                "BEGIN"
#define RS_TRAN_KEYWORD_LEN            sizeof("BEGIN") - 1

#define RS_TRAN_END_KEYWORD            "COMMIT"
#define RS_TRAN_END_KEYWORD_LEN        sizeof("COMMIT") - 1

#define RS_BINLOG_EOF_WAIT_SEC      1


struct rs_binlog_info_s {

    uint32_t    np;     /* next position */
    uint32_t    el;     /* event length */
    uint32_t    sbl;    /* status block length */
    uint32_t    tbl;    /* table length */
    uint32_t    sl;     /* sql length */
    uint32_t    dbl;    /* database length */
    uint64_t    ai;     /* auto increment */
    uint32_t    cn;     /* column number */
    uint32_t    ml;     /* meta length */
    uint32_t    dl;     /* data-part length */
    uint32_t    svrid;    /* server-id */

    uint64_t    tranid; /* transaction id */

    uint32_t    skip_n; /* skip flush number */

    /* char        eh[RS_BINLOG_EVENT_HEADER_LEN]; */
    char        sql[RS_SQL_MAX_LEN + 1];
    char        db[RS_DATABASE_NAME_MAX_LEN + 1];
    char        tb[RS_TABLE_NAME_MAX_LEN + 1];

    char        ct[RS_COLUMN_MAX_LEN];
    char        cm[RS_COLUMN_META_MAX_LEN];

    void        *data;

    char        t;
    char        intvar_t;

    char        mev; // Mission Event type

    unsigned    flush      :1;
    unsigned    sent       :1;
    unsigned    tran       :1;
    unsigned    auto_incr  :1;
    unsigned    log_format :1;
    unsigned    filter     :1;
    unsigned    skip       :1;

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
        (bi)->cn = 0;                                                        \
        (bi)->ml = 0;                                                        \
        (bi)->dl = 0;                                                        \
        (bi)->svrid = 0;                                                     \
        (bi)->tranid = 0;                                                    \
        rs_memzero((bi)->sql, RS_SQL_MAX_LEN + 1);                           \
        rs_memzero((bi)->db, RS_DATABASE_NAME_MAX_LEN + 1);                  \
        rs_memzero((bi)->tb, RS_TABLE_NAME_MAX_LEN + 1);                     \
        rs_memzero((bi)->ct, RS_COLUMN_MAX_LEN);                             \
        rs_memzero((bi)->cm, RS_COLUMN_META_MAX_LEN);                        \
        (bi)->data = NULL;                                                   \
        (bi)->t = 0;                                                         \
        (bi)->intvar_t = 0;                                                  \
        (bi)->mev = 0;                                                       \
        (bi)->flush = 0;                                                     \
        (bi)->tran = 0;                                                      \
        (bi)->sent = 0;                                                      \
        (bi)->filter = 1;                                                    \
        (bi)->skip = 0;                                                      \
        (bi)->auto_incr = 0

#define RS_BINLOG_MAGIC_NUM_LEN         4

#define RS_BINLOG_EVENT_NUM             27

/* binlog event type */
#define RS_START_EVENT_V3               1
#define RS_QUERY_EVENT                  2
#define RS_STOP_EVENT                   3
#define RS_ROTATE_EVENT                 4
#define RS_INTVAR_EVENT                 5
#define RS_LOAD_EVENT                   6
#define RS_SLAVE_EVENT                  7
#define RS_CREATE_FILE_EVENT            8
#define RS_APPEND_BLOCK_EVENT           9
#define RS_EXEC_LOAD_EVENT              10
#define RS_DELETE_FILE_EVENT            11
#define RS_NEW_LOAD_EVENT               12
#define RS_RAND_EVENT                   13
#define RS_USER_VAR_EVENT               14
#define RS_FORMAT_DESCRIPTION_EVENT     15
#define RS_XID_EVENT                    16
#define RS_BEGIN_LOAD_QUERY_EVENT       17
#define RS_EXECUTE_LOAD_QUERY_EVENT     18
#define RS_TABLE_MAP_EVENT              19
#define RS_PRE_GA_WRITE_ROWS_EVENT      20
#define RS_PRE_GA_UPDATE_ROWS_EVENT     21
#define RS_PRE_GA_DELETE_ROWS_EVENT     22 
#define RS_WRITE_ROWS_EVENT             23
#define RS_UPDATE_ROWS_EVENT            24
#define RS_DELETE_ROWS_EVENT            25
#define RS_INCIDENT_EVENT               26
#define RS_HEARTBEAT_LOG_EVENT          27


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
#define RS_BINLOG_TABLE_MAP_RESERVED_LEN  2
#define RS_BINLOG_TABLE_MAP_DB_NAME_LEN   1
#define RS_BINLOG_TABLE_MAP_TB_NAME_LEN   1

/* write_rows */
#define RS_BINLOG_WRITE_ROWS_TABLE_ID_LEN 6
#define RS_BINLOG_WRITE_ROWS_RESERVED_LEN 2

int rs_read_binlog(rs_reqdump_data_t *d);
int rs_has_next_binlog(rs_reqdump_data_t *d);
int rs_eof_read_binlog2(rs_reqdump_data_t *d, void *buf, size_t size);

#endif
