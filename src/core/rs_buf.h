
#ifndef _RS_BUF_H_INCLUDED_
#define _RS_BUF_H_INCLUDED_


#include <rs_config.h>
#include <rs_core.h>

typedef struct rs_buf_s rs_buf_t;

struct rs_buf_s {

    char        *pos;
    char        *last;
    char        *start;
    char        *end;
    uint32_t    size;

};

#define rs_buf_t_init(b)                                                     \
    (b)->pos = NULL;                                                         \
    (b)->start = NULL;                                                       \
    (b)->end = NULL;                                                         \
    (b)->last = NULL;                                                        \
    (b)->size = 0

rs_buf_t *rs_create_tmpbuf(uint32_t size);
int rs_send_tmpbuf(rs_buf_t *b, int fd);
int rs_recv_tmpbuf(rs_but_t *b, int fd, void *data, uint32_t size);
void rs_destroy_tmpbuf(rs_buf_t *b);


#define RS_RING_BUFFER_SPIN     65536 + 1 
#define rs_cpu_pause()          __asm__ (".byte 0xf3, 0x90")

typedef struct {

    uint32_t    len;
    void        *data;
    int         id;
} rs_ringbuf_data_t;

#define rs_ringbuf_data_t_init(d)                                            \
    (d)->len = 0;                                                            \
    (d)->data = NULL;                                                        \
    (d)->id = 0

typedef struct {

    uint64_t    rn;
    uint64_t    wn;

    uint32_t    num;

    char        *rp;
    char        *wp;

    char        *start;
    char        *end;

    int         id;
    rs_pool_t   *pool;

} rs_ringbuf_t;


rs_ringbuf_t *rs_create_ringbuf(rs_pool_t *p, uint32_t num);

int rs_ringbuf_get(rs_ringbuf_t *rb, rs_ringbuf_data_t **data);
void rs_ringbuf_get_advance(rs_ringbuf_t *rb);

int rs_ringbuf_set(rs_ringbuf_t *rb, rs_ringbuf_data_t **data);
void rs_ringbuf_set_advance(rs_ringbuf_t *rb);

int rs_ringbuf_spin_wait(rs_ringbuf_t *rb, rs_ringbuf_data_t **d);

void rs_destroy_ringbuf(rs_ringbuf_t *rb);

#endif
