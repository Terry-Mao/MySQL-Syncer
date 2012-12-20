
#include <rs_config.h>
#include <rs_core.h>

volatile sig_atomic_t rs_quit;
volatile sig_atomic_t rs_reload;

pid_t rs_pid;

int rs_init_daemon(rs_core_info_t *ci)
{
    int  fd;

    switch (fork()) {

    case -1:
        rs_log_err(rs_errno, "fork() failed");
        return RS_ERR;

    case 0:
        break;

    default:
        rs_close(rs_log_fd);
        exit(0);
    }

    if (setsid() == -1) {
        rs_log_err(rs_errno, "setsid() failed");
        return RS_ERR;
    }

    umask(0);

    fd = open("/dev/null", O_RDWR);

    if (fd == -1) {
        rs_log_err(rs_errno, "open(\"/dev/null\") failed");
        return RS_ERR;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        rs_log_err(rs_errno, "dup2(STDIN_FILENO) failed");
        return RS_ERR;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        rs_log_err(rs_errno, "dup2(STDOUT_FILENO) failed");
        return RS_ERR;
    }

    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            rs_log_err(rs_errno, "close() failed");
            return RS_ERR;
        }
    }

    return RS_OK;
}

int rs_init_signals(sigset_t *waitset) 
{
    sigemptyset(waitset);

    sigaddset(waitset, SIGINT);     /* CTRL + C */
    sigaddset(waitset, SIGTERM);    /* shell : kill */
    sigaddset(waitset, SIGPIPE);    /* IGNORE */
    sigaddset(waitset, SIGQUIT);    /* CTRL + \ */
    sigaddset(waitset, SIGHUP);     /* RELOAD */

    if (sigprocmask(SIG_BLOCK, waitset, NULL) == -1) {
        rs_log_err(rs_errno, "sigprocmask() failed");
        return RS_ERR;
    }

    return RS_OK;
}

void rs_sig_handle(int signum) {

    int err = errno;

    switch(signum) {

    case SIGPIPE:
        /* ignore */
        break;
    case SIGINT:
        rs_quit = 1;
        rs_log_info("get a SIGINT signal");
        break;
    case SIGTERM:
        rs_quit = 1;
        rs_log_info("get a SIGTERM signal");
        break;
    case SIGQUIT:
        rs_quit = 1;
        rs_log_info("get a SIGQUIT signal");
        break;
    case SIGHUP:
        rs_reload = 1;
        rs_log_info("get a SIGHUP signal");
    }

    errno = err;

    return;
}

int rs_create_pidfile(char *name) 
{
    char    pid_str[INT32_LEN + 1]; 
    int     len, fd;
    ssize_t n;

    len = 0;
    fd = -1;

    fd = open(name, O_CREAT | O_TRUNC | O_WRONLY, 00644);

    if(fd == -1) {
        rs_log_err(rs_errno, "open(\"%s\") failed", name);
        return RS_ERR;
    }

    len = snprintf(pid_str, INT32_LEN + 1, "%d\n", rs_pid);

    if(len < 0) {
        rs_log_err(rs_errno, "snprintf() failed");
        return RS_ERR;
    }

    n = rs_write(fd, pid_str, len); 

    if(n != len) {
        return RS_ERR;
    }

    rs_close(fd);

    return RS_OK;
}

void rs_delete_pidfile(char *name)
{
    if(unlink(name) == -1) {
        rs_log_err(rs_errno, "unlink(\"%s\")  failed", name);
    }
}

int rs_init_uid(char *user) 
{
    struct passwd *u;

    rs_errno = 0;
    u = getpwnam(user);

    if(u == NULL) {
        rs_log_err(rs_errno, "getpwnam(\"%s\")  failed", user);
        return RS_ERR;
    }

    if(setuid(u->pw_uid) == -1) {
        rs_log_err(rs_errno, "setuid() failed");
        return RS_ERR;
    }

    return RS_OK;
}

int rs_init_gid(char *grp) 
{
    struct group *g;

    rs_errno = 0;
    g = getgrnam(grp);

    if(g == NULL) {
        rs_log_err(rs_errno, "getgrnam(\"%s\")  failed", grp);
        return RS_ERR;
    }

    if(setgid(g->gr_gid) == -1) {
        rs_log_err(rs_errno, "setgid() failed");
        return RS_ERR;
    }

    return RS_OK;
}
