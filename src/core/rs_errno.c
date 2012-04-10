
#include <rs_config.h>
#include <rs_core.h>

static char **rs_sys_errlist;
static char *rs_unknown_error = "Unknown error";

char *rs_strerror(rs_err_t err, char *errstr, size_t size) 
{
    char *msg;

    msg = ((uint32_t) err < RS_SYS_NERR) ? rs_sys_errlist[err]
        : rs_unknown_error; // not modify

    size = rs_min(size, rs_strlen(msg));

    return rs_cpymem(errstr, msg, size);
}

int rs_strerror_init() 
{
    char        *msg, *p;
    size_t      len;
    rs_err_t    err;


    len = RS_SYS_NERR * sizeof(char *);

    rs_sys_errlist = malloc(len);

    if (rs_sys_errlist == NULL) {
        goto failed;
    }

    for (err = 0; err < RS_SYS_NERR; err++) {

        msg = strerror(err);
        len = rs_strlen(msg);

        p = malloc(len);
        if (p == NULL) {
            goto failed;
        }

        rs_memcpy(p, msg, len);
        rs_sys_errlist[err] = p;
    }

    return RS_OK;

failed:

    err = rs_errno;
    rs_log_stderr(0, "malloc(%uz) failed (%d: %s)", len, err, strerror(err));

    return RS_ERR;
}
