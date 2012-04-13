
#include <rs_config.h>
#include <rs_core.h>


static void *rs_mem_alloc(rs_slab_t *sl, uint32_t size);
static int rs_grow_slab(rs_slab_t *sl, int id);
static void *rs_mem_alloc(rs_slab_t *sl, uint32_t size);


int rs_slab_init(rs_slab_t *sl, uint32_t *size_list, uint32_t size, 
        double factor, uint32_t mem_size, int32_t flags)
{
    int         i;
    uint32_t    *l;

    i = RS_SLAB_CLASS_IDX_MIN;

    if(sl == NULL) {
        rs_log_err(0, "rs_slab_init() failed, sl is NULL");    
        return RS_ERR;
    }

    sl->max_mem_size = mem_size;

    /* Prealloc memory */
    if(flags == RS_SLAB_PREALLOC) {
        sl->start = malloc(mem_size); 

        if(sl->start == NULL) {
            rs_log_err(rs_errno, "malloc() failed, slab memory");
            return RS_ERR;
        }

        sl->free_mem_size = mem_size;
    }

    rs_memzero(sl->slab_class, sizeof(sl->slab_class));

    if(size_list == NULL) {
        for(i = RS_SLAB_CLASS_IDX_MIN; 
                i < RS_SLAB_CLASS_IDX_MAX && size < RS_SLAB_CHUNK_SIZE; i++) 
        {
            size = rs_align(size, RS_ALIGNMENT); 

            sl->slab_class[i].size = size;
            sl->slab_class[i].num = RS_SLAB_CHUNK_SIZE / 
                sl->slab_class[i].size;

            rs_log_debug(0, "slab class = %d, chunk size = %u, num = %u",
                    i, sl->slab_class[i].size, sl->slab_class[i].num);

            size *= factor;
        }

    } else {
        for(l = size_list; l && *l < RS_SLAB_CHUNK_SIZE; l++) {
            size = rs_align(size, RS_ALIGNMENT); 

            sl->slab_class[i].size = size;
            sl->slab_class[i].num = RS_SLAB_CHUNK_SIZE / 
                sl->slab_class[i].size;
            i++;

            rs_log_debug(0, "slab class = %d, chunk size = %u, num = %u",
                    i, sl->slab_class[i].size, sl->slab_class[i].num);
        }
    }

    sl->cur_slab_class_idx = i;
    sl->slab_class[i].size = RS_SLAB_CHUNK_SIZE;
    sl->slab_class[i].num = 1;

    return RS_OK;
}

int rs_slab_clsid(rs_slab_t *sl, uint32_t size)
{
    int i;
    
    i = RS_SLAB_CLASS_IDX_MIN;

    if(size == 0) {
        rs_log_debug(0, "rs_slab_clisid() failed ,size is zero");
        return RS_ERR;
    }

    while(size > sl->slab_class[i].size) {
        /* reach the last index of class */
        if(i++ == RS_SLAB_CLASS_IDX_MAX) {
            return RS_SLAB_OVERFLOW;
        }
    }

    return i;
}

static int rs_grow_slab(rs_slab_t *sl, int id)
{
    rs_slab_class_t *c;
    uint32_t        size;
    void            **p;

    c = &(sl->slab_class[id]);

    /* no more slabs */
    if(c->used_slab_n == c->total_slab_n) {
        size = (c->total_slab_n == 0) ? 16 : c->total_slab_n * 2; 

        /* resize slabs, unchange old data */
        p = realloc(c->slabs, size * sizeof(void *));

        if(p == NULL) {
            rs_log_err(rs_errno, "realloc() failed, glow slabs");
            return RS_ERR;
        }

        c->total_slab_n = size;
        c->slabs = p;
    }

    return RS_OK;
}

static int rs_slab_newslab(rs_slab_t *sl, int id)
{
    uint32_t            len;
    rs_slab_class_t     *c;
    char                *p;

    c = &(sl->slab_class[id]);
    len = c->size * c->num;

    /* test reach limit memory */
    if(sl->max_mem_size && 
            sl->used_mem_size + len > sl->max_mem_size && 
            c->used_slab_n >0) 
    {
        rs_log_err(0, "rs_slab_newslab() failed, memory reach limit");
        return RS_ERR;
    }

    /* add a new slabs */
    if(rs_grow_slab(sl, id) != RS_OK) {
        return RS_ERR;
    }

    /* allocate memory to new slab */
    if((p = rs_mem_alloc(sl, len)) == NULL) {
        return RS_ERR;
    }

    rs_memzero(p, len);
    c->slab = p;
    c->last_slab_n = c->num;

    c->slabs[c->used_slab_n++] = p;
    sl->used_mem_size += len;

    return RS_OK;
}

void *rs_slab_alloc(rs_slab_t *sl, uint32_t size, int id)
{
    rs_slab_class_t    *c;
    void               *p;

    p = NULL;

    if(sl == NULL) {
        rs_log_err(0, "rs_slab_alloc() failed, sl is NULL");
        return NULL;
    }

    if(id == -1) {
        p = malloc(size);

        if(p == NULL) {
            rs_log_err(rs_errno, "malloc() failed, rs_slab_alloc");
        }

        return p;
    }

    if(id < RS_SLAB_CLASS_IDX_MIN || id > RS_SLAB_CLASS_IDX_MAX) {
        rs_log_err(0, "rs_slab_alloc() failed, id = %d", id);
        return NULL;
    }

    c = &(sl->slab_class[id]);

    if(c->slab == NULL || c->used_free_slab_n == 0) {
        rs_log_err(0, "no more free slab, get a new slab");

        if(rs_slab_newslab(sl, id) == RS_ERR) {
            return NULL;
        }
    } else if(c->used_free_slab_n != 0) {
        p = c->free_slabs[--c->used_free_slab_n];
    } else {
        p = c->slab;

        if(--c->last_slab_n != 0) {
            c->slab = (void *) ((char *) c->slab + c->size);
        } else {
            c->slab = NULL;
        }
    }

    return p;
}

void rs_slab_free(rs_slab_t *sl, void *data, int id)
{
    rs_slab_class_t     *c;
    uint32_t            size;
    void                **free_slabs;

    c = &(sl->slab_class[id]);

    if (c->used_free_slab_n == c->free_slab_n) {
        size = (c->free_slab_n == 0) ? 16 : c->free_slab_n * 2;

        free_slabs = realloc(c->free_slabs, size * sizeof(void *));

        if(free_slabs == NULL) {
            rs_log_err(rs_errno, "realloc() failed, free_slabs");
            return;
        }

        c->free_slabs = free_slabs;
        c->free_slab_n = size;
    }

    c->free_slabs[c->used_free_slab_n++] = data;
}

static void *rs_mem_alloc(rs_slab_t *sl, uint32_t size)
{
    void    *p;

    /* no prelocate */
    if(sl->start == NULL) {
        p = malloc(size);

        if(p == NULL) {
            rs_log_err(rs_errno, "mallc() failed, rs_mem_alloc"); 
            return NULL;
        }

    } else {
        /* memory align */
        size = rs_align(size, RS_ALIGNMENT);

        if(size > sl->free_mem_size) {
            rs_log_err(0, "rs_mem_alloc() failed, no more memory");
            return NULL;
        }

        p = sl->cur; 

        sl->cur = (void *) ((char *) sl->cur + size);

        sl->free_mem_size -= size;
    }

    return p;
}
