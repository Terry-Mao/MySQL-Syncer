
#include <rs_config.h>
#include <rs_core.h>
#include <rs.h>

static int rs_get_options(int argc, char *const *argv);
static int rs_init_core_conf(rs_core_info_t *ci);
static rs_core_info_t *rs_init_core_info(rs_core_info_t *oc);
static void rs_free_core(rs_core_info_t *ci);

rs_core_info_t  *rs_core_info = NULL;
pid_t           rs_pid;
char            *rs_conf_path = NULL;
uint32_t        rs_log_level = RS_LOG_LEVEL_DEBUG;

static int      rs_show_help = 0;


int main(int argc, char * const *argv) 
{
    rs_core_info_t  *ci, *oc;

    /* init rs_errno */
    if(rs_strerror_init() != RS_OK) {
        return 1;
    }

    /* init argv */
    if(rs_get_options(argc, argv) != RS_OK) {
        return 1;
    }

    if(rs_show_help == 1) {
        printf( 
                "Usage: redis_[master|slave] [-?h] [-c filename]\n"
                "Options:\n"
                "   -?, -h          : this help\n"
                "   -c filename     : set configure file\n");
        return 0;
    }

    /* init core info */
    if((ci = rs_init_core_info(NULL)) == NULL) {
        return 1;
    }

    rs_core_info = ci;

    rs_log_info("main thread opened");

#if MASTER
        rs_init_master();
#elif SLAVE
        rs_init_slave();
#endif

    /* wait signal */
    for( ;; ) {

        if(sigwaitinfo(&(ci->sig_set), &(ci->sig_info)) == -1) {
            rs_log_err(rs_errno, "sigwaitinfo() failed");
            return 1;
        }

        rs_sig_handle(ci->sig_info.si_signo);

        if(rs_quit) {

        rs_log_info("redisync exiting");

#if MASTER
            rs_free_master(NULL);
#elif SLAVE
            rs_free_slave(NULL);
#endif
            break;        
        } else if(rs_reload) {

            /* init core info */
            if((ci = rs_init_core_info(rs_core_info)) == NULL) {
                rs_log_err(0, "redisync reload failed");
                rs_reload = 0;
                continue;
            }

            oc = rs_core_info;
            rs_core_info = ci;

#if MASTER
            rs_init_master();
#elif SLAVE
            rs_init_slave();
#endif

            if(oc != NULL) {
                rs_free_core(oc);
            }

            rs_reload = 0;

            continue;
        }
    }

    if(ci->pid_path != NULL) {
        rs_delete_pidfile(ci->pid_path);
    }


    rs_free_core(ci);

    rs_log_info("exit success!");

    return 0;
}

/*
 * DESCRIPTION 
 *    Process the cmd args.
 *    -c set the configure file location
 *    -[?h] show the help information
 *
 * RETURN VALUE
 *    On success RS_OK is returned, On error RS_ERR is returned.
 */
static int rs_get_options(int argc, char *const *argv) 
{
    char   *p;
    int     i;

    for (i = 1; i < argc; i++) {

        p = argv[i];

        if (*p++ != '-') {
            rs_log_stderr(0, "invalid option: \"%s\"", argv[i]);
            return RS_ERR;
        }

        while (*p) {

            switch (*p++) {
            case '?':
            case 'h':
                rs_show_help = 1;
                break;

            case 'c':
                if (*p) {
                    rs_conf_path = p;
                    goto next;
                }

                if (argv[++i]) {
                    rs_conf_path = argv[i];
                    goto next;
                }

                rs_log_stderr(0, "option \"-c\" requires config file");
                return RS_ERR;

            default:
                rs_log_stderr(0, "invalid option: \"%c\"", *(p - 1));
                return RS_ERR;
            }
        }

next:

        continue;
    }

    return RS_OK;
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

static rs_core_info_t *rs_init_core_info(rs_core_info_t *oc) 
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

static void rs_free_core(rs_core_info_t *ci)
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
