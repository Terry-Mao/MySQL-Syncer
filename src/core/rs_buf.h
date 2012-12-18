
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

int rs_create_temp_buf(rs_buf_t *b, uint32_t size);
int rs_send_temp_buf(int fd, rs_buf_t *b);
void rs_free_temp_buf(rs_buf_t *b);

#endif
