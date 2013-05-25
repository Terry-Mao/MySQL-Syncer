
#ifndef _RS_READ_BINLOG_H_INCLUDED_
#define _RS_READ_BINLOG_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

int rs_read_binlog(rs_binlog_info_t *bi, void *buf, uint32_t size);

#endif
