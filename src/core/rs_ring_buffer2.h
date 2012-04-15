
#ifndef _RS_RING_BUFFER2_H_INCLUDED_
#define _RS_RING_BUFFER2_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>


typedef struct {

    uint32_t                len;
    void                    *data;
    int                     id;
} rs_ring_buffer2_data_t;

#define rs_ring_buffer2_data_t_init(d)                                       \
    (d)->len = 0;                                                            \
    (d)->data = NULL;                                                        \
    (d)->id = 0

typedef struct {

    volatile uint64_t       rn;
    volatile uint64_t       wn;

    uint32_t                size;
    uint32_t                num;

    void                    *rp;
    void                    *wp;

    void                    *start;
    void                    *end;
} rs_ring_buffer2_t;


int rs_init_ring_buffer2(rs_ring_buffer2_t *rb, uint32_t num);

int rs_get_ring_buffer2(rs_ring_buffer2_t *rb, rs_ring_buffer2_data_t **data);
void rs_get_ring_buffer2_advance(rs_ring_buffer2_t *rb);

int rs_set_ring_buffer2(rs_ring_buffer2_t *rb, rs_ring_buffer2_data_t **data); 
void rs_set_ring_buffer2_advance(rs_ring_buffer2_t *rb);

#endif
