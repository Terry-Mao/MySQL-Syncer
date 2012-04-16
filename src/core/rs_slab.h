
#ifndef _RS_SLAB_H_INCLUDED_
#define _RS_SLAB_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>

#define RS_SLAB_CLASS_IDX_MIN       0
#define RS_SLAB_CLASS_IDX_MAX       200
#define RS_SLAB_CHUNK_SIZE          (1 * 1024 * 1024)

#define RS_SLAB_PREALLOC            1

typedef struct {
    uint32_t        size;
    uint32_t        num;

    void            **free_chunks;
    void            *chunk;

    uint32_t        used_slab_n;
    uint32_t        last_slab_n;
    uint32_t        total_slab_n;

    uint32_t        free_slab_n; 
    uint32_t        used_free_slab_n;

    void            **slabs;

    uint32_t        died_slab_n;
} rs_slab_class_t;


typedef struct {

    rs_slab_class_t slab_class[RS_SLAB_CLASS_IDX_MAX + 1];

    int             class_idx;
    
    void            *start;
    void            *cur;
    
    uint32_t        max_size;
    uint32_t        used_size;
    uint32_t        free_size; 
} rs_slab_t;


int rs_slab_clsid(rs_slab_t *sl, uint32_t size);
void *rs_alloc_slab(rs_slab_t *sl, uint32_t size, int id);
int rs_init_slab(rs_slab_t *sl, uint32_t *size_list, uint32_t size, 
        double factor, uint32_t mem_size, int32_t flags);
void rs_free_slabs(rs_slab_t *sl);
void rs_free_slab_chunk(rs_slab_t *sl, void *data, int id);

#endif
