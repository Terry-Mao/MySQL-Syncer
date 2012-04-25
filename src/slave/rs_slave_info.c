
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

static int rs_init_slave_conf(rs_slave_info_t *mi);
static int rs_parse_slave_info(rs_slave_info_t *s);

rs_slave_info_t     *rs_slave_info = NULL;

rs_slave_info_t *rs_init_slave_info(rs_slave_info_t *os) 
{
    int                 nr, ni, nrb, err;
    rs_slave_info_t     *si;

    si = (rs_slave_info_t *) malloc(sizeof(rs_slave_info_t));
    nr = 1;
    ni = 1;
    nrb = 1;

    if(si == NULL) {
        rs_log_err(rs_errno, "malloc() failed, rs_slave_info_t");
        goto free;
    }

    rs_slave_info_t_init(si);

    /* init conf */
    if(rs_init_slave_conf(si) != RS_OK) {
        rs_log_err(0, "slave conf init failed");
        goto free;
    }

    /* slab info */
    if(si->slab_factor <= 1) {
        si->slab_factor = RS_SLAB_GROW_FACTOR; 
        rs_log_info("slab_facto use default value, %u", si->slab_factor);
    }

    if(si->slab_init_size <= 0) {
        si->slab_init_size = RS_SLAB_INIT_SIZE;
        rs_log_info("slab_init_size use default value, %u", 
                si->slab_init_size);
    }

    if(si->slab_mem_size <= 5 * 1024 * 1024) {
        si->slab_mem_size = RS_SLAB_MEM_SIZE; 
        rs_log_info("slab_mem_size use default value, %u", si->slab_mem_size);
    }

    /* ring buf num */
    if(si->ring_buf_num <= 0) {
        si->ring_buf_num = RS_RING_BUFFER_NUM;
        rs_log_info("ring_buf_num use default value, %u", si->ring_buf_num);
    }

#if 0
    /* init thread attr */
    if((err = pthread_attr_init(&(si->thread_attr))) != 0) {
        rs_log_err(err, "pthread_attr_init() failed, thread_attr");
        goto free;
    }

    /* set thread detached */
    if((err = pthread_attr_setdetachstate(&(si->thread_attr), 
                    PTHREAD_CREATE_DETACHED)) != 0) 
    {
        rs_log_err(err, "pthread_attr_setdetachstate() failed, DETACHED");
        goto free;
    }
#endif

    /* slave info */
    si->info_fd = open(si->slave_info, O_CREAT | O_RDWR, 00666);

    if(si->info_fd == -1) {
        rs_log_err(rs_errno, "open(\"%s\") failed", si->slave_info);
        goto free;
    }

    /* parse slave.info */
    if(rs_parse_slave_info(si) != RS_OK) {
        goto free;
    }

    if(os != NULL) {
        /* dump file or pos */
        nrb = (os->dump_pos != si->dump_pos || 
                rs_strcmp(os->dump_file, si->dump_file) != 0) || 
            (os->ring_buf_num != si->ring_buf_num);

        /* redis addr or port */
        nr = (os->redis_port != si->redis_port || 
                rs_strcmp(os->redis_addr, si->redis_addr) != 0) | nrb;

        /* listen addr or port */
        ni = (os->listen_port != si->listen_port || 
                rs_strcmp(os->listen_addr, si->listen_addr) != 0) 
            | nrb;

        if(ni) {

            if(os->io_thread != 0) {

                rs_log_info("start exiting io thread");

                if((err = pthread_cancel(os->io_thread)) != 0) {
                    rs_log_err(err, "pthread_cancel() failed, io_thread");
                }
            }
        } else {
            /* reuse io_thread */
            si->io_thread = os->io_thread;
            si->svr_fd = os->svr_fd;
            os->svr_fd = -1; /* NOTICE: SKIP close svr_fd */
            os->io_thread = 0; /* NOTICE: SKIP cancel io_thread */
        }

        if(nr) {

            if(os->redis_thread != 0) {
                rs_log_info("start exiting redis thread");

                if((err = pthread_cancel(os->redis_thread)) != 0) {
                    rs_log_err(err, "pthread_cancel() failed, redis_thread");
                }
            }
        } else {
            /* reuse redis thread */ 
            si->redis_thread = os->redis_thread;
            si->c = os->c;
            si->redis_thread = 0; /* NOTICE: SKIP cancel redis_thread */
            si->c = NULL; /* NOTICE: SKIP close redisContext */
        }

        if(!nrb) {
            si->slab = os->slab;
            si->ring_buf = os->ring_buf;

            os->slab = NULL; /* NOTICE : SKIP free slab */
            os->ring_buf = NULL; /* NOTICE : SKIP free ring_buf */
        }
    }

    if(nrb) {

        /* init slab */
        si->slab = (rs_slab_t *) malloc(sizeof(rs_slab_t));

        if(si->slab == NULL) {
            rs_log_err(rs_errno, "malloc() failed, rs_slab_t");
            goto free;
        }

        if(rs_init_slab(si->slab, NULL, si->slab_init_size, si->slab_factor
                    , si->slab_mem_size, RS_SLAB_PREALLOC) != RS_OK) 
        {
            goto free;
        }

        /* init ring buf */
        si->ring_buf = (rs_ring_buffer2_t *) 
            malloc(sizeof(rs_ring_buffer2_t));

        if(si->ring_buf == NULL) {
            rs_log_err(rs_errno, "malloc() failed, rs_ring_buffer_t");
            goto free;
        }

        /* init ring buffer for io and redis thread */
        if(rs_init_ring_buffer2(si->ring_buf, RS_RING_BUFFER_NUM) != RS_OK) {
            goto free;
        }
    }

    if(ni) {
        rs_log_info("start io thread");

        /* init io thread */
        if((err = pthread_create(&(si->io_thread), NULL, 
                        rs_start_io_thread, (void *) si)) != 0) 
        {
            rs_log_err(err, "pthread_create() failed, io_thread");
            goto free;
        }
    }

    if(nr) {
        rs_log_info("start redis thread");

        /* init redis thread */
        if((err = pthread_create(&(si->redis_thread), NULL, 
                        rs_start_redis_thread, (void *) si)) != 0) 
        {
            rs_log_err(err, "pthread_create() failed redis_thread");
            goto free;
        }
    }

    return si;

free:

    /* free new slave info */
    rs_free_slave(si);

    return NULL;
}

static int rs_init_slave_conf(rs_slave_info_t *si)
{
    rs_conf_t *c;

    c = &(si->conf);

    if(
            (rs_add_conf_kv(c, "listen.addr", &(si->listen_addr), 
                            RS_CONF_STR) != RS_OK)
            ||
            (rs_add_conf_kv(c, "listen.port", &(si->listen_port),
                            RS_CONF_INT32) != RS_OK)
            ||
            (rs_add_conf_kv(c, "slave.info", &(si->slave_info), 
                            RS_CONF_STR) != RS_OK)
            || 
            (rs_add_conf_kv(c, "redis.addr", &(si->redis_addr), 
                            RS_CONF_STR) != RS_OK)
            ||
            (rs_add_conf_kv(c, "redis.port", &(si->redis_port), 
                            RS_CONF_INT32) != RS_OK)
            ||
            (rs_add_conf_kv(c, "slab.factor", &(si->slab_factor), 
                            RS_CONF_DOUBLE) != RS_OK)
            ||
            (rs_add_conf_kv(c, "slab.memsize", &(si->slab_mem_size), 
                            RS_CONF_UINT32) != RS_OK)
            ||
            (rs_add_conf_kv(c, "slab.initsize", &(si->slab_init_size), 
                            RS_CONF_UINT32) != RS_OK)
            ||
            (rs_add_conf_kv(c, "ringbuf.num", &(si->ring_buf_num), 
                            RS_CONF_UINT32) != RS_OK)
            ) 
            {
                rs_log_err(0, "rs_add_conf_kv() failed"); 
                return RS_ERR;
            }

    /* init master conf */
    if(rs_init_conf(c, rs_conf_path, RS_SLAVE_MODULE_NAME) != RS_OK) {
        rs_log_err(0, "slave conf init failed");
        return RS_ERR;
    }

    return RS_OK;
}

static int rs_parse_slave_info(rs_slave_info_t *si) 
{
    char    *p, buf[RS_SLAVE_INFO_STR_LEN];
    ssize_t n;

    /* binlog path, binlog pos  */
    p = buf;

    n = rs_read(si->info_fd, buf, RS_SLAVE_INFO_STR_LEN);

    if(n < 0) {
        rs_log_debug(0, "rs_read() failed, rs_parse_slave_info");
        return RS_ERR;
    } else if (n > 0) { 
        buf[n] = '\0';
        p = rs_ncp_str_till(si->dump_file, p, ',', PATH_MAX);
        si->dump_pos = rs_str_to_uint32(p);
    } else {
        /* if a new slave.info file */
        rs_log_err(0, "slave.info is empty, "
                "must specified format like \"binlog_file,binlog_pos\"");
        return RS_ERR;
    }

    rs_log_info("parse slave info binlog file = %s, pos = %u",
            si->dump_file, si->dump_pos);

    return RS_OK;
}

/*
 *  rs_flush_slave_info
 *  @s:rs_slave_info_s struct
 *
 *  Flush slave into to disk, format like rsylog_path,rsylog_pos
 *
 *  On success, RS_OK is returned. On error, RS_ERR is returned
 */
int rs_flush_slave_info(rs_slave_info_t *si, char *buf, size_t len) 
{
    ssize_t n;

    rs_log_info("flush slave.info buf = %*.*s", len, len, buf);

    if(lseek(si->info_fd, 0, SEEK_SET) == -1) {
        rs_log_err(rs_errno, "lseek() failed, %s", si->slave_info);
        return RS_ERR;
    }

    n = rs_write(si->info_fd, buf, len);

    if((size_t) n != len) {
        return RS_ERR;
    } 

    if(fdatasync(si->info_fd) != 0) {
        rs_log_err(rs_errno, "fdatasync() failed, %s", si->slave_info);
        return RS_ERR;
    }

    /* truncate other remained file bytes */
    if(truncate(si->slave_info, len) != 0) {
        rs_log_err(rs_errno, "truncate() failed, %s:%u", si->slave_info, len);
        return RS_ERR;
    }

    return RS_OK; 
}
