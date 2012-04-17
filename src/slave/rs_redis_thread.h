

#ifndef _RS_REDIS_THREAD_H_INCLUDED_
#define _RS_REDIS_THREAD_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>


#define RS_REDIS_CONNECT_RETRY_SLEEP_SEC    10
#define RS_RING_BUFFER_EMPTY_SLEEP_SEC      1      

#define RS_MYSQL_SKIP_DATA                  0
#define RS_MYSQL_INSERT_TEST                1
#define RS_MYSQL_DELETE_TEST                2

void *rs_start_redis_thread(void *data);

#endif
