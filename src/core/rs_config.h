
#ifndef _RS_CONFIG_H_INCLUDED_
#define _RS_CONFIG_H_INCLUDED_

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h> 
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <math.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <pwd.h>


#define RS_ALIGNMENT   sizeof(unsigned long)    /* platform word */
#define rs_align_ptr(p, a)                                                   \
    (char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

#define rs_align(d, a)     (((d) + (a - 1)) & ~(a - 1))

#endif
