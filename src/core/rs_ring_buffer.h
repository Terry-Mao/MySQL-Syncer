
#ifndef _RS_RING_BUFFER_H_INCLUDED_
#define _RS_RING_BUFFER_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>

typedef struct rs_ring_buffer_s rs_ring_buffer_t;

typedef struct {
    uint32_t len;
    char *data;
} rs_ring_buffer_data_t;

struct rs_ring_buffer_s {

    volatile uint64_t       rn;
    volatile uint64_t       wn;
    uint32_t                size;
    uint32_t                num;
    rs_ring_buffer_data_t   *rb;
    char                    *rp;
    char                    *wp;
    char                    *last;

};

#define RS_RING_BUFFER_DATA_LEN     sizeof(rs_ring_buffer_data_t)

#define RS_RING_BUFFER_SPIN         65536 + 1
#define rs_cpu_pause()              __asm__ (".byte 0xf3, 0x90")

#define rs_ring_buffer_t_init(rb)                                            \
    (rb)->rn = 0;                                                            \
    (rb)->wn = 0;                                                            \
    (rb)->size = 0;                                                          \
    (rb)->num = 0;                                                           \
    (rb)->rb = NULL;                                                         \
    (rb)->rp = NULL;                                                         \
    (rb)->wp = NULL;                                                         \
    (rb)->last = NULL  


int rs_init_ring_buffer(rs_ring_buffer_t *r, uint32_t size, uint32_t num);

int rs_get_ring_buffer(rs_ring_buffer_t *r, rs_ring_buffer_data_t **dst); 
void rs_get_ring_buffer_advance(rs_ring_buffer_t *r); 

int rs_set_ring_buffer(rs_ring_buffer_t *r, rs_ring_buffer_data_t **dst); 
void rs_set_ring_buffer_advance(rs_ring_buffer_t *r); 

void rs_free_ring_buffer(rs_ring_buffer_t *r); 


#endif
