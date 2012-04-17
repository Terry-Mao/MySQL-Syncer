
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>


static void rs_free_redis_thread(void *data);
static redisReply *rs_redis_command(rs_slave_info_t *si, int free, 
        const char *fmt, ...);
static int rs_redis_dml_message(rs_slave_info_t *si, char *buf, uint32_t len);
static int rs_redis_insert_test(rs_slave_info_t *si, Test test);
static int rs_redis_delete_test(rs_slave_info_t *si, Test test);


void *rs_start_redis_thread(void *data) 
{
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
                if(s % 60 == 0) {
                    s = 0;
                    rs_log_info("redis thread wait ring buffer fill data");
                }

                s += RS_RING_BUFFER_EMPTY_SLEEP_SEC;

                sleep(RS_RING_BUFFER_EMPTY_SLEEP_SEC);
                continue;
            }
        }

        p = d->data;

        /* get flush info */
        if((p = rs_strchr(p, ',')) == NULL) {
            rs_log_err(0, "rs_strchr(\"%s\", ',') failed", p);
            goto free;
        }

        rs_memcpy(si->dump_file, d->data, p - d->data);

        p++;

        if((p = rs_strchr(p, ',')) == NULL) {
            rs_log_err(0, "rs_strchr(\"%s\", ',') failed", p);
            goto free;
        }

        si->dump_pos = rs_estr_to_uint32(p - 1);

        /* COMMIT TO REDIS */
        if(rs_redis_dml_message(si, p, d->len - (p - (cha *) d->data)) 
                != RS_OK) 
        {
            goto free;
        }

        /* flush slave.info, NOTICE : must skip "," */
        if(rs_flush_slave_info(si, d->data, p - d->data) != RS_OK) {
            goto free;
        }

        rs_get_ring_buffer2_advance(si->ring_buf);
    } 

free:

    /* pop cleanup handle and execute */
    pthread_cleanup_pop(1);

    return NULL;
}

static int rs_redis_dml_message(rs_slave_info_t *si, char *buf, uint32_t len) 
{
    char                *p, t;

    p = buf;
    t = 0;

    p++; /* skip , */
    t = *p++;

    if(t == RS_MYSQL_SKIP_DATA) {
        return RS_OK;
    } else if(t == RS_MYSQL_INSERT_TEST) {
        return rs_redis_insert_test(si, p, len - 1);
    } else if(t == RS_MYSQL_DELETE_TEST) {
        return rs_redis_delete_test(si, p, len - 1); 
    }

    rs_log_info("unkonw cmd type = %d", t);

    return RS_OK;
}

static int rs_redis_insert_test(rs_slave_info_t *si, void *data, uint32_t len)
{
    Test *test;

    /* unpack the buffer */
    test = test__unpack(NULL, len, data);

    /* insert into redis */
    rs_redis_command(si, 1, "SET test_%u %s", test->id, test->msg);
    
    /* free pb test */
    test__free_unpacked(msg, NULL);

    return RS_OK;
}

static int rs_redis_delete_test(rs_slave_info_t *si, Test test)
{
    Test *test;

    /* unpack the buffer */
    test = test__unpack(NULL, len, data);

    /* insert into redis */
    rs_redis_command(si, 1, "SET test_%u %s", test->id, "");
    
    /* free pb test */
    test__free_unpacked(msg, NULL);

    return RS_OK;
}

static redisReply *rs_redis_command(rs_slave_info_t *si, int free, 
        const char *fmt, ...) 
{
    va_list         args;
    redisReply      *rp;
    redisContext    *c;
    int i;

    i = 0;
    c = si->c;

    for( ;; ) {

        /* init var */
        rp = NULL;

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
        rp = redisvCommand(c, fmt, args);
        va_end(args);

        if(rp == NULL || rp->type == REDIS_REPLY_ERROR) {

            /* retry connect */
            rs_log_err(rs_errno, "rs_redis_command() failed, \"%s\" command "
                    "error", fmt);
            redisFree(c);
            c = NULL;

            sleep(RS_REDIS_CONNECT_RETRY_SLEEP_SEC); 
            freeReplyObject(rp);

            continue;
        }    

        break;
    }

    si->c = c;

    if(free) {
        freeReplyObject(rp);
        return NULL;
    }


    return rp;
}

static void rs_free_redis_thread(void *data)
{
    rs_slave_info_t *si;

    si = (rs_slave_info_t *) data;

    if(si != NULL) {
        kill(rs_pid, SIGQUIT);
    }
}
