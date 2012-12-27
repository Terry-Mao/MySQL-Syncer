
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

/* redis api */
int rs_redis_get_replies(rs_slave_info_t *si)
{
    redisReply  *rp;

    while(si->cmdn) {

        if(redisGetReply(si->c, (void *) &rp) != REDIS_OK) {
            return RS_ERR;
        }

        freeReplyObject(rp);
        si->cmdn--;
    }

    return RS_OK;
}

int rs_redis_append_command(rs_slave_info_t *si, const char *fmt, ...) 
{
    va_list         args;
    redisContext    *c;
    int             i, err;

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
                    rs_log_error(RS_LOG_ERR, rs_errno, "redisConnect(\"%s\", "
                            "%d) failed, %s" , si->redis_addr, si->redis_port, 
                            c->errstr);
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
        rs_log_error(RS_LOG_ERR, rs_errno, "redisvAppendCommand() failed");
        return RS_ERR;
    }

    si->cmdn++;

    return RS_OK;
}
