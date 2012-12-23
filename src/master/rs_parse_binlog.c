
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

rs_binlog_func rs_binlog_funcs[] = {
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

int rs_binlog_header_handler(rs_reqdump_data_t *rd) 
{
    int                 r;
    rs_binlog_info_t    *bi;
    char                eh[RS_BINLOG_EVENT_HEADER_LEN], *p;

    bi = &(rd->binlog_info);
    p = eh;

    /* get event header */ 
    if((r = rs_eof_read_binlog2(rd, eh, RS_BINLOG_EVENT_HEADER_LEN)) != RS_OK) 
    {
        return r;
    }

    /* skip timestamp */
    p += RS_BINLOG_TIMESTAMP_LEN;

    /* get event type */
    bi->t = *p;

    /* skip type code */
    p += RS_BINLOG_TYPE_CODE_LEN;

    /* get server_id */
    rs_memcpy(&(bi->svrid), p, RS_BINLOG_SERVER_ID_LEN);
    p += RS_BINLOG_SERVER_ID_LEN;

    /* get event_length */
    rs_memcpy(&(bi->el), p, RS_BINLOG_EVENT_LENGTH_LEN);
    p += RS_BINLOG_EVENT_LENGTH_LEN;

    rd->dump_pos += bi->el;

    /* get next position */
    rs_memcpy(&(bi->np), p, RS_BINLOG_NEXT_POSITION_LEN);

    // rd->dump_pos = bi->np;
    bi->np = rd->dump_pos;

    bi->dl = bi->el - RS_BINLOG_EVENT_HEADER_LEN;

    rs_log_debug(0, 
            "\n========== event header =============\n"
            "server id              : %u\n"
            "event type             : %d\n"
            "event length           : %u\n"
            "event next position    : %u\n"
            "dump position          : %u\n"
            "data length            : %u\n",
            bi->svrid,
            bi->t,
            bi->el,
            bi->np,
            rd->dump_pos,
            bi->dl);

    return RS_OK;
}

int rs_binlog_query_handler(rs_reqdump_data_t *rd) 
{
    int                 r;
    rs_binlog_info_t    *bi;
    char                query[rd->binlog_info.dl], *p;

    bi = &(rd->binlog_info);
    p = query;

    if(bi->dl > RS_SQL_MAX_LEN * 2) {
        bi->skip = 1;
        return RS_OK;
    }

    /* get query event fixed data */ 
    if((r = rs_eof_read_binlog2(rd, query, bi->dl)) != RS_OK) 
    {
        return r;
    }

    /* seek after flags and unwant fixed data */
    p += RS_BINLOG_QUERY_THREAD_ID_LEN + RS_BINLOG_QUERY_EXEC_SEC_LEN;

    /* get database name len */
    rs_memcpy(&(bi->dbl), p, RS_BINLOG_QUERY_DB_NAME_LEN);
    bi->dbl = (bi->dbl & 0x000000FF) + 1;
    p += RS_BINLOG_QUERY_DB_NAME_LEN + RS_BINLOG_QUERY_ERR_CODE_LEN;

    /* get status block len */
    rs_memcpy(&(bi->sbl), p, RS_BINLOG_QUERY_STAT_BLOCK_LEN);
    bi->sbl = bi->sbl & 0x0000FFFF; 
    p += RS_BINLOG_QUERY_STAT_BLOCK_LEN + bi->sbl;

    /* get database name */
    rs_memcpy(bi->db, p, bi->dbl);
    p += bi->dbl;

    /* filter care about list */
    bi->sl = bi->el - RS_BINLOG_EVENT_HEADER_LEN - bi->sbl - 
        RS_BINLOG_QUERY_FIXED_DATA_LEN - bi->dbl; 

    bi->sl = rs_min(RS_SQL_MAX_LEN, bi->sl);

    rs_memcpy(bi->sql, p, bi->sl);

    bi->sql[bi->sl] = '\0';
    bi->log_format = RS_BINLOG_FORMAT_SQL_STATEMENT;

    rs_log_debug(0, 
            "\ndatabase name          : %s\n"
            "query sql              : %s\n"
            "next position          : %u\n",
            bi->db,
            bi->sql,
            bi->np
            );

    if(rs_strncmp(bi->sql, RS_TRAN_KEYWORD, RS_TRAN_KEYWORD_LEN) == 0) {
        bi->tran = 1;
    } else if(rs_strncmp(bi->sql, RS_TRAN_END_KEYWORD, 
                RS_TRAN_END_KEYWORD_LEN) == 0) {
        bi->tran = 0;
    } else {
        // TODO
        /*
        if(rs_binlog_filter_data(rd) != RS_OK) {
            return RS_ERR;
        }
        */
    }

    return RS_OK;
}


int rs_binlog_intvar_handler(rs_reqdump_data_t *rd) 
{
    int                 r;
    rs_binlog_info_t    *bi;
    char                intvar[rd->binlog_info.dl], *p;

    bi = &(rd->binlog_info);
    p = intvar;

    /* get intvar event */
    if((r = rs_eof_read_binlog2(rd, intvar, bi->dl)) 
            != RS_OK) 
    {
        return r;
    }

    /* get intvar type */
    bi->intvar_t = *p;
    p += RS_BINLOG_INTVAR_TYPE_LEN;

    if(bi->intvar_t != RS_BINLOG_INTVAR_TYPE_INCR) { 
        return RS_OK;
    }

    /* get auto_increment id */
    rs_memcpy(&(bi->ai), p, RS_BINLOG_INTVAR_INSERT_ID_LEN);

#if x86_64
    rs_log_debug(0, "\nincrement id           : %lu", bi->ai);
#elif x86_32
    rs_log_debug(0, "\nincrement id           : %llu", bi->ai);

#endif

    bi->auto_incr = 1;

    return RS_OK;
}

int rs_binlog_xid_handler(rs_reqdump_data_t *rd) 
{
    int                     r;
    rs_binlog_info_t        *bi;

    bi = &(rd->binlog_info);

    bi->tran = 0;

    if((r = rs_eof_read_binlog2(rd, &(bi->tranid), bi->dl)) != RS_OK) {
        return r;
    }

    rs_log_debug(0, 
            "\ntran                   : %d\n"
            "tran id                : %lu",
            bi->tran,
            bi->tranid);

    return RS_OK;
}

int rs_binlog_table_map_handler(rs_reqdump_data_t *rd)
{
    int                     r;
    uint32_t                len;
    rs_binlog_info_t        *bi;
    char                    tm[rd->binlog_info.dl], *p,
                            dt[RS_DATABASE_NAME_MAX_LEN + 
                                RS_TABLE_NAME_MAX_LEN + 3];

    bi = &(rd->binlog_info);
    p = tm;

    if((r = rs_eof_read_binlog2(rd, tm, bi->dl)) != RS_OK) {
        return r;
    }

    /* seek after table id and reserved */
    p += RS_BINLOG_TABLE_MAP_TABLE_ID_LEN + RS_BINLOG_TABLE_MAP_RESERVED_LEN;

    /* get database name len */
    rs_memcpy(&(bi->dbl), p, RS_BINLOG_TABLE_MAP_DB_NAME_LEN);
    bi->dbl = (bi->dbl & 0x000000FF) + 1;
    p += RS_BINLOG_TABLE_MAP_DB_NAME_LEN;

    /* get database name */
    rs_memcpy(bi->db, p, bi->dbl);
    p += bi->dbl;

    /* get table name len */
    rs_memcpy(&(bi->tbl), p, RS_BINLOG_TABLE_MAP_TB_NAME_LEN);
    bi->tbl = (bi->tbl & 0x000000FF) + 1;
    p += RS_BINLOG_TABLE_MAP_TB_NAME_LEN;

    /* get table name */
    rs_memcpy(bi->tb, p, bi->tbl);
    p += bi->tbl;

    /* get column number */
    bi->cn = (uint32_t) rs_parse_packed_integer(p, &len);
    p += len;

    /* get column type */
    rs_memcpy(bi->ct, p, bi->cn);
    p += bi->cn;

    /* get column meta length */
    bi->ml = (uint32_t) rs_parse_packed_integer(p, &len);
    p += len;

    /* get column meta */
    rs_memcpy(bi->cm, p, bi->ml);
    p += bi->ml;

    /* filter db and tables */
    if(rd->filter_tables != NULL) {

        if(snprintf(dt, RS_DATABASE_NAME_MAX_LEN + RS_TABLE_NAME_MAX_LEN + 3,
                    ",%s.%s,", bi->db, bi->tb) < 0)
        {
            rs_log_err(rs_errno, "snprintf() failed, ,db.tb,");
            return RS_ERR;
        }

        bi->filter = (rs_strstr(rd->filter_tables, dt) == NULL);
    }

    rs_log_debug(0, 
            "\ndatabase                   : %s\n"
            "table                      : %s\n"
            "column num                 : %u\n"
            "skip                       : %d",
            bi->db,
            bi->tb,
            bi->cn,
            bi->filter);

    return RS_OK;
}

int rs_binlog_write_rows_handler(rs_reqdump_data_t *rd)
{
    int                     r;
    rs_binlog_info_t        *bi;
    char                    wr[rd->binlog_info.dl];

    bi = &(rd->binlog_info);

    if(bi->filter) {

        if(!bi->tran) {
            bi->filter = 0;
        }

        bi->skip = 1;
        return RS_OK;
    }

    if((r = rs_eof_read_binlog2(rd, wr, bi->dl)) != RS_OK) {
        return r;
    }

    bi->data = wr;
    bi->log_format = RS_BINLOG_FORMAT_ROW_BASED;
    bi->mev = RS_WRITE_ROWS_EVENT;

    if((r = rs_binlog_create_data(rd)) != RS_OK) {
        return r;
    }

    return RS_OK;
}

int rs_binlog_update_rows_handler(rs_reqdump_data_t *rd)
{
    int                     r;
    rs_binlog_info_t        *bi;
    char                    wr[rd->binlog_info.dl];

    bi = &(rd->binlog_info);

    if(bi == NULL) {
        return RS_ERR;
    }

    if(bi->filter) {

        if(!bi->tran) {
            bi->filter = 0;
        }

        bi->skip = 1;
        return RS_OK;
    }

    if((r = rs_eof_read_binlog2(rd, wr, bi->dl)) != RS_OK) {
        return r;
    }

    bi->data = wr;
    bi->log_format = RS_BINLOG_FORMAT_ROW_BASED;
    bi->mev = RS_UPDATE_ROWS_EVENT;

    if((r = rs_binlog_create_data(rd)) != RS_OK) {
        return r;
    }

    return RS_OK;
}

int rs_binlog_delete_rows_handler(rs_reqdump_data_t *rd)
{
    int                     r;
    rs_binlog_info_t        *bi;
    char                    wr[rd->binlog_info.dl];

    bi = &(rd->binlog_info);

    if(bi == NULL) {
        return RS_ERR;
    }

    if(bi->filter) {

        if(!bi->tran) {
            bi->filter = 0;
        }

        bi->skip = 1;
        return RS_OK;
    }

    if((r = rs_eof_read_binlog2(rd, wr, bi->dl)) != RS_OK) {
        return r;
    }

    bi->data = wr;
    bi->log_format = RS_BINLOG_FORMAT_ROW_BASED;
    bi->mev = RS_DELETE_ROWS_EVENT;

    if((r = rs_binlog_create_data(rd)) != RS_OK) {
        return r;
    }

    return RS_OK;
}

int rs_binlog_finish_handler(rs_reqdump_data_t *rd) 
{
    rs_binlog_info_t        *bi;

    bi = &(rd->binlog_info);

    if((!bi->tran && !bi->sent && !bi->filter) || bi->flush) {

        rs_log_debug(0, "send dump file = %s, send dump pos = %u, tran = %d, "
                "sent = %d, flush = %d", rd->dump_file, 
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

            rs_log_debug(0, "fseek");
            if(fseek(rd->binlog_fp, rd->dump_pos, SEEK_SET) == -1) {
                rs_log_err(rs_errno, "fseek() failed, seek_set pos = %u", 
                        rd->dump_pos);
                return RS_ERR;
            }
        }

        bi->skip = 0;
    }

    return RS_OK;
}

int rs_binlog_skip_handler(rs_reqdump_data_t *rd)
{
    rd->binlog_info.skip = 1; 
    return RS_OK;
}

int rs_binlog_stop_handler(rs_reqdump_data_t *rd)
{
    int err;

    if(rd->server_id != rd->binlog_info.svrid) {
        rs_log_info("server id is not match, skip rotate event or "
                "stop event");
        rd->binlog_info.skip = 1;
        return RS_OK;
    }

    for( ;; ) {

        if((err = rs_has_next_binlog(rd)) == RS_OK) {
            return RS_HAS_BINLOG;
        } else if(err == RS_ERR) {
            return RS_ERR;
        } else if(err == RS_NO_BINLOG) {
            sleep(RS_BINLOG_EOF_WAIT_SEC);
        }
    }

    return RS_OK;
}
