
#include <rs_config.h>
#include <rs_core.h>

static int rs_init_core_conf(rs_core_info_t *ci);

rs_core_info_t  *rs_core_info = NULL;

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
    rs_conf_t *c;

    c = &(ci->conf);

    if(
            (rs_add_conf_kv(c, "user", &(ci->user), RS_CONF_STR) != RS_OK) 
            ||
            (rs_add_conf_kv(c, "pid", &(ci->pid_path), RS_CONF_STR) != RS_OK) 
            ||
            (rs_add_conf_kv(c, "log", &(ci->log_path), RS_CONF_STR) != RS_OK) 
            ||
            (rs_add_conf_kv(c, "log.level", &rs_log_level, 
                            RS_CONF_UINT32) != RS_OK) 
            ||
            (rs_add_conf_kv(c, "daemon", &(ci->daemon), 
                            RS_CONF_UINT32) != RS_OK)
      )
    {
        rs_log_err(0, "rs_add_conf_kv() failed");
        return RS_ERR;
    }

    /* init core conf */
    if(rs_init_conf(c, rs_conf_path, RS_CORE_MODULE_NAME) != RS_OK) {
        rs_log_err(0, "core conf init failed, %s", rs_conf_path);
        return RS_ERR;
    }

    return RS_OK;
}

void rs_free_core(rs_core_info_t *ci)
{
    if(ci != NULL) {

        if(ci->log_fd != -1) {
            rs_close(ci->log_fd);
        }

        /* free conf */
        rs_free_conf(&(ci->conf));

        if(ci->conf.kv != NULL) {
            free(ci->conf.kv);
        }

        free(ci);
    }
}
