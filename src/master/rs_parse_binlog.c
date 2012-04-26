
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

int rs_def_header_handle(rs_request_dump_t *rd) 
{
    int                 r;
    rs_binlog_info_t    *bi;
    char                eh[RS_BINLOG_EVENT_HEADER_LEN], *p;

    bi = &(rd->binlog_info);
    p = eh;

    /* skip magic num */
    if(rd->dump_pos == 0) {
        if(fseek(rd->binlog_fp, RS_BINLOG_MAGIC_NUM_LEN, SEEK_SET) == -1) {
            rs_log_err(rs_errno, "fseek() failed, seek_set pos = %u", p);
            return RS_ERR;
        }
    }

    /* get event header */ 
    if((r = rs_eof_read_binlog(rd, eh, RS_BINLOG_EVENT_HEADER_LEN)) != RS_OK) 
    {
        return r;
    }

    /* skip timestamp */
    p += RS_BINLOG_TIMESTAMP_LEN;

    /* get event type */
    bi->t = *p;

    /* skip server id */
    p += (RS_BINLOG_TYPE_CODE_LEN + RS_BINLOG_SERVER_ID_LEN);

    /* get event_length */
    rs_memcpy(&(bi->el), p, RS_BINLOG_EVENT_LENGTH_LEN);
    p += RS_BINLOG_EVENT_LENGTH_LEN;

    /* get next position */
    rs_memcpy(&(bi->np), p, RS_BINLOG_NEXT_POSITION_LEN);

    rd->dump_pos = bi->np;

    rs_log_debug(0, 
            "\n========== event header =============\n"
            "event type             : %d\n"
            "event length           : %u\n"
            "event next position    : %u\n"
            "dump position          : %u\n"
            "\n=====================================\n",
            bi->t,
            bi->el,
            bi->np,
            rd->dump_pos);

    return RS_OK;

}

int rs_def_query_handle(rs_request_dump_t *rd) 
{
    int                 r;
    rs_binlog_info_t    *bi;
    char                fixed[RS_BINLOG_QUERY_FIXED_DATA_LEN], *p;

    p = fixed;
    bi = &(rd->binlog_info);

    /* get query event fixed data */ 
    if((r = rs_eof_read_binlog(rd, fixed, RS_BINLOG_QUERY_FIXED_DATA_LEN)) 
            != RS_OK) 
    {
        return r;
    }

    /* seek after flags and unwant fixed data */
    p += (RS_BINLOG_QUERY_THREAD_ID_LEN + RS_BINLOG_QUERY_EXEC_SEC_LEN);

    /* get database name len */
    rs_memcpy(&(bi->dbl), p, RS_BINLOG_QUERY_DB_NAME_LEN);
    bi->dbl = (bi->dbl & 0x000000FF) + 1;
    p += (RS_BINLOG_QUERY_DB_NAME_LEN + RS_BINLOG_QUERY_ERR_CODE_LEN);

    /* get status block len */
    rs_memcpy(&(bi->sbl), p, RS_BINLOG_QUERY_STAT_BLOCK_LEN);
    bi->sbl = bi->sbl & 0x0000FFFF;

    /* seek after status block */
    if(fseek(rd->binlog_fp, bi->sbl, SEEK_CUR) == -1) {
        return RS_ERR;
    }

    /* get database name */
    if((r = rs_eof_read_binlog(rd, bi->db, bi->dbl)) != RS_OK) {
        return r;
    }

    /* filter care about list */
    bi->sl = bi->el - RS_BINLOG_EVENT_HEADER_LEN - bi->sbl - 
        RS_BINLOG_QUERY_FIXED_DATA_LEN - bi->dbl; 

    bi->sl = rs_min(RS_SQL_MAX_LEN, bi->sl);

    if((r = rs_eof_read_binlog(rd, bi->sql, bi->sl)) != RS_OK) {
        return r;
    }

    bi->sql[bi->sl] = '\0';

    if(rs_strncmp(bi->sql, RS_TRAN_KEYWORD, RS_TRAN_KEYWORD_LEN) == 0) {
        bi->tran = 1;
    } else if(rs_strncmp(bi->sql, RS_TRAN_END_KEYWORD, 
                RS_TRAN_END_KEYWORD_LEN) == 0) {
        bi->tran = 0;
    }

    rs_log_debug(0, 
            "\n========== query event ==============\n"
            "database name          : %s\n"
            "query sql              : %s\n"
            "next position          : %u\n"
            "tran                   : %d\n"
            "\n=====================================\n",
            bi->db,
            bi->sql,
            bi->np,
            bi->tran);


    if(rs_binlog_filter_data(rd) != RS_OK) {
        return RS_ERR;
    }

    return RS_OK;
}


int rs_def_intvar_handle(rs_request_dump_t *rd) 
{
    int                 r;
    rs_binlog_info_t    *bi;
    char                intvar[RS_BINLOG_INTVAR_EVENT_LEN], *p;

    p = intvar;
    bi = &(rd->binlog_info);

    /* get intvar event */
    if((r = rs_eof_read_binlog(rd, intvar, RS_BINLOG_INTVAR_EVENT_LEN)) 
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
    rs_log_debug(0, 
            "\n========== intvar event ==============\n"
            "increment id           : %lu"
            "\n=====================================\n",
            bi->ai);
#elif x86_32
    rs_log_debug(0, 
            "\n========== intvar event ==============\n"
            "increment id           : %llu"
            "\n=====================================\n",
            bi->ai);

#endif

    bi->auto_incr = 1;

    return RS_OK;
}

int rs_def_xid_handle(rs_request_dump_t *rd) 
{
    rs_binlog_info_t        *bi;

    bi = &(rd->binlog_info);

    bi->tran = 0;

    rs_log_debug(0, 
            "\n========== xid event ==============\n"
            "tran                   : %d"
            "\n=====================================\n",
            bi->tran);

    return RS_OK;
}

int rs_def_table_map_handle(rs_request_dump_t *rd)
{
#if 0
    int                     r;
    rs_binlog_info_t        *bi;

    bi = &(rd->binlog_info);

    /* seek after error code */
    if(fseek(rd->binlog_fp, RS_BINLOG_TABLE_MAP_TABLE_ID_LEN + 
                RS_BINLOG_TABLE_MAP_RESERVED_LEN, SEEK_CUR) == -1) 
    {
        return RS_ERR;
    }

    /* get database name len */
    if((r = rs_eof_read_binlog(rd, &(bi->dbl), RS_BINLOG_DATABASE_NAME_LEN)) 
            != RS_OK) 
    {
        return r;
    }

    bi->dbl = bi->dbl & 0x000000FF;
    bi->dbl++;

    /* get database name */
    if((r = rs_eof_read_binlog(rd, bi->db, bi->dbl)) != RS_OK) {
        return r;
    }

    /* get table name len */
    if((r = rs_eof_read_binlog(rd, &(bi->tbl), RS_BINLOG_TABLE_NAME_LEN)) != 
            RS_OK) 
    {
        return r;
    }

    bi->tbl = bi->tbl & 0x000000FF;
    bi->tbl++;

    /* get table name */
    if((r = rs_eof_read_binlog(rd, bi->tb, bi->tbl)) != RS_OK) {
        return r;
    }
#endif

    return RS_OK;
}

int rs_def_finish_handle(rs_request_dump_t *rd) 
{
    rs_binlog_info_t        *bi;

    bi = &(rd->binlog_info);

    if((!bi->tran && !bi->sent) || bi->flush) {

        rs_log_debug(0, "send dump file = %s, send dump pos = %u, tran = %d, "
                "sent = %d, flush = %d", rd->dump_file, 
                rd->dump_pos, bi->tran, bi->sent, bi->flush);

        /* add to ring buffer */
        if(rs_binlog_create_data(rd) != RS_OK) {
            return RS_ERR;
        }
    }

    bi->sent = 0;

    rs_log_debug(0, "tran = %d, sent = %d", bi->tran, bi->sent);

    rs_log_debug(0, "seek binlog pos = %u", bi->np);

    /* seek after server_id and event_length */
    if(fseek(rd->binlog_fp, bi->np, SEEK_SET) == -1) {
        rs_log_err(rs_errno, "fseek(\"%s\") failed", rd->dump_file);
        return RS_ERR;
    }

    return RS_OK;
}
