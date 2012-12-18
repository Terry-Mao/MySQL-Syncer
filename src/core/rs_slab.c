
#include <rs_config.h>
#include <rs_core.h>


static void *rs_slab_allocmem(rs_slab_t *sl, uint32_t size);
static int rs_grow_slab(rs_slab_t *sl, int id);


int rs_init_slab(rs_slab_t *sl, uint32_t size, double factor, uint32_t mem_size
        , int32_t flags)
{
    int         i;

    if(sl == NULL) {
        rs_log_err(0, "rs_slab_init() failed, sl is NULL");    
        return RS_ERR;
    }

    sl->max_size = mem_size;

    /* Prealloc memory */
    if(flags == RS_SLAB_PREALLOC) {
        sl->start = malloc(mem_size); 

        if(sl->start == NULL) {
            rs_log_err(rs_errno, "malloc() in rs_init_slab failed");
            return RS_ERR;
        }

        sl->used_size = 0;
        sl->free_size = mem_size;
        sl->cur = sl->start;
    }

    rs_memzero(sl->slab_class, sizeof(sl->slab_class));

    for(i = RS_SLAB_CLASS_IDX_MIN; 
            i < RS_SLAB_CLASS_IDX_MAX && size < RS_SLAB_CHUNK_SIZE; i++) 
    {
        size = rs_align(size, RS_ALIGNMENT); 

        sl->slab_class[i].size = size;
        sl->slab_class[i].num = RS_SLAB_CHUNK_SIZE / 
            sl->slab_class[i].size;

        sl->slab_class[i].free_chunks = NULL;

        sl->slab_class[i].slabs = NULL;
        sl->slab_class[i].chunk = NULL;

        sl->slab_class[i].used_slab_n = 0;
        sl->slab_class[i].total_slab_n = 0;
        sl->slab_class[i].last_slab_n = 0;
        sl->slab_class[i].free_slab_n = 0;
        sl->slab_class[i].used_free_slab_n = 0;

        rs_log_debug(0, "slab class = %d, chunk size = %u, num = %u",
                i, sl->slab_class[i].size, sl->slab_class[i].num);

        size *= factor;
    }

    sl->class_idx = i;
    sl->slab_class[i].size = RS_SLAB_CHUNK_SIZE;
    sl->slab_class[i].num = 1;

    return RS_OK;
}

int rs_slab_clsid(rs_slab_t *sl, uint32_t size)
{
    int i;
    
    i = RS_SLAB_CLASS_IDX_MIN;

    if(size == 0) {
        rs_log_err(0, "rs_slab_clsid() faield, size is zero");
        return RS_ERR;
    }

    /* if > 1MB use malloc() */
    if(size > RS_SLAB_CHUNK_SIZE) {
        rs_log_debug(0, "rs_slab_cliid() overflow clsid");
        return RS_SLAB_OVERFLOW;
    }

    while(size > sl->slab_class[i].size) {
        /* reach the last index of class */
        if(i++ == sl->class_idx) {
            rs_log_debug(0, "rs_slab_cliid() overflow clsid");
            return RS_SLAB_OVERFLOW;
        }
    }

    rs_log_debug(0, "rs_slab_cliid() clsid = %d", i);

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
            rs_log_err(rs_errno, "realloc() in rs_grow_slab failed");
            return RS_ERR;
        }

        c->total_slab_n = size;
        c->slabs = p;
    }

    return RS_OK;
}

static int rs_new_slab(rs_slab_t *sl, int id)
{
    uint32_t            len;
    rs_slab_class_t     *c;
    char                *p;

    c = &(sl->slab_class[id]);
    len = c->size * c->num;

    /* test reach limit memory */
    if(sl->max_size && 
            sl->used_size + len > sl->max_size && 
            c->used_slab_n >0) 
    {
        rs_log_err(0, "rs_new_slab() failed, memory reach limit");
        return RS_ERR;
    }

    /* add a new slabs */
    if(rs_grow_slab(sl, id) != RS_OK) {
        return RS_ERR;
    }

    /* allocate memory to new slab */
    if((p = rs_slab_allocmem(sl, len)) == NULL) {
        return RS_ERR;
    }

    rs_memzero(p, len);
    c->chunk = p;
    c->last_slab_n = c->num;

    c->slabs[c->used_slab_n++] = p;
    sl->used_size += len;

    rs_log_debug(0, "used_slab_n = %u, last_slab_n = %u, used_size = %u", 
            c->used_slab_n, c->num, sl->used_size);

    return RS_OK;
}

void *rs_alloc_slab_chunk(rs_slab_t *sl, uint32_t size, int id)
{
    rs_slab_class_t     *c;
    void                *p;

    if(sl == NULL) {
        rs_log_err(0, "rs_slab_alloc() failed, sl is NULL");
        return NULL;
    }

    p = NULL;
    c = NULL;

    // use malloc()
    if(id == RS_SLAB_OVERFLOW) {
        p = (void *) malloc(size);

        if(p == NULL) {
            rs_log_err(rs_errno, "malloc() in rs_alloc_slab_chunk failed, size"
                    " = %u", size);
            return NULL;
        }

        return p;
    }

    if(id < RS_SLAB_CLASS_IDX_MIN || id > RS_SLAB_CLASS_IDX_MAX) {
        rs_log_err(0, "rs_alloc_slab_chunk() failed, id = %d overflow", id);
        return NULL;
    }

    c = &(sl->slab_class[id]);

    if(!(c->chunk != NULL || c->used_free_slab_n != 0 || 
            rs_new_slab(sl, id) != RS_ERR)) 
    {
        return NULL;
    } else if(c->used_free_slab_n > 0) {
        /* use free slab */
        p = c->free_chunks[--c->used_free_slab_n];
        rs_log_debug(0, "used_free_slab_n = %u", c->used_free_slab_n);
    } else {
        /* use slab */
        p = c->chunk;

        if(--c->last_slab_n > 0) {
            c->chunk = (void *) ((char *) c->chunk + c->size);
        } else {
            c->chunk = NULL;
        }

        rs_log_debug(0, "last_slab_n = %u", c->last_slab_n);
    }

    return p;
}

void rs_free_slab_chunk(rs_slab_t *sl, void *data, int id)
{
    rs_slab_class_t     *c;
    void                **free_chunks;
    uint32_t            fs;
    
    if(sl == NULL || data == NULL) {
        rs_log_err(0, "rs_free_slab_chunk() failed, sl or data is null");
        return;
    }

    c = NULL;
    free_chunks = NULL;

    if(id == RS_SLAB_OVERFLOW) {
       free(data); 
       return;
    }

    if(id < RS_SLAB_CLASS_IDX_MIN || id > RS_SLAB_CLASS_IDX_MAX) {
        rs_log_err(0, "rs_alloc_slab_chunk() failed, id = %d overflow", id);
        return;
    }

    c = &(sl->slab_class[id]);

    if (c->used_free_slab_n == c->free_slab_n) {
        fs = (c->free_slab_n == 0) ? 16 : c->free_slab_n * 2;

        free_chunks = realloc(c->free_chunks, fs * sizeof(void *));

        if(free_chunks == NULL) {
            rs_log_err(rs_errno, "realloc() in rs_free_slab_chunk failed");
            return;
        }

        c->free_chunks = free_chunks;
        c->free_slab_n = fs;
    }

    c->free_chunks[c->used_free_slab_n++] = data;

    rs_log_debug(0, "used_free_slab_n = %u, free_slab_n = %u", 
            c->used_free_slab_n, c->free_slab_n);
}

static void *rs_slab_allocmem(rs_slab_t *sl, uint32_t size)
{
    void    *p;

    p = NULL;

    /* no prelocate */
    if(sl->start == NULL) {
        p = malloc(size);

        if(p == NULL) {
            rs_log_err(rs_errno, "mallc() in rs_slab_allocmem failed"); 
            return NULL;
        }

    } else {
        /* memory align */
        size = rs_align(size, RS_ALIGNMENT);

        if(size > sl->free_size) {
            rs_log_err(0, "rs_mem_alloc() failed, no more memory");
            return NULL;
        }

        p = sl->cur; 

        sl->cur = (void *) ((char *) sl->cur + size);

        sl->free_size -= size;

        rs_log_debug(0, "free_size = %u, size = %u", sl->free_size, size);
    }

    return p;
}

void rs_free_slabs(rs_slab_t *sl)
{
    int i;

    if(sl == NULL) {
        rs_log_err(0, "rs_free_slabs() failed, sl is null");
        return;
    }

    for(i = RS_SLAB_CLASS_IDX_MIN; i <= sl->class_idx; i++) {

        if(sl->slab_class[i].slabs != NULL) {
            free(sl->slab_class[i].slabs);
        }

        if(sl->slab_class[i].free_chunks != NULL) {
            free(sl->slab_class[i].free_chunks);
        }
    }

    free(sl->start);
}
