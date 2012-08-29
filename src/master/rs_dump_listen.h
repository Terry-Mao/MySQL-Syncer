
#ifndef _RS_DUMP_LISTEN_H_INCLUDED_
#define _RS_DUMP_LISTEN_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

#define RS_BACKLOG              512
#define RS_RETRY_BIND_TIMES     5
#define RS_RETRY_BIND_SLEEP_MSC 500

int rs_dump_listen(rs_master_info_t *mi); 
void *rs_start_accept_thread(void *data); 

#endif
