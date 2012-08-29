
#include <rs_config.h>
#include <rs_core.h>

static rs_str_t *rs_sys_errlist = NULL;
static rs_str_t rs_unknown_error = rs_string("Unknown error");

char *rs_strerror(rs_err_t err, char *errstr, size_t size) 
{
    rs_str_t *msg;

    msg = ((uint32_t) err < RS_SYS_NERR) ? &rs_sys_errlist[err]
        : &rs_unknown_error; // not modify

    size = rs_min(size, msg->len);

    return rs_cpymem(errstr, msg->data, size);
}

int rs_init_strerror() 
{
    char        *msg, *p;
    size_t      len;
    rs_err_t    err;


    len = RS_SYS_NERR * sizeof(rs_str_t);

    rs_sys_errlist = malloc(len);

    if (rs_sys_errlist == NULL) {
        goto failed;
    }

    for (err = 0; err < RS_SYS_NERR; err++) {

        msg = NULL;
        p = NULL;

        msg = strerror(err);
        len = rs_strlen(msg);

        p = malloc(len);

        if(p == NULL) {
            goto failed;
        }

        rs_memcpy(p, msg, len);
        rs_sys_errlist[err].len = len;
        rs_sys_errlist[err].data = p;
    }

    return RS_OK;

failed:

    err = rs_errno;
    rs_log_stderr(0, "malloc(%uz) failed (%d: %s)", len, err, strerror(err));

    return RS_ERR;
}

void rs_free_strerr()
{
    rs_err_t    err;

    if(rs_sys_errlist != NULL) {
        for (err = 0; err < RS_SYS_NERR; err++) {
            if(rs_sys_errlist[err].len != 0) {
                free(rs_sys_errlist[err].data);
            }
        }

        free(rs_sys_errlist);
    }    
}
