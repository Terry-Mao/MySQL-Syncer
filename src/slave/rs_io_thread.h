
#ifndef _RS_IO_THREAD_H_INCLUDED_
#define _RS_IO_THREAD_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

#define RSYLOG_RING_BUFFER_NUM      50000

#define RS_REGISTER_SLAVE_CMD_LEN   1 + PATH_MAX + UINT32_LEN


#define RS_CMD_RETRY_TIMES          600
#define RS_RETRY_CONNECT_SLEEP_SEC  10

#define RS_RING_BUFFER_FULL_SLEEP_SEC    5


void *rs_start_io_thread(void *data); 

#endif
