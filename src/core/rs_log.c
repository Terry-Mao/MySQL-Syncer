

#include <rs_config.h>
#include <rs_core.h>

static void rs_get_time(char *buf);

static rs_str_t rs_log_levels[] = {
    rs_string(RS_LOG_ERR_STR),
    rs_string(RS_LOG_INFO_STR),
    rs_string(RS_LOG_DEBUG_STR)
};

static rs_str_t rs_debug_levels[] = {
    rs_string(RS_DEBUG_ALLOC_STR),
    rs_string(RS_DEBUG_HASH_STR),
    rs_string(RS_DEBUG_TMPBUF_STR),
    rs_string(RS_DEBUG_RINGBUF_STR),
    rs_string(RS_DEBUG_BINLOG_STR),
    rs_null_string
};


char        *rs_log_path = "./rs.log";
int         rs_log_fd = STDOUT_FILENO;

uint32_t    rs_log_level = RS_LOG_INFO;
uint32_t    rs_debug_level = 0;


static void rs_log_base(rs_err_t err, int fd, uint32_t level, const char *fmt, 
        va_list args);

int rs_log_init(char *name, int flags) 
{
    if(name == NULL) {
        name = rs_log_path;
    }

    return open(name, flags, 00644);
}

int rs_log_set_levels(char *debug_level)
{
    int         i;
    char        *p;
    rs_str_t    t;

    for(p = debug_level; p != NULL; p = rs_strchr(p, '|')) {

        i = 0;
        for(t = rs_debug_levels[i]; t.len > 0 && t.data != NULL; 
                t = rs_debug_levels[++i])
        {
            if(rs_strncmp(t.data, p, t.len) == 0) {
                rs_debug_level |= (RS_DEBUG_FIRST << i);
                break;
            }
        }

        if(t.len == 0 && t.data == NULL) {
            rs_log_error(RS_LOG_ERR, 0, "invalid debug level, %s", p);
            return RS_ERR;
        }
    }

    return RS_OK;
}

void rs_log_error(uint32_t level, rs_err_t err, const char *fmt, ...) 
{
    va_list args;

    va_start(args, fmt);
    rs_log_base(err, rs_log_fd, level, fmt, args);
    va_end(args);
}

void rs_log_debug(uint32_t level, rs_err_t err, const char *fmt, ...)
{
    if(!(rs_debug_level & level)) {
        return;
    }

    va_list args;

    va_start(args, fmt);
    rs_log_base(err, rs_log_fd, RS_LOG_DEBUG, fmt, args);
    va_end(args);
}

void rs_log_stderr(rs_err_t err, const char *fmt, ...) 
{
    va_list args;

    va_start(args, fmt);
    rs_log_base(err, STDERR_FILENO, RS_LOG_ERR, fmt, args);
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
    dl_str = rs_log_levels[rs_min(level, RS_LOG_MAX)];
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
