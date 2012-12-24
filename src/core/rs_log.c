

#include <rs_config.h>
#include <rs_core.h>

static void rs_get_time(char *buf);

static rs_str_t rs_log_level_list[] = {
    rs_string(RS_LOG_LEVEL_ERR_STR),
    rs_string(RS_LOG_LEVEL_INFO_STR),
    rs_string(RS_LOG_LEVEL_DEBUG_STR),
#if MASTER
    rs_string(RS_LOG_LEVEL_MASTER_STR),
#elif SLAVE
    rs_string(RS_LOG_LEVEL_SLAVE_STR),
#endif
    rs_string(RS_LOG_LEVEL_CORE_STR),
};


char        *rs_log_path = "./rs.log";
uint32_t    rs_log_level = RS_LOG_LEVEL_DEBUG;
int         rs_log_fd = STDOUT_FILENO;


static void rs_log_base(rs_err_t err, int fd, uint32_t level, const char *fmt, 
        va_list args);

int rs_log_init(char *name, int flags) 
{
    if(name == NULL) {
        name = rs_log_path;
    }

    return open(name, flags, 00644);
}

void rs_log_err(rs_err_t err, const char *fmt, ...) 
{
    va_list args;

    va_start(args, fmt);
    rs_log_base(err, rs_log_fd, RS_LOG_LEVEL_ERR, fmt, args);
    va_end(args);
}

void rs_log_debug(rs_err_t err, const char *fmt, ...) 
{
    va_list args;

    va_start(args, fmt);
    rs_log_base(err, rs_log_fd, RS_LOG_LEVEL_DEBUG, fmt, args);
    va_end(args);

}

void rs_log_info(const char *fmt, ...) 
{
    va_list args;

    va_start(args, fmt);
    rs_log_base(0, rs_log_fd, RS_LOG_LEVEL_INFO, fmt, args);
    va_end(args);
}

void rs_log_master(const char *fmt, ...) 
{
    va_list args;

    va_start(args, fmt);
    rs_log_base(0, rs_log_fd, RS_LOG_LEVEL_MASTER, fmt, args);
    va_end(args);
}

void rs_log_slave(const char *fmt, ...) 
{
    va_list args;

    va_start(args, fmt);
    rs_log_base(0, rs_log_fd, RS_LOG_LEVEL_SLAVE, fmt, args);
    va_end(args);
}

void rs_log_core(const char *fmt, ...) 
{
    va_list args;

    va_start(args, fmt);
    rs_log_base(0, rs_log_fd, RS_LOG_LEVEL_CORE, fmt, args);
    va_end(args);
}


void rs_log_stderr(rs_err_t err, const char *fmt, ...) 
{
    va_list args;

    va_start(args, fmt);
    rs_log_base(err, STDERR_FILENO, RS_LOG_LEVEL_ERR, fmt, args);
    va_end(args);

}

static void rs_log_base(rs_err_t err, int fd, uint32_t level, const char *fmt, 
        va_list args) 
{
    if(rs_log_level < level) {
        return;
    }

    char        *p, *last;
    int         n;
    char        errstr[RS_MAX_ERROR_STR], timestr[TIME_LEN + 1];
    rs_str_t    dl_str;

    p = errstr;
    last = errstr + RS_MAX_ERROR_STR;

    /* time str */
    rs_get_time(timestr);
    p = rs_cpymem(p, timestr, TIME_LEN);
    *p++ = ' ';

    /* debug level str */
    dl_str = rs_log_level_list[rs_min(level, RS_LOG_LEVEL_MAX)];
    p = rs_cpymem(p, dl_str.data, dl_str.len);

    n = snprintf(p, last - p, "<thread : %lu> ", pthread_self());

    p += n;

    /* log message */
    n = vsnprintf(p, RS_MAX_ERROR_STR - TIME_LEN - dl_str.len, fmt, args);
    p += n;

    /* error msg feed */
    if (err) {
        if(p > last - 50) {
            p = last - 50;
            *p++ = '.';
            *p++ = '.';
            *p++ = '.';
        }

        n = snprintf(p, last - p, " ---- (%d: ", err);
        p += n;

        p = rs_strerror(err, p, last - p);
        if(p < last) {
            *p++ = ')'; 
        }
    }

    /* line feed */
    if (p > last - 1) {
        p = last - 1;
    }

    *p++ = '\n';

    (void) rs_write_fd(fd, errstr, p - errstr);
}

static void rs_get_time(char *buf) 
{
    struct timeval tv; 
    struct tm tm;
    time_t sec;

    gettimeofday(&tv, (void *) 0);
    sec = tv.tv_sec;

    (void) localtime_r(&sec, &tm);
    tm.tm_mon++;
    tm.tm_year += 1900;

    (void) sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year,
            tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min,
            tm.tm_sec);
}
