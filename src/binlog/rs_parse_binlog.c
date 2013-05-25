
#include <rs_config.h>
#include <rs_core.h>
#include <rs_binlog.h>

static int rs_binlog_header_handler(rs_binlog_info_t *bi);
static int rs_binlog_skip_handler(rs_binlog_info_t *bi);
static int rs_binlog_stop_handler(rs_binlog_info_t *bi);
static int rs_binlog_finish_handler(rs_binlog_info_t *bi);

static int rs_binlog_query_event_handler(rs_binlog_info_t *bi);
static int rs_binlog_intvar_event_handler(rs_binlog_info_t *bi);
static int rs_binlog_xid_event_handler(rs_binlog_info_t *bi);
static int rs_binlog_table_map_event_handler(rs_binlog_info_t *bi);
static int rs_binlog_write_rows_event_handler(rs_binlog_info_t *bi);
static int rs_binlog_update_rows_event_handler(rs_binlog_info_t *bi);
static int rs_binlog_delete_rows_event_handler(rs_binlog_info_t *bi);

rs_binlog_parse_handler rs_binlog_parse_handlers[] = {
    NULL,
    rs_binlog_skip_handler,
    rs_binlog_query_handler,
    rs_binlog_stop_handler,
    rs_binlog_stop_handler,
    rs_binlog_intvar_handler,
    rs_binlog_skip_handler,
    rs_binlog_skip_handler,
    rs_binlog_skip_handler,
    rs_binlog_skip_handler,
    rs_binlog_skip_handler,
    rs_binlog_skip_handler,
    rs_binlog_skip_handler,
    rs_binlog_skip_handler,
    rs_binlog_skip_handler,
    rs_binlog_skip_handler,
    rs_binlog_xid_handler,
    rs_binlog_skip_handler,
    rs_binlog_skip_handler,
    rs_binlog_table_map_handler,
    rs_binlog_skip_handler,
    rs_binlog_skip_handler,
    rs_binlog_skip_handler,
    rs_binlog_write_rows_handler,
    rs_binlog_update_rows_handler,
    rs_binlog_delete_rows_handler,
    rs_binlog_skip_handler,
    rs_binlog_skip_handler     
};

/*
    Binlog event header parse handler
*/
int rs_binlog_header_handler(rs_binlog_info_t *bi) 
{
    int     r;
    char    eh[RS_BINLOG_EVENT_HEADER_LEN], *p;

    p = eh;
    /* get event header */ 
    if((r = rs_read_binlog(bi, eh, RS_BINLOG_EVENT_HEADER_LEN)) != RS_OK) {
        return r;
    }

    /* skip timestamp */
    p += RS_BINLOG_HEADER_TIMESTAMP_LEN;
    /* get event type */
    bi->type = *p;
    if(bi->type == 0 || bi->type > RS_BINLOG_EVENT_NUM) {
        rs_log_error(RS_LOG_ERR, 0, "unknown binlog event, %u", bi->type);
        return RS_ERR;
    }

    /* skip type code */
    p += RS_BINLOG_HEADER_TYPE_CODE_LEN;
    /* get server_id */
    rs_memcpy(&(bi->server_id), p, RS_BINLOG_HEADER_SERVER_ID_LEN);
    p += RS_BINLOG_HEADER_SERVER_ID_LEN;
    /* get event_length */
    rs_memcpy(&(bi->event_len), p, RS_BINLOG_HEADER_EVENT_LENGTH_LEN);
    p += RS_BINLOG_HEADER_EVENT_LENGTH_LEN;
    /* get next position */
    rs_memcpy(&(bi->next_pos), p, RS_BINLOG_HEADER_NEXT_POSITION_LEN);
    // TODO
    //rd->dump_pos += bi->el;
    // rd->dump_pos = bi->np;
    //bi->np = rd->dump_pos;
    bi->data_len = bi->event_len - RS_BINLOG_EVENT_HEADER_LEN;
    rs_log_debug(RS_DEBUG_BINLOG, 0, 
            "\n========== %s header =============\n"
            "server id              : %u\n"
            "event type             : %u\n"
            "event length           : %u\n"
            "event next position    : %u\n"
            //"dump position          : %u\n"
            "data length            : %u\n",
            rs_binlog_event_name[idx],
            bi->server_id,
            bi->type,
            bi->event_len,
            bi->next_pos,
            //rd->dump_pos,
            bi->data_len);

    return RS_OK;
}

int rs_binlog_query_handler(rs_binlog_info_t *bi) 
{
    int         r, id;
    uint32_t    sbl, sqll;
    char        *p;

    p = NULL
    r = RS_OK;

    id = rs_palloc_id(bi->pool, bi->data_len);
    p = rs_palloc(bi->pool, bi->data_len, id);
    if(p == NULL) {
        return RS_ERR;
    }

    /* get query event fixed data */ 
    if((r = rs_read_binlog(bi, p, bi->data_len)) != RS_OK) {
        goto free;
    }

    /* seek after flags and unwant fixed data */
    p += RS_BINLOG_QUERY_THREAD_ID_LEN + RS_BINLOG_QUERY_EXEC_SEC_LEN;

    /* get database name len */
    rs_memcpy(&(bi->db_name_len), p, RS_BINLOG_QUERY_DB_NAME_LEN);
    bi->db_name_len++;
    p += RS_BINLOG_QUERY_DB_NAME_LEN + RS_BINLOG_QUERY_ERR_CODE_LEN;

    /* get status block len */
    rs_memcpy(&sbl, p, RS_BINLOG_QUERY_STAT_BLOCK_LEN);
    p += RS_BINLOG_QUERY_STAT_BLOCK_LEN + sbl;

    /* get database name */
    rs_memcpy(bi->db, p, bi->db_name_len);
    p += bi->db_name_len;

    /* filter care about list */
    sqll = bi->event_len - RS_BINLOG_EVENT_HEADER_LEN - sbl - 
        RS_BINLOG_QUERY_FIXED_DATA_LEN - bi->db_name_len; 

    rs_log_debug(RS_DEBUG_BINLOG, 0, 
            "\ndatabase name          : %s\n"
            "query sql              : %*.*s\n"
            bi->db_name,
            sqll,
            sqll,
            p
            );

    if(rs_strncmp(p, RS_BINLOG_START_TRAN, sqll) == 0) {
        bi->tran = 1;
    } else if(rs_strncmp(p, RS_BINLOG_COMMIT_TRAN, sqll) == 0) {
        bi->tran = 0;
    }

    bi->log_format = RS_BINLOG_FORMAT_SQL_STATEMENT;

free:
    if(p != NULL) {
        rs_pfree(bi->pool, p, id);    
    }
    return r;
}

int rs_binlog_intvar_handler(rs_binlog_info_t *bi) 
{
    int64_t     auto_incr;
    int         r;
    char        intvar[bi->data_len], *p;

    p = intvar;

    /* get intvar event */
    if((r = rs_read_binlog(bi, intvar, bi->data_len)) != RS_OK) {
        return r;
    }

    /* get intvar type */
    if(*p != RS_BINLOG_INTVAR_TYPE_INCR) { 
        return RS_OK;
    }

    p += RS_BINLOG_INTVAR_TYPE_LEN;
    /* get auto_increment id */
    rs_memcpy(&auto_incr, p, RS_BINLOG_INTVAR_INSERT_ID_LEN);

#if x86_64
    rs_log_debug(RS_DEBUG_BINLOG, 0, "\nincrement id        : %ld", auto_incr);
#elif x86_32
    rs_log_debug(RS_DEBUG_BINLOG, 0, "\nincrement id        : %lld", auto_incr);
#endif

    return RS_OK;
}

int rs_binlog_xid_handler(rs_binlog_info_t *bi) 
{
    int     r;
    int64_t tran_id;

    bi->tran = 0;
    if((r = rs_read_binlog(bi, &tran_id, bi->data_len)) != RS_OK) {
        return r;
    }

    rs_log_debug(RS_DEBUG_BINLOG, 0, 
            "tran id                : %lu",
            tran_id);

    return RS_OK;
}

int rs_binlog_table_map_handler(rs_binlog_info_t *bi)
{
    int         r;
    uint32_t    len;
    char        tm[bi->data_len], *p;

    p = tm;

    if((r = rs_read_binlog(bi, tm, bi->data_len)) != RS_OK) {
        return r;
    }

    /* seek after table id and reserved */
    p += RS_BINLOG_TABLE_MAP_TABLE_ID_LEN + RS_BINLOG_TABLE_MAP_RESERVED_LEN;
    /* get database name len */
    rs_memcpy(&(bi->db_name_len), p, RS_BINLOG_TABLE_MAP_DB_NAME_LEN);
    bi->db_name_len++;
    p += RS_BINLOG_TABLE_MAP_DB_NAME_LEN;
    /* get database name */
    rs_memcpy(bi->db_name, p, bi->db_name_len);
    p += bi->db_name_len;
    /* get table name len */
    rs_memcpy(&(bi->tb_name_len), p, RS_BINLOG_TABLE_MAP_TB_NAME_LEN);
    bi->tb_name_len++;
    p += RS_BINLOG_TABLE_MAP_TB_NAME_LEN;
    /* get table name */
    rs_memcpy(bi->tb_name, p, bi->tb_name_len);
    p += bi->tb_name_len;
    /* get column number */
    bi->col_num = (uint32_t) rs_parse_packed_integer(p, &len);
    p += len;
    /* get column type */
    rs_memcpy(bi->col_type, p, bi->col_num);
    p += bi->col_num;
    /* get column meta length */
    bi->meta_len = (uint32_t) rs_parse_packed_integer(p, &len);
    p += len;
    /* get column meta */
    rs_memcpy(bi->col_meta, p, bi->meta_len);
    p += bi->meta_len;
    
    char dt[bi->db_name_len + bi->tb_name_len + 2];

    /* filter db and tables */
    if(snprintf(dt, bi->db_name_len + bi->tb_name_len + 2, ",%s.%s,", 
                bi->db_name, bi->tb_name) < 0)
    {
        rs_log_error(RS_LOG_ERR, rs_errno, "snprintf() failed");
        return RS_ERR;
    }

    bi->filter = (rs_strstr(bi->filter_tables, dt) == NULL);
    rs_log_debug(RS_DEBUG_BINLOG, 0, 
            "\ndatabase                   : %s\n"
            "table                      : %s\n"
            "column num                 : %u\n"
            "skip                       : %d",
            bi->db_name,
            bi->tb_name,
            bi->col_num,
            bi->filter);

    return RS_OK;
}

int rs_binlog_write_rows_handler(rs_binlog_info_t *bi)
{
    int                     r;
    char                    wr[bi->binlog_info.dl];

    if(bi->filter) {
        bi->skip = 1;
        return RS_OK;
    }

    if((r = rs_read_binlog(bi, wr, bi->data_len)) != RS_OK) {
        return r;
    }

    bi->data = wr;
    bi->log_format = RS_BINLOG_FORMAT_ROW_BASED;
    bi->mev = RS_WRITE_ROWS_EVENT;

    if((r = rs_binlog_create_data(bi)) != RS_OK) {
        return r;
    }

    return RS_OK;
}

int rs_binlog_update_rows_handler(rs_binlog_info_t *bi)
{
    int                     r;
    char                    wr[bi->binlog_info.dl];

    if(bi->filter) {
        bi->skip = 1;
        return RS_OK;
    }

    if((r = rs_eof_read_binlog2(bi, wr, bi->dl)) != RS_OK) {
        return r;
    }

    bi->data = wr;
    bi->log_format = RS_BINLOG_FORMAT_ROW_BASED;
    bi->mev = RS_UPDATE_ROWS_EVENT;

    if((r = rs_binlog_create_data(bi)) != RS_OK) {
        return r;
    }

    return RS_OK;
}

int rs_binlog_delete_rows_handler(rs_binlog_info_t *bi)
{
    int     r;
    char    wr[bi->binlog_info.data_len];

    if(bi->filter) {
        bi->skip = 1;
        return RS_OK;
    }

    if((r = rs_eof_read_binlog2(bi, wr, bi->dl)) != RS_OK) {
        return r;
    }

    bi->data = wr;
    bi->log_format = RS_BINLOG_FORMAT_ROW_BASED;
    bi->mev = RS_DELETE_ROWS_EVENT;

    if((r = rs_binlog_create_data(bi)) != RS_OK) {
        return r;
    }

    return RS_OK;
}

int rs_binlog_finish_handler(rs_binlog_info_t *bi) 
{
    if((!bi->sent && !bi->filter) || bi->flush) {
        rs_log_debug(RS_DEBUG_BINLOG, 0, "send dump file = %s, "
        "send dump pos = %u, tran = %d, sent = %d, flush = %d", rd->dump_file, 
                rd->dump_pos, bi->tran, bi->sent, bi->flush);

        /* add to ring buffer */
        if(rs_binlog_create_data(rd) != RS_OK) {
            return RS_ERR;
        }
    }

    bi->sent = 0;
    if(bi->skip) {
        if((uint32_t) (rd->io_buf->last - rd->io_buf->pos) >= bi->dl) {
            rd->io_buf->pos += bi->dl;
        } else {
            rd->io_buf->pos = rd->io_buf->last; 

            if(fseek(rd->binlog_fp, rd->dump_pos, SEEK_SET) == -1) {
                rs_log_error(RS_LOG_ERR, rs_errno, "fseek() failed, "
                        "seek_set pos = %u", rd->dump_pos);
                return RS_ERR;
            }
        }

        bi->skip = 0;
    }

    return RS_OK;
}

int rs_binlog_skip_handler(rs_binlog_info_t *bi)
{
    bi->skip = 1; 
    return RS_OK;
}

int rs_binlog_stop_handler(rs_binlog_info_t *bi)
{
    int err;

    if(bi->conf_server_id != bi->server_id) {
        rs_log_error(RS_LOG_ERR, 0, "server id is not match, skip rotate event" 
                "or stop event");
        bi->skip= 1;
        return RS_OK;
    }

    for( ;; ) {
        if((err = rs_has_next_binlog(bi)) == RS_OK) {
            return RS_HAS_BINLOG;
        } else if(err == RS_ERR) {
            return RS_ERR;
        } else if(err == RS_NO_BINLOG) {
            sleep(RS_BINLOG_EOF_WAIT_SEC);
            //TODO inotify
            // set inotify
            // continue;
            // wait binlog index file changed
        }
    }

    return RS_OK;
}
