
#include <rs_config.h>
#include <rs_core.h>

ssize_t rs_read(int fd, void *buf, size_t count) 
{
    ssize_t n;
    int     err;

    for( ;; ) {
        n = read(fd, buf, count);

        if(n < 0) {
            err = errno;
            if(err == EINTR)
                continue;

            rs_log_err(err, "read() failed");
        }

        break;
    }

    return n;
}

ssize_t rs_write(int fd, const void *buf, size_t count) 
{
    ssize_t n;
    int     err;

    for( ;; ) {
        n = write(fd, buf, count);

        if(n < 0) {
            err = errno;
            if(err == EINTR)
                continue;

            rs_log_err(err, "write() failed");
        }

        break;
    }

    return n;

}

ssize_t rs_recv(int fd, void *buf, size_t count, int flags) 
{
    ssize_t n;
    int     err;

    for( ;; ) {
        n = recv(fd, buf, count, flags);

        if(n < 0) {
            err = errno;
            if(err == EINTR) {
                continue;
            }

            if(err == EWOULDBLOCK) {
                return RS_TIMEDOUT;
            }

            rs_log_err(err, "read() failed");
        }

        break;
    }

    return n;
}


int rs_init_io_watch() 
{
    int fd;

    fd = inotify_init();
    if(fd < 0) {
        rs_log_err(rs_errno, "inotify_init() failed");
        return RS_ERR;
    }

    return fd;
}

int rs_add_io_watch(int fd, char *path, uint32_t mask) 
{
    int wd;

    wd = inotify_add_watch(fd, path, mask);
    if(wd < 0) {
        rs_log_err(rs_errno, "inotify_add_watch(\"%s\") failed", path);
        return RS_ERR;
    }

    return wd;
}

int rs_close(int fd) 
{
    if(close(fd) != 0) {
        rs_log_err(rs_errno, "close(%d) failed", fd);
        return RS_ERR;    
    }

    return RS_OK;
}
