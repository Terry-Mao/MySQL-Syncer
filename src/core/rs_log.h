#ifndef _RS_LOG_H_INCLUDED_
#define _RS_LOG_H_INCLUDED_


#include <rs_config.h>
#include <rs_core.h>

#define RS_MAX_ERROR_STR    2048

#define RS_LOG_MAX          2

#define RS_LOG_ERR          0
#define RS_LOG_INFO         1    
#define RS_LOG_DEBUG        2

#define RS_LOG_ERR_STR      "[ERROR] "
#define RS_LOG_INFO_STR     "[INFO]  "
#define RS_LOG_DEBUG_STR    "[DEBUG] "

#define RS_DEBUG_FIRST      0x010
#define RS_DEBUG_ALLOC      0x010
#define RS_DEBUG_HASH       0x020
#define RS_DEBUG_TMPBUF     0x040
#define RS_DEBUG_RINGBUF    0x080
#define RS_DEBUG_BINLOG     0x100

#define RS_DEBUG_ALL            0x7FFFFFF0

#define RS_DEBUG_ALLOC_STR      "ALLOC"
#define RS_DEBUG_HASH_STR       "HASH"
#define RS_DEBUG_TMPBUF_STR     "TMPBUF"
#define RS_DEBUG_RINGBUF_STR    "RINGBUF"
#define RS_DEBUG_BINLOG_STR     "BINLOG"


int rs_log_init(char *name, int flags);
int rs_log_set_levels(char *debug_level);

void rs_log_debug(uint32_t level, rs_err_t err, const char *fmt, ...);
void rs_log_error(uint32_t level, rs_err_t err, const char *fmt, ...);
void rs_log_stderr(rs_err_t err, const char *fmt, ...);



#endif
