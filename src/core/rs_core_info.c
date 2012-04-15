
#include <rs_config.h>
#include <rs_core.h>

static int rs_init_core_conf(rs_core_info_t *ci);

rs_core_info_t *rs_init_core_info(rs_core_info_t *oc) 
{
    int             nl, nd;
    char            *p;
    rs_core_info_t  *ci;  

    p = NULL;
    nl = 1;
    nd = 1;
    ci = (rs_core_info_t *) malloc(sizeof(rs_core_info_t));

    if(ci == NULL) {
        rs_log_err(rs_errno, "malloc() failed, rs_core_info_t");
        goto free;
    }

    rs_core_info_t_init(ci);

    /* init core conf */
    if(rs_init_core_conf(ci) != RS_OK) {
        goto free;
    }

    if(oc != NULL) {

        if(!(nl = (rs_strcmp(oc->log_path, ci->log_path) != 0))) {
            ci->log_fd = oc->log_fd;  
            oc->log_fd = -1;
        }

        nd = (oc->daemon != ci->daemon);

        if(strcmp(oc->pid_path, ci->pid_path) != 0) {
            rs_delete_pidfile(oc->pid_path);
        }
    }

    /* init working directory */
    if(getcwd(ci->cwd, PATH_MAX + 1) == NULL) {
        rs_log_stderr(rs_errno, "getcwd() failed");
        goto free;
    }


    /* init uid & gid */
    if(ci->user != NULL) {
        p = rs_strchr(ci->user, ' '); 

        if(p != NULL) {
            *p = '\0';
            p++;
            if(rs_init_gid(p) != RS_OK) {
                goto free;
            }
        }

        if(rs_init_uid(ci->user) != RS_OK) {
            goto free;
        }
    }

    /* init log */
    if(nl) {
        if((ci->log_fd = rs_log_init(ci->log_path, ci->cwd, 
                        O_CREAT| O_RDWR| O_APPEND)) == -1) 
        {
            rs_log_stderr(rs_errno, "open(\"%s\") failed", ci->log_path);
            goto free;
        }
    } 

    /* init signals */
    if(rs_init_signals(&(ci->sig_set)) != RS_OK) {
        goto free;
    }

    if(nd) {
        /* daemon */
        if(ci->daemon == 1) {
            if(rs_init_daemon() != RS_OK) {
                goto free;
            }
        }
    }

    rs_pid = getpid();

    /* create pid file */
    if(rs_create_pidfile(ci->pid_path) != RS_OK) {
        goto free;
    }


    return ci;

free :

    rs_free_core(ci);

    return NULL;
}

static int rs_init_core_conf(rs_core_info_t *ci)
{
    rs_conf_kv_t *core_conf;

    core_conf = (rs_conf_kv_t *) malloc(sizeof(rs_conf_kv_t) * 
            RS_CORE_CONF_NUM);

    if(core_conf == NULL) {
        rs_log_err(rs_errno, "malloc() failed, rs_conf_kv_t * core_num");
        return RS_ERR;
    }

    ci->conf = core_conf;

    rs_str_set(&(core_conf[0].k), "user");
    rs_conf_v_set(&(core_conf[0].v), ci->user, RS_CONF_STR);

    rs_str_set(&(core_conf[1].k), "pid");
    rs_conf_v_set(&(core_conf[1].v), ci->pid_path, RS_CONF_STR);

    rs_str_set(&(core_conf[2].k), "log");
    rs_conf_v_set(&(core_conf[2].v), ci->log_path, RS_CONF_STR);

    rs_str_set(&(core_conf[3].k), "log.level");
    rs_conf_v_set(&(core_conf[3].v), rs_log_level, RS_CONF_UINT32);

    rs_str_set(&(core_conf[4].k), "daemon");
    rs_conf_v_set(&(core_conf[4].v), ci->daemon, RS_CONF_UINT32);

    rs_str_set(&(core_conf[5].k), NULL);
    rs_conf_v_set(&(core_conf[5].v), "", RS_CONF_NULL);

    /* init core conf */
    if(rs_init_conf(rs_conf_path, RS_CORE_MODULE_NAME, core_conf) != RS_OK) {
        rs_log_err(0, "core conf init failed");
        return RS_ERR;
    }

    return RS_OK;
}

void rs_free_core(rs_core_info_t *ci)
{
    ci = (ci == NULL ? rs_core_info : ci);

    if(ci != NULL) {

        if(ci->log_fd != -1) {
            rs_close(ci->log_fd);
        }

        if(ci->conf != NULL) {
            rs_free_conf(ci->conf);
            free(ci->conf);
        }

        free(ci);
    }
}
