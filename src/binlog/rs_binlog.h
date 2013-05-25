
#ifndef _RS_BINLOG_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>

#define RS_BINLOG_DB_NAME_MAX_LEN           64
#define RS_BINLOG_TB_NAME_MAX_LEN           64
#define RS_BINLOG_COL_TYPE_MAX_LEN          4096
#define RS_BINLOG_COL_META_MAX_LEN          (4096 * 2)

// Binlog format
#define RS_BINLOG_FORMAT_SQL_STATEMENT      0
#define RS_BINLOG_FORMAT_ROW_BASED          1

// Binlog tran keyword
#define RS_BINLOG_START_TRAN                "BEGIN"
#define RS_BINLOG_COMMIT_TRAN               "COMMIT"

#define RS_BINLOG_EOF_WAIT_SEC      1

struct rs_binlog_info_s {

    uint32_t    next_pos;
    uint32_t    event_len;     /* event length */
    uint32_t    tb_name_len;    /* table length */
    uint32_t    db_name_len;    /* database length */
    uint32_t    col_num;     /* column number */
    uint32_t    meta_len;     /* meta length */
    uint32_t    data_len;     /* datalength */ 
    uint32_t    server_id;    /* server-id */
    uint32_t    evnet_type;

    uint32_t    skip_num; /* skip send event number */

    char        db_name[RS_BINLOG_DB_NAME_MAX_LEN + 1];
    char        tb_name[RS_BINLOG_TB_NAME_MAX_LEN + 1];

    char        col_type[RS_BINLOG_COL_TYPE_MAX_LEN];
    char        col_meta[RS_BINLOG_COL_META_MAX_LEN];

    void        *data;

    // TODO
    char        send_type; // Mission Event type

    int         fd;
    rs_but_t    *io_buf;
    rs_pool_t   *pool;

    unsigned    flush           :1;
    unsigned    sent            :1;
    unsigned    tran            :1;
    unsigned    log_format      :1;
    unsigned    filter          :1;
    unsigned    skip            :1;

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


extern char *rs_binlog_event_name[] = {
    NULL,
    "START_EVENT_V3",
    "QUERY_EVENT",
    "STOP_EVENT",
    "ROTATE_EVENT",
    "INTVAR_EVNET",
    "LOAD_EVENT",
    "SLAVE_EVENT",
    "CREATE_FILE_EVENT",
    "APPEND_BLOCK_EVENT",
    "EXEC_LOAD_EVENT",
    "DELETE_FILE_EVENT",
    "NEW_LOAD_EVENT",
    "RAND_EVENT",
    "USER_VAR_EVENT",
    "FORMAT_DESCRIPTION_EVENT",
    "XID_EVENT",
    "BEGIN_LOAD_QUERY_EVENT",
    "EXECUTE_LOAD_QUERY_EVENT",
    "TABLE_MAP_EVENT",
    "PRE_GA_WRITE_ROWS_EVENT",
    "PRE_GA_UPDATE_ROWS_EVENT",
    "PRE_GA_DELETE_ROWS_EVENT",
    "WRITE_ROWS_EVENT",
    "UPDATE_ROWS_EVENT",
    "DELETE_ROWS_EVENT",
    "INCIDENT_EVENT",
    "HEARTBEAT_LOG_EVENT"
}

// Binlog event type
#define RS_BINLOG_START_EVENT_V3               1
#define RS_BINLOG_QUERY_EVENT                  2
#define RS_BINLOG_STOP_EVENT                   3
#define RS_BINLOG_ROTATE_EVENT                 4
#define RS_BINLOG_INTVAR_EVENT                 5
#define RS_BINLOG_LOAD_EVENT                   6
#define RS_BINLOG_SLAVE_EVENT                  7
#define RS_BINLOG_CREATE_FILE_EVENT            8
#define RS_BINLOG_APPEND_BLOCK_EVENT           9
#define RS_BINLOG_EXEC_LOAD_EVENT              10
#define RS_BINLOG_DELETE_FILE_EVENT            11
#define RS_BINLOG_NEW_LOAD_EVENT               12
#define RS_BINLOG_RAND_EVENT                   13
#define RS_BINLOG_USER_VAR_EVENT               14
#define RS_BINLOG_FORMAT_DESCRIPTION_EVENT     15
#define RS_BINLOG_XID_EVENT                    16
#define RS_BINLOG_BEGIN_LOAD_QUERY_EVENT       17
#define RS_BINLOG_EXECUTE_LOAD_QUERY_EVENT     18
#define RS_BINLOG_TABLE_MAP_EVENT              19
#define RS_BINLOG_PRE_GA_WRITE_ROWS_EVENT      20
#define RS_BINLOG_PRE_GA_UPDATE_ROWS_EVENT     21
#define RS_BINLOG_PRE_GA_DELETE_ROWS_EVENT     22 
#define RS_BINLOG_WRITE_ROWS_EVENT             23
#define RS_BINLOG_UPDATE_ROWS_EVENT            24
#define RS_BINLOG_DELETE_ROWS_EVENT            25
#define RS_BINLOG_INCIDENT_EVENT               26
#define RS_BINLOG_HEARTBEAT_LOG_EVENT          27

#define RS_BINLOG_EVENT_NUM             27
#define RS_BINLOG_MAGIC_NUM_LEN         4

/* binlog event header field */
#define RS_BINLOG_HEADER_TIMESTAMP_LEN        4
#define RS_BINLOG_HEADER_TYPE_CODE_LEN        1   
#define RS_BINLOG_HEADER_SERVER_ID_LEN        4
#define RS_BINLOG_HEADER_EVENT_LENGTH_LEN     4
#define RS_BINLOG_HEADER_NEXT_POSITION_LEN    4
#define RS_BINLOG_HEADER_FLAGS_LEN            2
#define RS_BINLOG_HEADER_EVENT_HEADER_LEN     19

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

#endif
