
#include <rs_config.h>
#include <rs_core.h>


int rs_init_ring_buffer2(rs_ring_buffer2_t *rb, uint32_t num) 
{
    void                    *p;
    uint32_t                len, i;

    if(rb == NULL) {
        rs_log_err(0, "rs_init_ring_buffer2() failed, rb is NULL");
        return RS_ERR;
    }

    len =  sizeof(rs_ring_buffer2_data_t) * num;

    rb->num = num;
    rb->wn = 0;
    rb->rn = 0;

    rb->start = malloc(len);

    if(rb->start == NULL) {
        rs_log_err(rs_errno, "malloc() failed, rs_ring_buffer2_data_t * num");
        return RS_ERR;
    }

    p = rb->start;

    for(i = 0; i < num; i++) {
       d = (rs_ring_buffer2_data_t *) p; 
       rs_ring_buffer2_data_t_init(d);
       p = d->data + sizeof(rs_ring_buffer2_data_t);
    }

    rb->rp = rb->rb;
    rb->wp = rb->rb;
    rb->end = rb->rb + len;

    return RS_OK;
}

int rs_get_ring_buffer2(rs_ring_buffer2_t *rb, rs_ring_buffer2_data_t **data)
{
    rs_ring_buffer2_data_t *d;

    if(r == NULL) {
        rs_log_err(0, "rs_get_ring_buffer2() failed, pointer r is NULL");
        return RS_ERR;
    } 

    if(rb->wn - rb->rn == 0) {
        return RS_EMPTY;
    }

    rs_log_debug(0, "ring buffer write : %u, read : %u", rb->wn, rb->rn);

    d = (rs_ring_buffer2_data_t *) rb->rp;

    if(d == NULL) {
        rs_log_err(0, "rs_get_ring_buffer2() failed, data is NULL");
        return RS_ERR;
    }

    *data = d;

    return RS_OK;
}

void rs_get_ring_buffer2_advance(rs_ring_buffer2_t *rb) 
{
    rb->rp = (rb->rp + sizeof(rs_ring_buffer2_data_t)) == rb->end ? 
        rb->start : rb->rp + sizeof(rs_ring_buffer2_data_t);
    rb->rn++;
}

int rs_set_ring_buffer2(rs_ring_buffer2_t *rb, rs_ring_buffer2_data_t **data) 
{
    rs_ring_buffer2_data_t *d;

    if(rb == NULL) {
        rs_log_err(0, "rs_set_ring_buffer2() failed, rb is NULL");
        return RS_ERR;
    } 

    if(rb->wn - rb->rn == rb->num) {
        return RS_FULL;
    }

    rs_log_debug(0, "ring buffer write : %u, read : %u", rb->wn, rb->rn);

    d = (rs_ring_buffer2_data_t *) rb->wp;
    if(d == NULL) {
        rs_log_err(0, "rs_set_ring_buffer2() failed, data is NULL");
        return RS_ERR;
    }

    *data = d;

    return RS_OK;
}

void rs_set_ring_buffer2_advance(rs_ring_buffer2_t *rb) 
{
    rb->wp = (rb->wp + sizeof(rs_ring_buffer2_data_t)) == r->end ? 
        r->rb : r->wp + sizeof(rs_ring_buffer2_data_t);

    r->wn++;
}
