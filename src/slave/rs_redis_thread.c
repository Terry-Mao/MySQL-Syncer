
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>


static void rs_free_redis_thread(void *data);

static char *rs_redis_parse_test(char *p, u_char *cm, uint32_t i, void *obj);
static int rs_redis_insert_row_based_test(rs_slave_info_t *si, void *obj);
static int rs_redis_before_update_row_based_test(rs_slave_info_t *si, void *obj);
static int rs_redis_update_row_based_test(rs_slave_info_t *si, void *obj);
static int rs_redis_delete_row_based_test(rs_slave_info_t *si, void *obj);



static int rs_redis_append_command(rs_slave_info_t *si, const char *fmt, ...); 

static int rs_redis_get_replies(rs_slave_info_t *si, uint32_t cmdn);


static int rs_redis_dml_data(rs_slave_info_t *si, char *buf, uint32_t len);

static int rs_redis_dml_row_based(rs_slave_info_t *si, void *data, 
        uint32_t len, char type, 
        rs_redis_dml_row_based_pt write_handle, 
        rs_redis_dml_row_based_pt before_update_handle, 
        rs_redis_dml_row_based_pt update_handle, 
        rs_redis_dml_row_based_pt delete_handle,
        rs_redis_parse_row_based_pt parse_handle, void *obj);

void *rs_start_redis_thread(void *data) 
{/*{{{*/
    int                         r, n, i, s;
    char                        *p;
    rs_ring_buffer2_data_t      *d;
    rs_slave_info_t             *si;

    s = 0;
    si = (rs_slave_info_t *) data;

    /* push cleanup handle */
    pthread_cleanup_push(rs_free_redis_thread, (void *) si);

    if(si == NULL) {
        rs_log_err(0, "redis thread can not get slave info struct");
        goto free;
    }

    for( ;; ) {

        r = rs_get_ring_buffer2(si->ring_buf, &d);

        if(r == RS_ERR) {
            rs_log_err(0, "rs_get_ring_buffer() failed, get dump data");
            goto free;
        }

        if(r == RS_EMPTY) {

            /* commit redis cmd */
            if(si->cmdn) {
                if(rs_redis_get_replies(si, si->cmdn) != RS_OK) {
                    goto free;
                }

                si->cmdn = 0;
            }

            si->cur_binlog_save++;

            for (n = 1; n < RS_RING_BUFFER_SPIN; n <<= 1) {

                for (i = 0; i < n; i++) {
                    rs_cpu_pause();
                }

                r = rs_get_ring_buffer2(si->ring_buf, &d);

                if(r == RS_OK) {
                    break;
                }
            }

            if(r != RS_OK) {
                if(s % 60000000 == 0) {
                    s = 0;
                    rs_log_info("redis thread wait ring buffer fill data");
                }

                s += RS_RING_BUFFER_EMPTY_SLEEP_USEC;

                if(rs_flush_slave_info(si) != RS_OK) {
                    goto free;
                }

                usleep(RS_RING_BUFFER_EMPTY_SLEEP_USEC);
                continue;
            }
        }

        p = d->data;

        /* get flush info */
        if((p = rs_strchr(p, ',')) == NULL) {
            rs_log_err(0, "rs_strchr(\"%s\", ',') failed", p);
            goto free;
        }

        rs_memcpy(si->dump_file, d->data, p - (char *) d->data);

        p++; /* NOTICE : file,pos */

        if((p = rs_strchr(p, ',')) == NULL) {
            rs_log_err(0, "rs_strchr(\"%s\", ',') failed", p);
            goto free;
        }

        si->dump_pos = rs_estr_to_uint32(p - 1);

        /* COMMIT TO REDIS */
        if(rs_redis_dml_data(si, p, d->len - (p - (char *) d->data)) 
                != RS_OK) 
        {
            goto free;
        }

        si->cur_binlog_save++;

        if(rs_flush_slave_info(si) != RS_OK) {
            goto free;
        }

        rs_get_ring_buffer2_advance(si->ring_buf);
    } 

free:

    /* pop cleanup handle and execute */
    pthread_cleanup_pop(1);

    pthread_exit(NULL);
}/*}}}*/

static int rs_redis_dml_data(rs_slave_info_t *si, char *buf, uint32_t len) 
{/*{{{*/
    char                *p, t, *e;
    uint32_t            tl, rl;

    p = buf;
    t = 0;
    tl = 0;
    rl = 0;
    e =  NULL;

    p++; /* NOTICE : pos,mev */
    t = *p++;

    if(t == RS_MYSQL_SKIP_DATA) {
        return RS_OK;

    } else if(t == RS_WRITE_ROWS_EVENT || t == RS_UPDATE_ROWS_EVENT || 
            t == RS_DELETE_ROWS_EVENT) 
    {
        p++; /* NOTICE : mev,db.tabletran */

        if((e = rs_strchr(p, ',')) == NULL) {
            rs_log_err(0, "unkown cmd format");
            return RS_ERR;
        }

        tl = e++ - p;
        rl = len - 4 - tl;

        if(rs_strncmp(p, RS_FILTER_TABLE_TEST_TEST, tl) == 0) {

            /* test.test */
            rs_mysql_test_t test;

            if(rs_redis_dml_row_based(si, e, rl, t,
                        rs_redis_insert_row_based_test, 
                        rs_redis_before_update_row_based_test,
                        rs_redis_update_row_based_test, 
                        rs_redis_delete_row_based_test, 
                        rs_redis_parse_test, &test) != RS_OK)
            {
                return RS_ERR;
            }
        } else {
            rs_log_err(0, "unknown row-based binlog filter table");
            return RS_ERR;
        }

        /* commit redis cmd */
        if(si->cmdn >= RS_REDIS_CMD_COMMIT_NUM) {
            if(rs_redis_get_replies(si, si->cmdn) != RS_OK) {
                return RS_ERR;
            }

            si->cmdn = 0;
        }

    } else {
        rs_log_err(0, "unkonw cmd type = %d", t);
        return RS_ERR;
    }

    return RS_OK;
}/*}}}*/


static int rs_redis_dml_row_based(rs_slave_info_t *si, void *data, 
        uint32_t len, char type, rs_redis_dml_row_based_pt write_handle, 
        rs_redis_dml_row_based_pt before_update_handle,
        rs_redis_dml_row_based_pt update_handle,
        rs_redis_dml_row_based_pt delete_handle,
        rs_redis_parse_row_based_pt parse_handle, void *obj)
{/*{{{*/
    char                        *p;
    uint32_t                    i, j, un, nn, before, ml, cn, cl, cmdn;
    int                         cmd;
    rs_redis_dml_row_based_pt   handle;

    p = data;
    before = 0;
    cmdn = 0;
    handle = NULL;

    if(si == NULL || data == NULL || obj == NULL || parse_handle == NULL)
    {
        rs_log_err(0, "rs_redis_dml_row_based() failed, args can't be null");
        return RS_ERR;
    }

    /* get column number */
    rs_memcpy(&cn, p, 4);
    p += 4;

    u_char ct[cn];
    /* get column type */
    rs_memcpy(ct, p, cn);
    p += cn;

    /* get column meta length */
    rs_memcpy(&ml, p, 4);
    p += 4;

    u_char cm[ml];
    /* get column meta */
    rs_memcpy(cm, p, ml);
    p += ml;

    /* skip table id and reserved id */
    p += 8;

    /* get column number */
    cn = (uint32_t) rs_parse_packed_integer(p, &cl);
    p += cl;

    /* get used bits */
    un = (cn + 7) / 8;

    char use_bits[un], use_bits_after[un];

    rs_memcpy(use_bits, p, un);
    p += un;

    /* only for update event */
    if(type == RS_UPDATE_ROWS_EVENT) {
        rs_memcpy(use_bits_after, p, un);
        p += un;    
    }

    /* get column value */
    while(p < (char *) data + len) {

        j = 0;    
        cmd = 0;
        nn = (un * 8 + 7) / 8;
        char null_bits[nn];

        rs_memcpy(null_bits, p, nn);
        p += nn;

        for(i = 0; i < cn; i++) {

            /* used */
            if((type == RS_UPDATE_ROWS_EVENT && before == 0) ||
                    type != RS_UPDATE_ROWS_EVENT) 
            {
                if(!(use_bits[i / 8] >> (i % 8))  & 0x01) {
                    continue;
                }
            } else if(type == RS_UPDATE_ROWS_EVENT && before == 1) {
                if(!(use_bits_after[i / 8] >> (i % 8))  & 0x01) {
                    continue;
                }
            }

            /* null */
            if(!((null_bits[j / 8] >> (j % 8)) & 0x01)) {
                /* parse */
                p = parse_handle(p, cm, i, obj);
            }

            j++;
        }

        /* append redis cmd */
        if(type == RS_WRITE_ROWS_EVENT) {
            handle = write_handle;
        } else if(type == RS_UPDATE_ROWS_EVENT) {
            if(before++ == 1) {
                handle = update_handle;
                before = 0;
            } else {
                handle = before_update_handle;
            }
        } else if(type == RS_UPDATE_ROWS_EVENT) {
            handle = delete_handle;
        } else {
            return RS_ERR;
        }

        if(handle == NULL) {
            continue;
        }

        if((cmd = handle(si, obj)) == RS_ERR) {
            rs_log_err(rs_errno, "handle() failed");
            return RS_ERR;
        }

        cmdn += (uint32_t) cmd;

    }

    si->cmdn += cmdn;

    return RS_OK;
}/*}}}*/

/* test */
static char *rs_redis_parse_test(char *p, u_char *cm, uint32_t i, void *obj)
{
    uint32_t        cl, sn;
    rs_mysql_test_t *test;

    cl = 0;
    sn = 0;
    test = (rs_mysql_test_t *) obj;

    switch(i) {
        /* ID */
        case 0:
            rs_mysql_test_t_init(test);
            rs_memcpy(&(test->id), p, 4);
            cl = 4;
            break;
        /* COL */
        case 1:
            rs_memcpy(&sn, p, 1);
            p += 1;

            rs_memcpy(test->col, p, sn);

            rs_log_debug(0,
                    "\n========== test ==========\n"
                    "ID             : %lu\n"
                    "COL            : %s\n"
                    "\n==============================\n",
                    test->id,
                    test->col
                    );

            cl = sn;

            break;

        default:
            break;
    }

    return p + cl;
}

static int rs_redis_insert_row_based_test(rs_slave_info_t *si, void *obj)
{/*{{{*/
    rs_mysql_test_t *test;
    uint32_t            cmdn;

    if(si == NULL || obj == NULL) {
        return RS_ERR;
    }  

    test = (rs_mysql_test_t *) obj;
    cmdn = 0;

    if(rs_redis_append_command(si, "SET test_%u %s", test->id, 
                test->col) != RS_OK)
    {
        return RS_ERR;
    }

    cmdn++;

    return cmdn;
}/*}}}*/

static int rs_redis_before_update_row_based_test(rs_slave_info_t *si, void *obj)
{/*{{{*/
    return RS_OK;
}/*}}}*/

static int rs_redis_update_row_based_test(rs_slave_info_t *si, void *obj)
{
    return rs_redis_insert_row_based_test(si, obj);
}

static int rs_redis_delete_row_based_test(rs_slave_info_t *si, void *obj)
{/*{{{*/
    return RS_OK;
}/*}}}*/


/* redis api */
static int rs_redis_append_command(rs_slave_info_t *si, const char *fmt, ...) 
{/*{{{*/
    va_list         args;
    redisContext    *c;
    int i, err;

    i = 0;
    err = 0;
    c = si->c;

    for( ;; ) {

        if(c == NULL) {

            /* retry connect*/
            c = redisConnect(si->redis_addr, si->redis_port);

            if(c->err) {
                if(i % 60 == 0) {
                    i = 0;
                    rs_log_err(rs_errno, "redisConnect(\"%s\", %d) failed, %s"
                            , si->redis_addr, si->redis_port, c->errstr);
                }

                redisFree(c);
                c = NULL;

                i += RS_REDIS_CONNECT_RETRY_SLEEP_SEC;
                sleep(RS_REDIS_CONNECT_RETRY_SLEEP_SEC); 

                continue;
            }
        }

        va_start(args, fmt);
        err = redisvAppendCommand(c, fmt, args);
        va_end(args);

        break;
    }

    si->c = c;

    if(err != REDIS_OK) {
        rs_log_err(rs_errno, "redisvAppendCommand() failed");
        return RS_ERR;
    } 

    return RS_OK;
}/*}}}*/

static int rs_redis_get_replies(rs_slave_info_t *si, uint32_t cmdn)
{/*{{{*/
    redisReply  *rp;

    while(cmdn--) {

        if(redisGetReply(si->c, (void *) &rp) != REDIS_OK) {
            return RS_ERR;
        }

        freeReplyObject(rp);
    } 

    return RS_OK;
}/*}}}*/

static void rs_free_redis_thread(void *data)
{
    rs_slave_info_t *si;

    si = (rs_slave_info_t *) data;

    if(si != NULL) {
        rs_log_info("set redis thread exit state");
        si->redis_thread_exit = 1;
        if(rs_quit == 0 && rs_reload == 0) {
            rs_log_info("redis thread send SIGQUIT signal");
            kill(rs_pid, SIGQUIT);
        }
    }
}
