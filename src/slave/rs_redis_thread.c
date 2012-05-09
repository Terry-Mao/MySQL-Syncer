
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>


static void rs_free_redis_thread(void *data);
static redisReply *rs_redis_command(rs_slave_info_t *si, int free, 
        const char *fmt, ...);
static int rs_redis_dml_message(rs_slave_info_t *si, char *buf, uint32_t len);

static int rs_redis_insert_test(rs_slave_info_t *si, void *data, 
        uint32_t len);

static int rs_redis_delete_test(rs_slave_info_t *si, void *data, 
        uint32_t len);

static int rs_redis_insert_row_based_test(rs_slave_info_t *si, void *data, 
        uint32_t len);

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

        rs_log_debug(0, "d->data = %s", d->data);

        p = d->data;

        /* get flush info */
        if((p = rs_strchr(p, ',')) == NULL) {
            rs_log_err(0, "rs_strchr(\"%s\", ',') failed", p);
            goto free;
        }

        rs_memcpy(si->dump_file, d->data, p - (char *) d->data);

        p++;

        if((p = rs_strchr(p, ',')) == NULL) {
            rs_log_err(0, "rs_strchr(\"%s\", ',') failed", p);
            goto free;
        }

        si->dump_pos = rs_estr_to_uint32(p - 1);

        rs_log_debug(0, "total len = %u", d->len);
        rs_log_debug(0, "file,pos = %u", (p - (char *) d->data));

        /* COMMIT TO REDIS */
        if(rs_redis_dml_message(si, p, d->len - (p - (char *) d->data)) 
                != RS_OK) 
        {
            goto free;
        }

        /* flush slave.info, NOTICE : must skip "," */
        if(rs_flush_slave_info(si, d->data, p - (char *) d->data) != RS_OK) {
            goto free;
        }

        rs_get_ring_buffer2_advance(si->ring_buf);
    } 

free:

    /* pop cleanup handle and execute */
    pthread_cleanup_pop(1);

    pthread_exit(NULL);
}

static int rs_redis_dml_message(rs_slave_info_t *si, char *buf, uint32_t len) 
{
    char                *p, t;

    p = buf;
    t = 0;

    p++; /* skip , */
    t = *p++;

    rs_log_debug(0, "pb len = %u", len - 2);

    if(t == RS_MYSQL_SKIP_DATA) {
        return RS_OK;
    } else if(t == RS_MYSQL_INSERT_TEST) {
        return rs_redis_insert_test(si, p, len - 2);
    } else if(t == RS_MYSQL_DELETE_TEST) {
        return rs_redis_delete_test(si, p, len - 2); 
    } else if(t == RS_MYSQL_ROW_BASED) {
        return rs_redis_insert_row_based_test(si, p, len - 2); 
    }

    rs_log_info("unkonw cmd type = %d", t);

    return RS_OK;
}

static int rs_redis_insert_test(rs_slave_info_t *si, void *data, uint32_t len)
{
    Test *test;

    /* unpack the buffer */
    test = test__unpack(NULL, len, data);

    if(test == NULL) {
        rs_log_err(rs_errno, "test__unpack() failed");
        return RS_ERR;
    }

    /* insert into redis */
    rs_redis_command(si, 1, "SET test_%u %s", test->id, test->msg);

    /* free pb test */
    test__free_unpacked(test, NULL);

    return RS_OK;
}

static int rs_redis_delete_test(rs_slave_info_t *si, void *data, uint32_t len)
{
    Test *test;

    /* unpack the buffer */
    test = test__unpack(NULL, len, data);

    /* insert into redis */
    rs_redis_command(si, 1, "SET test_%u %s", test->id, "");

    /* free pb test */
    test__free_unpacked(test, NULL);

    return RS_OK;
}

static int rs_redis_insert_row_based_test(rs_slave_info_t *si, void *data, 
        uint32_t len) 
{
    /* TODO PARSE WRITE_ROWS EVENT */
    char        *p, *msg;
    uint64_t    cn;
    uint32_t    un, nn, sn, i, j, is_null, id;

    p = data;
    msg = NULL;
    p += 8;


    rs_log_debug(0, "packed integer : %d", *p);

    if((u_char) *p <= 250) {
        cn = (u_char) *p - '\0'; 
        p++;
    } else if((u_char) *p <= 252) {
        rs_memcpy(&cn, p, 2);
        cn &= 0x000000000000FFFF;
        p += 2;
    } else if((u_char) *p <= 253) {
        rs_memcpy(&cn, p, 3);
        cn &= 0x0000000000FFFFFF;
        p += 3;
    } else if((u_char) *p <= 254) {
        rs_memcpy(&cn, p, 4);
        cn &= 0x00000000FFFFFFFF;
        p += 4;
    } else {
        rs_memcpy(&cn, p, 8);
        p += 8;
    }

    rs_log_debug(0, "column number : %llu", cn);

    un = (cn + 7) / 8;
    char use_bits[un];

    rs_memcpy(use_bits, p, un);
    p += un;

    while(p < (char *) data + len) {

        j = 0;    
        nn = (un + 7) / 8;
        char null_bits[nn];

        rs_memcpy(null_bits, p, nn);
        p += nn;

        for(i = 0; i < cn; i++) {

            if(!(use_bits[i / 8] >> (i % 8))  & 0x01) {
                continue;
            }

            is_null = (null_bits[j / 8] >> (j % 8)) & 0x01;

            switch(i) {
                /* id */
                case 0:
                if(is_null) {
                    id = 0;
                } else {
                    rs_memcpy(&id, p, 4);
                    p += 4;
                }
                break;
                /* msg */
                case 1:
                if(is_null) {
                    msg = NULL; 
                } else {
                    if((u_char) *p >= 255) {
                        rs_memcpy(&sn, p, 2);
                        sn &= 0x0000FFFF;
                        p += 2;
                    } else {
                        sn = (u_char) *p;
                        p++;
                    }

                    msg = p;
                    p += sn;
                }
            }

            j++;
        }

        rs_log_debug(0, "\n==============\n"
            "id     : %u\n"
            "msg    : %*.*s\n"
            "============\n",
        id, sn, sn, msg);

        //update to redis
    }

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
        si->redis_thread = 0;
    }

}
