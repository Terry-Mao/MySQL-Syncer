
#include <rs_config.h>
#include <rs_core.h>

static int rs_init_core_conf(rs_core_info_t *ci);

rs_core_info_t  *rs_core_info = NULL;

rs_core_info_t *rs_init_core_info(rs_core_info_t *oc) 
{
    int             nl, nd, fd, id;
    char            *t;
    rs_core_info_t  *ci;  
    rs_pool_t       *p;

    t = NULL;
    nl = 1;
    nd = 1;


    p = rs_create_pool(200, 1024 * 1024 * 10, rs_pagesize, RS_POOL_CLASS_IDX, 
            1.5, RS_POOL_PREALLOC);
    id = rs_palloc_id(p, sizeof(rs_core_info_t) + sizeof(rs_conf_t));

    ci = (rs_core_info_t *) rs_palloc(p, 
            sizeof(rs_core_info_t) + sizeof(rs_conf_t), id);

    if(ci == NULL) {
        goto free;
    }

    ci->cf = (rs_conf_t *) ((char *) ci + sizeof(rs_core_info_t));
    ci->pool = p;
    ci->id = id;

    rs_core_info_t_init(ci);

    /* init core conf */
    if(rs_init_core_conf(ci) != RS_OK) {
        goto free;
    }

    /* has old core_info */
    if(oc != NULL) {

        nl = rs_strcmp(oc->log_path, ci->log_path) != 0;
        nd = (oc->daemon != ci->daemon);

        if(strcmp(oc->pid_path, ci->pid_path) != 0) {
            rs_delete_pidfile(oc->pid_path);
        }
    }

    /* init working directory */
    if(rs_chdir(ci->cwd) != RS_OK) {
        rs_log_err(rs_errno, "chdir() failed");
        goto free;
    }

    /* init uid & gid */
    if(ci->user != NULL) {
        t = rs_strchr(ci->user, ' '); 

        if(t != NULL) {
            *t = '\0';
            t++;
            if(rs_init_gid(t) != RS_OK) {
                goto free;
            }
        }

        if(rs_init_uid(ci->user) != RS_OK) {
            goto free;
        }
    }

    /* init log */
    if(nl) {
        if((fd = rs_log_init(ci->log_path, O_CREAT| O_RDWR| O_APPEND)) == -1) {
            rs_log_stderr(rs_errno, "open(\"%s\") failed", ci->log_path);
            goto free;
        }

        rs_log_fd = fd;
    }

    /* init signals */
    if(rs_init_signals(&(ci->sig_set)) != RS_OK) {
        goto free;
    }

    if(nd) {
        /* daemon */
        if(ci->daemon == 1) {
            if(rs_init_daemon(ci) != RS_OK) {
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
    if(rs_conf_register(ci->cf, "cwd", &(ci->cwd), RS_CONF_STR) != RS_OK) {
        return RS_ERR;    
    }

    if(rs_conf_register(ci->cf, "user", &(ci->user), RS_CONF_STR) != RS_OK) {
        return RS_ERR;    
    }

    if(rs_conf_register(ci->cf, "pid", &(ci->pid_path), RS_CONF_STR) != RS_OK) {
        return RS_ERR;    
    }

    if(rs_conf_register(ci->cf, "log", &(ci->log_path), RS_CONF_STR) != RS_OK) {
        return RS_ERR;    
    }

    if(rs_conf_register(ci->cf, "log.level", &rs_log_level, RS_CONF_UINT32) 
            != RS_OK) 
    {
        return RS_ERR;    
    }

    if(rs_conf_register(ci->cf, "daemon", &(ci->daemon), RS_CONF_UINT32) 
            != RS_OK) 
    {
        return RS_ERR;    
    }

    /* init core conf */
    return rs_init_conf(ci->cf, rs_conf_path, RS_CORE_MODULE_NAME);
}

void rs_free_core(rs_core_info_t *ci)
{
    /* free conf */
    rs_destroy_conf(ci->cf);
    rs_destroy_pool(ci->pool);
}
