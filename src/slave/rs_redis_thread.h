
#ifndef _RS_REDIS_THREAD_H_INCLUDED_
#define _RS_REDIS_THREAD_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

#define RS_REDIS_CMD_COMMIT_NUM             36

#define RS_RING_BUFFER_EMPTY_SLEEP_USEC     (1000 * 10)

int rs_tables_handle(rs_slave_info_t *si, char *p, uint32_t tl, char *e, 
        uint32_t rl, char t);

void *rs_start_redis_thread(void *data);

#endif
