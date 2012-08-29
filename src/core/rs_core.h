
#ifndef _RS_CORE_H_INCLUDED_
#define _RS_CORE_H_INCLUDED_

#ifdef MASTER

extern int  rs_init_master();
extern void rs_free_master(void *data);

#elif SLAVE

extern int  rs_init_slave();
extern void rs_free_slave(void *data);

#endif


/* public */
extern char             *rs_conf_path;
extern pid_t            rs_pid;
extern uint32_t         rs_log_level;

extern volatile sig_atomic_t     rs_quit;
extern volatile sig_atomic_t     rs_reload;

#define RS_OK                       0
#define RS_ERR                      -1
#define RS_HAS_BINLOG               -2
#define RS_NO_RECORD                -3
#define RS_HEARTBEAT                -4
#define RS_TIMEDOUT                 -5
#define RS_FULL                     -6
#define RS_EMPTY                    -7
#define RS_NO_BINLOG                -8
#define RS_CMD_ERR                  -9
#define RS_SLAB_OVERFLOW            -10

#define INT32_SIZE            sizeof(int32_t);
#define INT32_LEN             sizeof("-2147483648") - 1


#define UINT64_LEN            sizeof("18446744073709551615") - 1

#define UINT32_LEN            sizeof("4294967295") - 1
#define UINT32_SIZE           sizeof(uint32_t);

#define SMALLINT_LEN          sizeof("65535") - 1
#define TINYINT_LEN           sizeof("255") - 1
#define TIME_LEN              sizeof("2012-12-12 12:12:12") - 1

#define rs_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))
#define rs_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))

#include <rs_errno.h>
#include <rs_string.h>
#include <rs_buf.h>
#include <rs_io.h>
#include <rs_log.h>
#include <rs_slab.h>
#include <rs_ring_buffer2.h>
#include <rs_conf.h>
#include <rs_core_info.h>
#include <rs_process.h>

#endif
