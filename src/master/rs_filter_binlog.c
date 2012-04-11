
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

static char *rs_create_test_event(char *buf, void *data);
static int rs_filter_test(rs_binlog_info_t *bi);

int rs_def_filter_data_handle(rs_request_dump_t *rd)
{
    rs_binlog_info_t    *bi;
    rs_mysql_test_t     *t;

    bi = &(rd->binlog_info);

    if(rs_strncmp(bi->db, RS_FILTER_DB_NAME, bi->dbl) != 0) {
        return RS_OK;
    }

    if(bi->data == NULL) {
        
        bi->data = malloc(RS_SYNC_DATA_SIZE);

        if(bi->data == NULL) {
            rs_log_err(rs_errno, "malloc() failed, rs_sync_data_size", 
                    RS_SYNC_DATA_SIZE);
            return RS_ERR;
        }
    }

    if(bi->tran == 0 && rs_memcmp(bi->sql, TEST_FILTER_KEY, 
                TEST_FILTER_KEY_LEN) == 0) {

        t = (rs_mysql_test_t *) bi->data; 
        rs_mysql_test_t_init(t);

        /* INSERT TEST */
        if(rs_filter_test(bi) != RS_OK) {
            return RS_ERR;
        }

        bi->flush = 1;
        bi->mev = RS_MYSQL_INSERT_TEST;
    }

    return RS_OK;
}

int rs_def_create_data_handle(rs_request_dump_t *rd) 
{
    int                     i, r, len;
    char                    *p;
    rs_binlog_info_t        *bi;
    rs_ring_buffer_data_t   *d;

    i = 0;
    bi = &(rd->binlog_info);

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
                    rd->dump_file, rd->dump_pos, bi->mev); 

            if(len < 0) {
                rs_log_err(rs_errno, "snprintf() failed");
                return RS_ERR;
            }

            if(bi->mev == 0) {
                d->len = len;
            } else {
                p += len;
                p = rs_create_test_event(p, bi->data);

                if(p != NULL) {
                    d->len = p - d->data;
                    rs_set_ring_buffer_advance(&(rd->ring_buf));
                }
            }

            rs_set_ring_buffer_advance(&(rd->ring_buf));

            break;
        }

    } // EXIT RING BUFFER 

    bi->flush = 0;
    bi->mev = 0;
    bi->sent = 1;

    return RS_OK;
}

static int rs_filter_test(rs_binlog_info_t *bi)
{
    char            *p;
    rs_mysql_test_t *t;

    p = bi->sql;
    t = (rs_mysql_test_t *) bi->data;

    if(bi->auto_incr == 0) {
        rs_log_err(0, "filter test failed, no auto_incr");
    }

    /* COLUMN : ID */
    t->id = (uint32_t) bi->ai;
    t->nullbit[0] |= (1 << 0);

    if((p = rs_strstr_end(p, TEST_COLUMN_MSG, TEST_COLUMN_MSG_LEN)) == NULL) {
        rs_log_err(0, "filter test failed, \"%s\"", TEST_COLUMN_MSG);
        return RS_ERR;
    }
    
    /* COLUMN : MSG */
    p = rs_cp_utf8_str(t->msg, p);
    t->nullbit[0] |= (1 << 1);

    return RS_OK;
}

static char *rs_create_test_event(char *buf, void *data)
{
    char            *p;
    size_t          len;
    rs_mysql_test_t *t;

    p = buf;
    t = (rs_mysql_test_t *) data;

    /* set nullbit */
    p = rs_cpymem(p, t->nullbit, RS_MYSQL_TEST_NULL_BIT_NUM);

    /* set var-length value */
    len = rs_strlen(t->msg) + 1;
    p = rs_cpymem(p, &len, 4);
    p = rs_cpymem(p, t->msg, len);

    return p;
}
