
#include <rs_config.h>
#include <rs_core.h>

rs_buf_t *rs_create_tmpbuf(uint32_t size)
{
    rs_buf_t    *b;
    
    b = malloc(sizeof(rs_buf_t) + size);
    if (b== NULL) {
        rs_log_error(RS_LOG_ERR, rs_errno, "malloc() failed, %u", 
                sizeof(rs_buf_t) + size);
        return NULL;
    }

    b->start = (char *) b + sizeof(rs_buf_t);
    b->size = size;
    b->pos = b->start;
    b->last = b->start;
    b->end = b->start + size;

    return b;
}

int rs_send_tmpbuf(rs_buf_t *b, int fd)
{
    uint32_t    size;
    ssize_t     n;

    if((size = (b->last - b->pos)) == 0) {
        return RS_OK;
    }

    rs_log_debug(RS_DEBUG_TMPBUF, 0, "tmpbuf send size : %u", size);
    while(size > 0) {
        n = rs_write(fd, b->pos, size);
        if(n <= 0) {
            return RS_ERR;
        }

        b->pos += n;
        size = (b->last - b->pos);
    }

    b->pos = b->start;
    b->last = b->start;

    return RS_OK;
}

int rs_recv_tmpbuf(rs_buf_t *b, int fd, void *data, uint32_t size)
{
    uint32_t    l, c;
    ssize_t     n;

    if(size > b->size) {
        rs_log_debug(RS_DEBUG_TMPBUF, 0, "tmpbuf recv size %u, "
                "buf size %u overflow", size, b->size);
        return RS_ERR;
    }

    l = b->last - b->pos;
    if(l >= size) {
        rs_memcpy(data, b->pos, size);
        b->pos += size;
        rs_log_debug(RS_DEBUG_TMPBUF, 0, "tmpbuf recv size : %u", size);
        return RS_OK;
    } else {
        rs_memcpy(data, b->pos, l);
        b->pos = b->start;
        b->last = b->start;
        c = size - l;

        do {
            n = rs_read(fd, b->last, b->size);
            if(n <= 0) {
                return RS_ERR;
            }

            b->last += n;
        } while((b->last - b->pos) < c);

        rs_memcpy((char *) data + l, b->pos, c);
        b->pos += c;
    }

    rs_log_debug(RS_DEBUG_TMPBUF, 0, "tmpbuf recv size : %u", size);
    return RS_OK;
}

uint32_t rs_get_tmpbuf(rs_buf_t *b, void *buf, uint32_t size)
{
    uint32_t ms;

    ms = rs_min((uint32_t) (bi->io_buf->last - bi->io_buf->pos), size);
    if(ms > 0) {
        f = rs_cpymem(buf, b->pos, ms);
        io_buf->pos += ms;
    }

    return ms;
}

ssize_t rs_read_tmpbuf(rs_buf_t *b, int fd) 
{
    ssize_t n;

    n = rs_read(fd, b->start, b->size);
    if(n > 0) {
        b->last = b->start + n;
    }

    return n;
}

void rs_clear_tmpbuf(rs_buf_t *b) 
{
    b->pos = b->start;
    b->last = b->start;
}

void rs_destroy_tmpbuf(rs_buf_t *b)
{
    free(b); 
}

rs_ringbuf_t *rs_create_ringbuf(rs_pool_t *p, uint32_t num)
{
    rs_ringbuf_t        *rb;
    rs_ringbuf_data_t   *d;
    char                *t;
    uint32_t            len, i, id;

    rb = NULL;
    d = NULL;
    t= NULL;

    len =  sizeof(rs_ringbuf_data_t) * num;
    id = rs_palloc_id(p, sizeof(rs_ringbuf_t) + len);
    rb = rs_palloc(p, sizeof(rs_ringbuf_t) + len, id);

    if(rb == NULL) {
        rs_log_error(RS_LOG_ERR, rs_errno, "malloc(%u) failed", 
                len + sizeof(rs_ringbuf_t));
        return NULL;
    }

    rb->id = id;
    rb->pool = p;
    rb->start = (char *) rb + sizeof(rs_ringbuf_t);
    rb->num = num;
    rb->wn = 0;
    rb->rn = 0;

    t = rb->start;

    for(i = 0; i < num; i++) {
        d = (rs_ringbuf_data_t *) t; 
        rs_ringbuf_data_t_init(d);
        t += sizeof(rs_ringbuf_data_t);
    }

    rb->rp = rb->start;
    rb->wp = rb->start;
    rb->end = rb->start + len;

    return rb;
}

int rs_ringbuf_get(rs_ringbuf_t *rb, rs_ringbuf_data_t **data)
{
    rs_ringbuf_data_t *d;

    if(rb->wn - rb->rn == 0) {
        return RS_EMPTY;
    }

    rs_mem_barrier();
    d = (rs_ringbuf_data_t *) rb->rp;

    *data = d;

    return RS_OK;
}

void rs_ringbuf_get_advance(rs_ringbuf_t *rb) 
{
    rb->rp = (rb->rp + sizeof(rs_ringbuf_data_t) == rb->end) ? rb->start : 
        rb->rp + sizeof(rs_ringbuf_data_t);
    rs_mem_barrier();
    rb->rn++;

#if x86_64
    rs_log_debug(RS_DEBUG_RINGBUF, 0, "ringbuf write : %lu, read : %lu", 
            rb->wn, rb->rn);
#elif x86_32
    rs_log_debug(RS_DEBUG_RINGBUF, 0, "ringbuf write : %llu, read : %llu"
            , rb->wn, rb->rn);
#endif
}

int rs_ringbuf_set(rs_ringbuf_t *rb, rs_ringbuf_data_t **data)
{
    rs_ringbuf_data_t *d;

    if(rb->wn - rb->rn == rb->num) {
        return RS_FULL;
    }
    rs_mem_barrier();
    d = (rs_ringbuf_data_t *) rb->wp;

    *data = d;

    return RS_OK;
}

void rs_ringbuf_set_advance(rs_ringbuf_t *rb) 
{
    rb->wp = ((rb->wp + sizeof(rs_ringbuf_data_t)) == rb->end ? rb->start : 
            rb->wp + sizeof(rs_ringbuf_data_t));
    rs_mem_barrier();
    rb->wn++;
#if x86_64
    rs_log_debug(RS_DEBUG_RINGBUF, 0, "ringbuf write : %lu, read : %lu", 
            rb->wn, rb->rn);
#elif x86_32
    rs_log_debug(RS_DEBUG_RINGBUF, 0, "ringbuf write : %llu, read : %llu"
            , rb->wn, rb->rn);
#endif
}

int rs_ringbuf_spin_wait(rs_ringbuf_t *rb, rs_ringbuf_data_t **d)
{
    int err, n, i;

    for (n = 1; n < RS_RINGBUF_SPIN; n <<= 1) {

        for (i = 0; i < n; i++) {
            rs_cpu_pause();
        }

        if((err = rs_ringbuf_get(rb, d)) == RS_OK) {
            return RS_OK;
        }
    }

    return err;
}

void rs_destroy_ringbuf(rs_ringbuf_t *rb)
{
    rs_pfree(rb->pool, rb, rb->id); 
}
