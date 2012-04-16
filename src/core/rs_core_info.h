
#ifndef _RS_CORE_INFO_H_INCLUDED_
#define _RS_CORE_INFO_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>

#define RS_CORE_MODULE_NAME    "core"
#define RS_CORE_CONF_NUM       6 


#define RS_MAX_KEY_LEN       12
#define RS_MAX_VALUE_LEN     PATH_MAX

#define RS_MAX_MODULE_LEN    6

typedef struct {
    char            *user;
    char            cwd[PATH_MAX + 1];

    char            *pid_path;

    uint32_t        daemon;

    char            *log_path;
    int             log_fd;

    sigset_t        sig_set;
    siginfo_t       sig_info;

    rs_conf_t       conf;

} rs_core_info_t;

#define rs_core_info_t_init(ci)                                              \
    rs_memzero(ci->cwd, PATH_MAX + 1);                                       \
    ci->pid_path = NULL;                                                     \
    ci->daemon = 0;                                                          \
    ci->log_path = NULL;                                                     \
    ci->log_fd = -1;                                                         \
    rs_conf_t_init(&(ci->conf))


extern rs_core_info_t   *rs_core_info;

rs_core_info_t *rs_init_core_info(rs_core_info_t *oc);
void rs_free_core(rs_core_info_t *ci);

#endif
