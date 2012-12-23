
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

            rs_log_err(rs_errno, "read() failed");
        }

        break;
    }

    return n;
}


int rs_size_read(int fd, void *buf, size_t size) 
{
    size_t  cur_size;
    ssize_t n;
    char    *pack_buf;

    pack_buf = (char *) buf;
    cur_size = 0;

    while(cur_size != size) {

        n = rs_read(fd, pack_buf + cur_size, size - cur_size);

        if(n <= 0) {
            return RS_ERR;
        }

        cur_size += (size_t) n;
    }

    return RS_OK;
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

            rs_log_err(rs_errno, "write() failed");
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

int rs_timed_select(int fd, uint32_t sec, uint32_t usec)
{
    int                     ready, mfd;
    fd_set                  rset, tset;
    struct timeval          tv;

    mfd = 0;
    ready = 0;

    FD_ZERO(&rset);
    FD_ZERO(&tset);
    mfd = rs_max(fd, mfd);
    tv.tv_sec = sec;
    tv.tv_usec = usec;

    FD_SET(fd, &rset);
    tset = rset;
    ready = select(mfd + 1, &tset, NULL, NULL, &tv);

    for(;;) {
        /* select timedout */
        if(ready == 0) {
            return RS_TIMEDOUT;
        }

        /* select error */
        if(ready == -1) {
            if(rs_errno == EINTR) {
                continue;
            }

            rs_log_err(rs_errno, "select failed()");
            return RS_ERR;
        }

        /* unknown fd */
        if(!FD_ISSET(fd, &tset)) {
            rs_log_err(rs_errno, "select failed(), unknown fd");
            return RS_ERR;
        }

        FD_CLR(fd, &rset);
        break;
    }

    return RS_OK;
}
