

#ifndef _RS_REDIS_H_INCLUDED_
#define _RS_REDIS_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

#define RS_REDIS_CONNECT_RETRY_SLEEP_SEC    10

int rs_redis_get_replies(rs_slave_info_t *si);
int rs_redis_append_command(rs_slave_info_t *si, const char *fmt, ...);

#endif
