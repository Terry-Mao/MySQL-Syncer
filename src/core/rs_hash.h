
#ifndef _RS_HASH_H_INCLUDED_
#define _RS_HASH_H_INCLUDED_


#include <rs_config.h>
#include <rs_core.h>


typedef struct rs_shash_node_s rs_shash_node_t;

struct rs_shash_node_s {
    char                *key;
    void                *val;
    rs_shash_node_t     *next, **prev;
};

typedef struct {
    rs_shash_node_t *first;  
} rs_shash_head_t;  


typedef struct {
    rs_shash_head_t *ht;
    uint32_t        size;
} rs_shash_t;


rs_shash_t *rs_init_shash(uint32_t size);
int rs_add_shash(char *key, void *val, rs_shash_t *h);
int rs_get_shash(char *key, rs_shash_t *h, void **val);
void rs_free_shash(rs_shash_t *h);

#endif
