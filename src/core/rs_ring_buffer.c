
#include <rs_config.h>
#include <rs_core.h>


int rs_init_ring_buffer(rs_ring_buffer_t *r, uint32_t size, uint32_t num) 
{
    char                    *p;
    uint32_t                len, i;
    rs_ring_buffer_data_t   *d;

    len = (sizeof(rs_ring_buffer_data_t) + size) * num;

    if(r == NULL) {
        rs_log_err(0, "rs_init_ring_buffer() failed, pointer r is null");
        return RS_ERR;
    }

    r->size = size;
    r->num = num;
    r->wn = 0;
    r->rn = 0;

    r->rb = malloc(len);

    if(r->rb == NULL) {
        rs_log_err(rs_errno, "malloc() failed, rs_ring_buffer_data_t");
        return RS_ERR;
    }

    p = (char *) r->rb;

    //01234 5678
    for(i = 0; i < num; i++) {
       d = (rs_ring_buffer_data_t *) p; 
       d->len = 0;
       d->data = p + sizeof(rs_ring_buffer_data_t);
       p = d->data + r->size;
    }

    r->rp = (char *) r->rb;
    r->wp = (char *) r->rb;
    r->last = (char *) r->rb + len - 1;

    return RS_OK;
}


int rs_get_ring_buffer(rs_ring_buffer_t *r, rs_ring_buffer_data_t **dst) 
{
    rs_ring_buffer_data_t *d;

    if(r == NULL) {
        rs_log_err(0, "rs_get_ring_buffer() failed, pointer r is null");
        return RS_ERR;
    } 

    // rs_log_debug(0, "ring buffer write num : %u, read num : %u", r->wn, r->rn);

    if(r->wn - r->rn == 0) {
        return RS_EMPTY;
    }

    d = (rs_ring_buffer_data_t *) r->rp;

    if(d == NULL) {
        rs_log_err(0, "rs_get_ring_buffer() failed, data is null");
        return RS_ERR;
    }

    *dst = d;

    return RS_OK;
}

void rs_get_ring_buffer_advance(rs_ring_buffer_t *r) 
{
    r->rp = (r->rp + r->size - 1 + RS_RING_BUFFER_DATA_LEN) == r->last ? 
        (char *) r->rb : r->rp + r->size + RS_RING_BUFFER_DATA_LEN;

    r->rn++;
}

int rs_set_ring_buffer(rs_ring_buffer_t *r, rs_ring_buffer_data_t **dst) 
{
    rs_ring_buffer_data_t *d;

    if(r == NULL) {
        rs_log_err(0, "rs_set_ring_buffer() failed, pointer r is null");
        return RS_ERR;
    } 

    // rs_log_debug(0, "ring buffer write num : %u, read num : %u", r->wn, r->rn);

    if(r->wn - r->rn == r->num) {
        return RS_FULL;
    }

    d = (rs_ring_buffer_data_t *) r->wp;
    if(d == NULL) {
        rs_log_err(0, "rs_set_ring_buffer() failed, data is null");
        return RS_ERR;
    }

    *dst = d;

    return RS_OK;
}

void rs_set_ring_buffer_advance(rs_ring_buffer_t *r) 
{
    r->wp = (r->wp + r->size - 1 + RS_RING_BUFFER_DATA_LEN) == r->last ? 
        (void *) r->rb : r->wp + r->size + RS_RING_BUFFER_DATA_LEN;

    r->wn++;
}

void rs_free_ring_buffer(rs_ring_buffer_t *r) 
{
    if(r != NULL && r->rb != NULL) {   
        free(r->rb);
    }
}
