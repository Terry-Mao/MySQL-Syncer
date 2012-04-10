
#ifndef _RS_ERRNO_H_INCLUDED_
#define _RS_ERRNO_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>

typedef int rs_err_t;

#define rs_errno       errno
#define RS_SYS_NERR    135

int rs_strerror_init(void);
char *rs_strerror(rs_err_t err, char *errstr, size_t size);


#endif
