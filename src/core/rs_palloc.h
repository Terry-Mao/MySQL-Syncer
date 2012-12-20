
#ifndef _RS_SLAB_H_INCLUDED_
#define _RS_SLAB_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>

#define RS_MEMSLAB_CLASS_IDX_MAX       200
#define RS_MEMSLAB_CHUNK_SIZE          (uint32_t) (getpagesize() - 1)

#define RS_POOL_PAGEALLOC           0
#define RS_POOL_PREALLOC            1

typedef struct {
    uint32_t        size; /* slab chunk size */
    uint32_t        num;

    char            **free_chunk;
    char            *chunk;

    uint32_t        free;

    uint32_t        used_slab;
    uint32_t        total_slab;

    uint32_t        total_free; 
    uint32_t        used_free;

    char            **slab;

    uint32_t        died;
} rs_pool_class_t;


typedef struct {

    rs_pool_class_t slab_class[RS_MEMSLAB_CLASS_IDX_MAX + 1]; /* chunk class */
    int32_t         max_class; /* max slab class index */

    char            *start; /* while prealloc start is start memory ptr */
    char            *cur; /* while preaaloc cur is current memory ptr */
    
    uint32_t        max_size; /* max memory size */
    uint32_t        used_size; /* used memory max size */
    uint32_t        free_size; /* free memory size*/

    //uint32_t        cur_page; /* current page */
    //uint32_t        max_page; /**/

    int             flag;

} rs_pool_t;

/*
 * DESCRIPTION 
 *   Calc alloc size belong to which class id.
 *
 * PARAMTER 
 *   sl    : struct rs_pool_t
 *   size  : alloc size
 *
 * RETURN VALUE
 *   On success class_id is returned, On error RS_ERR is returned.
 */
int rs_palloc_id(rs_pool_t *p, uint32_t size);

/*
 * DESCRIPTION 
 *   Init memory slab.
 *
 * PARAMTER 
 *   init_size  : init chunk size
 *   mem_size   : memory max size
 *   factor     : chunk class grow factor
 *   flag       : prealloc or page assgin
 *
 * RETURN VALUE
 *   On success rs_pool_t is returned, On error NULL is returned.
 */
rs_pool_t *rs_create_pool(uint32_t init_size, uint32_t mem_size, double factor, 
        int32_t flag);

/*
 * DESCRIPTION 
 *   Alloc memory from slab class
 *
 * PARAMTER 
 *   sl    : struct rs_pool_t
 *   size  : memory size
 *   id    : chunk class id
 *
 * RETURN VALUE
 *   On success rs_pool_t is returned, On error NULL is returned.
 */
void *rs_palloc(rs_pool_t *p, uint32_t size, int id);

/*
 * DESCRIPTION 
 *   Free class chunk
 *
 * PARAMTER 
 *   sl    : struct rs_pool_t
 *   data  : free chunk data ptr
 *   id    : chunk class id
 *
 * RETURN VALUE
 *   On success class_id is returned, On error RS_ERR is returned.
 */
void rs_pfree(rs_pool_t *p, void *data, int id);

/*
 * DESCRIPTION 
 *   Free all slabs
 *
 * PARAMTER 
 *   sl    : struct rs_pool_t 
 *
 * RETURN VALUE
 *   None
 */
void rs_destroy_pool(rs_pool_t *p);

#endif
