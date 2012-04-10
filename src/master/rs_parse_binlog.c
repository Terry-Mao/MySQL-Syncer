
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

int rs_redis_header_handle(rs_request_dump_t *rd) 
{
    int                 r;
    uint32_t            p;
    rs_binlog_info_t    *bi;

    bi = &(rd->binlog_info);

    p = rd->dump_pos + BINLOG_TIMESTAMP_LEN + 
        (rd->dump_pos == 0 ? MAGIC_NUM_LEN : 0);

    /* skip timestamp */
    if(fseek(rd->binlog_fp, p, SEEK_SET) == -1) {
        rs_log_err(rs_errno, "fseek() failed, seek_set pos = %u", p);
        return RS_ERR;
    }

    /* get event type */
    if((r = rs_eof_read_binlog(rd, &(bi->t), BINLOG_TYPE_CODE_LEN)) 
            != RS_OK) 
    {
        return r;
    }

    /* skip server id */
    if(fseek(rd->binlog_fp, BINLOG_SERVER_ID_LEN, SEEK_CUR) == -1) {
        rs_log_err(rs_errno, "fseek() failed, seek_cur pos = %u", 
                BINLOG_SERVER_ID_LEN);
        return RS_ERR;
    }

    /* get event_length */
    if((r = rs_eof_read_binlog(rd, &(bi->el), BINLOG_EVENT_LENGTH_LEN)) 
            != RS_OK) 
    {
        return r;
    }

    /* get next position */
    if((r = rs_eof_read_binlog(rd, &(bi->np), BINLOG_NEXT_POSITION_LEN)) 
            != RS_OK) 
    {
        return r;
    }

    rd->dump_pos = bi->np;

    if(!bi->el) {
        rs_log_err(0, "binlog crash");
        return RS_ERR;
    }

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

int rs_redis_query_handle(rs_request_dump_t *rd) 
{
    int r;
    rs_binlog_info_t *bi;

    bi = &(rd->binlog_info);

    /* seek after flags and unwant fixed data */
    if(fseek(rd->binlog_fp, BINLOG_FLAGS_LEN + BINLOG_SKIP_FIXED_DATA_LEN, 
                SEEK_CUR) == -1) {
        return RS_ERR;
    }

    /* get database name len */
    if((r = rs_eof_read_binlog(rd, &(bi->dbl), BINLOG_DATABASE_NAME_LEN)) != 
            RS_OK) {
        return r;
    }

    bi->dbl = bi->dbl & 0x000000FF;
    bi->dbl++;

    /* seek after error code */
    if(fseek(rd->binlog_fp, BINLOG_ERROR_CODE_LEN, SEEK_CUR) == -1) {
        return RS_ERR;
    }

    /* get status block len */
    if((r = rs_eof_read_binlog(rd, &(bi->sbl), BINLOG_STATUS_BLOCK_LEN)) != 
            RS_OK) {
        return r;
    }

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
    bi->sl = bi->el - BINLOG_EVENT_HEADER_LEN - bi->sbl - 
        BINLOG_FIXED_DATA_LEN - bi->dbl; 

    bi->sl = rs_min(SQL_MAX_LEN, bi->sl);

    if((r = rs_eof_read_binlog(rd, bi->sql, rs_min(SQL_MAX_LEN, bi->sl))) 
            != RS_OK) {
        return r;
    }

    bi->sql[bi->sl] = '\0';

    rs_log_debug(0, 
            "\n========== query event ==============\n"
            "database name          : %s\n"
            "query sql              : %s\n"
            "\n=====================================\n",
            bi->db,
            bi->sql,
            bi->np);

    if(rs_strncmp(bi->sql, TRAN_KEYWORD, TRAN_KEYWORD_LEN) == 0) {
        rs_log_info("START TRANSACTION");
        bi->tran = 1;
    } else if(rs_strncmp(bi->sql, TRAN_END_KEYWORD, 
                TRAN_END_KEYWORD_LEN) == 0) {
        rs_log_info("END TRANSACTION");
        bi->tran = 0;
    }

    if(rs_binlog_filter_data(rd) != RS_OK) {
        return RS_ERR;
    }

    return RS_OK;
}


int rs_redis_intvar_handle(rs_request_dump_t *rd) 
{
    int                 r;
    rs_binlog_info_t    *bi;

    bi = &(rd->binlog_info);

    /* seek after flags */
    if(fseek(rd->binlog_fp, BINLOG_FLAGS_LEN, SEEK_CUR) == -1) {
        return RS_ERR;
    }

    /* get intvar type */
    if((r = rs_eof_read_binlog(rd, &(bi->intvar_t), BINLOG_INTVAR_TYPE_LEN)) 
            != RS_OK) {
        return r;
    }

    if(bi->intvar_t != BINLOG_INTVAR_TYPE_INCR) { 
        return RS_OK;
    }

    /* get auto_increment id */
    if((r = rs_eof_read_binlog(rd, &(bi->ai), BINLOG_INTVAR_INSERT_ID_LEN)) 
            != RS_OK) {
        return r;
    }

    rs_log_debug(0, "get increment id = %lu", bi->ai);

    bi->auto_incr = 1;

    return RS_OK;
}

int rs_redis_xid_handle(rs_request_dump_t *rd) 
{

    int                     r, len, i;
    char                    *p;
    rs_ring_buffer_data_t   *d;
    rs_binlog_info_t        *bi;

    i = 0;
    bi = &(rd->binlog_info);

    /* flush events */
    if(bi->flush) {

        /* add to ring buffer */
        for( ;; ) {

            r = rs_set_ring_buffer(&(rd->ring_buf), &d);

            if(r == RS_FULL) {
                if(i % 60 == 0) {
                    rs_log_info("request dump's ring buffer is full, wait"
                            "%u secs" ,RS_RING_BUFFER_FULL_SLEEP_SEC);
                    i = 0;
                }

                i += RS_RING_BUFFER_FULL_SLEEP_SEC;

                sleep(RS_RING_BUFFER_FULL_SLEEP_SEC);

                continue;
            }

            if(r == RS_ERR) {
                return RS_ERR;
            }

            if(r == RS_OK) {
                p = d->data;

                len = snprintf(p, RS_SYNC_DATA_CMD_SIZE, "%s,%u,%c", 
                        rd->dump_file, rd->dump_pos, bi->mev); 

                if(len < 0) {
                    rs_log_err(rs_errno, "snprintf() failed, dump_file, "
                            "dump_pos");
                    return RS_ERR;
                }

                p += len;
                p = rs_create_message_event(p, bi->data);

                if(p != NULL) {
                    d->len = p - d->data;
                    rs_set_ring_buffer_advance(&(rd->ring_buf));
                }

                break;
            }

        } // EXIT RING BUFFER 

        bi->flush = 0;
        bi->mev = 0;
        bi->sent = 1;
    }

    bi->tran = 0;

    return RS_OK;
}

int rs_redis_finish_handle(rs_request_dump_t *rd) 
{
    int                     i, r, len;
    char                    *p;
    rs_binlog_info_t        *bi;
    rs_ring_buffer_data_t   *d;

    i = 0;
    bi = &(rd->binlog_info);

    if(!bi->tran && !bi->sent) {

        rs_log_info("send dump file = %s, send dump pos = %u", rd->dump_file, 
                rd->dump_pos);

        /* add to ring buffer */
        for( ;; ) {

            r = rs_set_ring_buffer(&(rd->ring_buf), &d);

            if(r == RS_FULL) {
                if(i % 60 == 0) {
                    rs_log_info("request dump's ring buffer is full, wait %u "
                            "secs" ,RS_RING_BUFFER_FULL_SLEEP_SEC);
                    i = 0;
                }

                i += RS_RING_BUFFER_FULL_SLEEP_SEC;

                sleep(RS_RING_BUFFER_FULL_SLEEP_SEC);

                continue;
            }

            if(r == RS_ERR) {
                return RS_ERR;
            }

            if(r == RS_OK) {
                p = d->data;

                len = snprintf(p, RS_SYNC_DATA_CMD_SIZE, "%s,%u,%c", 
                        rd->dump_file, rd->dump_pos, RS_MYSQL_SKIP_DATA); 

                if(len < 0) {
                    rs_log_err(rs_errno, "snprintf() failed");
                    return RS_ERR;
                }

                d->len = len;

                rs_set_ring_buffer_advance(&(rd->ring_buf));

                break;
            }

        } // EXIT RING BUFFER 
    }

    bi->sent = 0;

    rs_log_debug(0, "tran = %d, sent = %d", bi->tran, bi->sent);

    rs_log_info("seek binlog pos = %u", bi->np);

    /* seek after server_id and event_length */
    if(fseek(rd->binlog_fp, bi->np, SEEK_SET) == -1) {
        rs_log_err(rs_errno, "fseek(\"%s\") failed", rd->dump_file);
        return RS_ERR;
    }

    return RS_OK;
}
