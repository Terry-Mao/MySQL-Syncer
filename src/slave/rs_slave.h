
#ifndef _RS_SLAVE_H_INCLUDED_
#define _RS_SLAVE_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>

#define RS_SLAVE_MODULE_NAME        "slave"


typedef struct rs_slave_info_s          rs_slave_info_t;
typedef struct rs_redis_thread_info_s   rs_redis_thread_info_t;


extern rs_slave_info_t                  *rs_slave_info; 

#include <hiredis.h>
#include <rs_slave_info.h>
#include <rs_io_thread.h>
#include <rs_redis_thread.h>

#endif

