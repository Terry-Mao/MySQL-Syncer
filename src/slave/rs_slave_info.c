
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>


static int rs_init_slave_conf(rs_slave_info_t *mi);

rs_slave_info_t     *rs_slave_info = NULL;

rs_slave_info_t *rs_init_slave_info(rs_slave_info_t *os) 
{
    int                 nr, ni, nrb, err, id;
    ssize_t             n;
    rs_slave_info_t     *si;
    rs_pool_t           *p;

    nr = 1;
    ni = 1;
    nrb = 1;

    p = rs_create_pool(200, 1024 * 1024 * 10, rs_pagesize, RS_POOL_CLASS_IDX, 
            1.5, RS_POOL_PREALLOC);

    if(p == NULL) {
        return NULL;
    }

    id = rs_palloc_id(p, sizeof(rs_slave_info_t));
    si = rs_palloc(p, sizeof(rs_slave_info_t), id);

    if(si == NULL) {
        rs_destroy_pool(p);
        return NULL;
    }

    rs_slave_info_t_init(si);

    si->pool = p;
    si->id = id;
    si->cf = rs_create_conf(p, RS_SLAVE_CONF_NUM);

    if(si->cf == NULL) {
        goto free;
    }

    /* init conf */
    if(rs_init_slave_conf(si) != RS_OK) {
        goto free;
    }

    /* recreate pool */
    p = rs_create_pool(si->pool_initsize, si->pool_memsize, rs_pagesize, 
            RS_POOL_CLASS_IDX, si->pool_factor, RS_POOL_PREALLOC);

    if(p == NULL) {
        goto free; 
    }

    rs_free_slave(si);

    si = rs_palloc(p, sizeof(rs_slave_info_t), id);

    if(si == NULL) {
        rs_destroy_pool(p);
        return NULL;
    }

    rs_slave_info_t_init(si);

    si->pool = p;
    si->id = id;
    si->cf = rs_create_conf(p, RS_SLAVE_CONF_NUM);

    if(si->cf == NULL) {
        goto free;
    }

    /* init conf */
    if(rs_init_slave_conf(si) != RS_OK) {
        goto free;
    }

    if(os != NULL) {
        // TODO 
    }

    if(nrb) {
        /* init ring buf */
        si->ringbuf = rs_create_ringbuf(p, si->ringbuf_num);

        if(si->ringbuf == NULL) {
            goto free;
        }
    }

    /* init packet buf */
    si->recv_buf = rs_create_tmpbuf(si->recvbuf_size);

    if(si->recv_buf == NULL) {
        goto free;
    }

    /* register tables */
    if(rs_register_tables(si) != RS_OK) {
        goto free;
    }

    /* slave info */
    si->info_fd = open(si->slave_info, O_CREAT | O_RDWR, 00666);

    if(si->info_fd == -1) {
        rs_log_err(rs_errno, "open(\"%s\") failed", si->slave_info);
        goto free;
    }

    n = rs_read(si->info_fd, si->dump_info, RS_SLAVE_INFO_STR_LEN);

    if(n <= 0) {
        goto free;
    } else if (n > 0) { 
        si->dump_info[n] = '\0';
    }

    if(ni) {
        rs_log_info("start io thread");

        /* init io thread */
        if((err = pthread_create(&(si->io_thread), NULL, 
                        rs_start_io_thread, (void *) si)) != 0) 
        {
            rs_log_err(err, "pthread_create() failed");
            goto free;
        }
    }

    if(nr) {
        rs_log_info("start redis thread");

        /* init redis thread */
        if((err = pthread_create(&(si->redis_thread), NULL, 
                        rs_start_redis_thread, (void *) si)) != 0) 
        {
            rs_log_err(err, "pthread_create() failed");
            goto free;
        }
    }

    return si;

free:

    rs_free_slave(si);
    return NULL;
}

static int rs_init_slave_conf(rs_slave_info_t *si)
{
    if(rs_conf_register(si->cf, "listen.addr", &(si->listen_addr), 
                RS_CONF_STR) != RS_OK) 
    {
        return RS_ERR;
    }
    
    if(rs_conf_register(si->cf, "listen.port", &(si->listen_port),
                RS_CONF_INT32) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(si->cf, "slave.info", &(si->slave_info), 
                RS_CONF_STR) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(si->cf, "redis.addr", &(si->redis_addr), 
                RS_CONF_STR) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(si->cf, "redis.port", &(si->redis_port), 
                RS_CONF_INT32) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(si->cf, "pool.factor", &(si->pool_factor), 
                RS_CONF_DOUBLE) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(si->cf, "pool.memsize", &(si->pool_memsize), 
                RS_CONF_UINT32) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(si->cf, "pool.initsize", &(si->pool_initsize), 
                RS_CONF_UINT32) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(si->cf, "ringbuf.num", &(si->ringbuf_num), 
                RS_CONF_UINT32) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(si->cf, "binlog.save", &(si->binlog_save), 
                RS_CONF_UINT32) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(si->cf, "filter.tables", &(si->filter_tables), 
                RS_CONF_STR) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(si->cf, "recvbuf.size", &(si->recvbuf_size), 
                RS_CONF_UINT32) != RS_OK)
    {
        return RS_ERR;
    }

    if(rs_conf_register(si->cf, "binlog.savesec", &(si->binlog_savesec), 
                RS_CONF_UINT32) != RS_OK)
    {
        return RS_ERR;
    }

    /* init master conf */
    return rs_init_conf(si->cf, rs_conf_path, RS_SLAVE_MODULE_NAME);
}
