
#ifndef _RS_HASH_H_INCLUDED_
#define _RS_HASH_H_INCLUDED_


#include <rs_config.h>
#include <rs_core.h>


typedef struct rs_shash_node_s rs_shash_node_t;

struct rs_shash_node_s {
    char                *key;
    void                *val;
    rs_shash_node_t     *next, **prev;
    int32_t             id;
};

typedef struct {
    rs_shash_node_t *first;  
} rs_shash_head_t;  


typedef struct {
    rs_shash_head_t *ht;
    uint32_t        num;
    int32_t         id;
    rs_pool_t       *pool;
} rs_shash_t;


rs_shash_t *rs_create_shash(rs_pool_t *p, uint32_t num);
int rs_shash_add(rs_shash_t *h, char *key, void *val); 
int rs_shash_get(rs_shash_t *h, char *key, void **val); 
void rs_destroy_shash(rs_shash_t *h);

#endif
