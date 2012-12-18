

#ifndef _RS_IO_H_INCLUDED_
#define _RS_IO_H_INCLUDED_


#include <rs_config.h>
#include <rs_core.h>

#define RS_IN_MODIFY    IN_MODIFY


ssize_t rs_read(int fd, void *buf, size_t count);
int rs_size_read(int fd, void *buf, size_t size);
ssize_t rs_write(int fd, const void *buf, size_t count);
ssize_t rs_recv(int fd, void *buf, size_t count, int flags);

static inline ssize_t rs_write_fd(int fd, void *buf, size_t n)
{
    return write(fd, buf, n);
}

int rs_init_io_watch();
int rs_add_io_watch(int fd, char *path, uint32_t mask);

int rs_close(int fd);
int rs_timed_select(int fd, uint32_t sec, uint32_t usec);

#endif
