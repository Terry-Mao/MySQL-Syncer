
#include <rs_config.h>
#include <rs_core.h>


static char *rs_alloc_memslab(rs_memslab_t *ms, uint32_t size);
static int rs_grow_memslab(rs_memslab_t *ms, int id);
static int rs_new_memslab(rs_memslab_t *sl, int id);

static int rs_grow_memslab(rs_memslab_t *ms, int id)
{
    rs_memslab_class_t *c;
    uint32_t        size;
    char            **p;

    c = &(ms->slab_class[id]);

    /* no more slabs */
    if(c->used_slab == c->total_slab) {
        size = (c->total_slab == 0) ? 16 : c->total_slab * 2; 

        /* resize slabs, unchange old data */
        p = realloc(c->slab, size * sizeof(char *));

        if(p == NULL) {
            rs_log_err(rs_errno, "realloc(%u) failed", size * sizeof(char *));
            return RS_ERR;
        }

        c->total_slab = size;
        c->slab = p;
    }

    return RS_OK;
}

static int rs_new_memslab(rs_memslab_t *ms, int id)
{
    uint32_t            len;
    rs_memslab_class_t  *c;
    char                *p;

    p = NULL;
    c = &(ms->slab_class[id]);
    len = c->size * c->num;

    /* add a new slabs */
    if(rs_grow_memslab(ms, id) != RS_OK) {
        return RS_ERR;
    }

    /* allocate memory to new slab */
    if((p = rs_alloc_memslab(ms, len)) == NULL) {
        return RS_ERR;
    }

    rs_memzero(p, len);
    c->chunk = p;
    c->free = c->num;
    c->slab[c->used_slab++] = p;

    rs_log_debug(0, "id %d, used slab num = %u, total slab num = %u", 
            id,
            c->used_slab, 
            c->total_slab);

    return RS_OK;
}

static char *rs_alloc_memslab(rs_memslab_t *ms, uint32_t size)
{
    char    *p;

    p = NULL;

    if(ms->flag == RS_MEMSLAB_PAGEALLOC) {
        /* no prelocate */
        if(ms->cur_page++ >= ms->max_page - 1 || size > ms->free_size) 
        {
            rs_log_err(0, "rs_slab_allocmem() failed, no more memory");
            return NULL;
        }

        p = malloc(size);

        if(p == NULL) {
            rs_log_err(rs_errno, "malloc(%u) failed", size);
            return NULL;
        }

    } else if(ms->flag == RS_MEMSLAB_PREALLOC) {
        /* prealloc */
        if(size > ms->free_size) {
            rs_log_err(0, "rs_slab_allocmem() failed, no more memory");
            return NULL;
        }

        p = ms->cur; 
        ms->cur = ms->cur + size;
    }

    ms->free_size -= size;
    ms->used_size += size;

    rs_log_debug(0, "memslab free_size = %u, used_size = %u", 
            ms->free_size, ms->used_size);

    return p;
}

int rs_memslab_clsid(rs_memslab_t *ms, uint32_t size)
{
    int id;
    
    id = 0;

    if(size == 0) {
        rs_log_err(0, "rs_memslab_clsid() faield, size = 0");
        return RS_ERR;
    }

    /* if > 1MB use malloc() */
    if(size > RS_MEMSLAB_CHUNK_SIZE) {
        rs_log_debug(0, "rs_memslab_clsid(), size %u overflow %u", 
                size, RS_MEMSLAB_CHUNK_SIZE); 
        return RS_SLAB_OVERFLOW;
    }

    while(size > ms->slab_class[id].size) {
        /* reach the last index of class */
        if((uint32_t) id++ == ms->max_class) {
            rs_log_debug(0, "rs_memslab_clsid(), size %u overflow %u", 
                    size, RS_MEMSLAB_CHUNK_SIZE); 
            return RS_SLAB_OVERFLOW;
        }
    }

    rs_log_debug(0, "size %u, clsid %d", size, id);

    return id;
}

rs_memslab_t *rs_init_memslab(uint32_t init_size, uint32_t mem_size, 
        double factor, int32_t flag)
{
    int             i;
    uint32_t        ps;
    rs_memslab_t    *ms;
    char            *p;

    mem_size = rs_align(mem_size, RS_ALIGNMENT); 
    init_size = rs_align(init_size, RS_ALIGNMENT); 
    rs_log_debug(0, "memslab align mem_size %u", mem_size);

    if(flag == RS_MEMSLAB_PREALLOC) {
        /* prealloc memory */
        p = (char *) malloc(sizeof(rs_memslab_t) + mem_size);

        if(p == NULL) {
            rs_log_err(rs_errno, "malloc(%u) failed", 
                    sizeof(rs_memslab_t) + mem_size);
            return NULL;
        }

        ms = (rs_memslab_t *) p;
        ms->start = p + sizeof(rs_memslab_t);
        ms->cur = ms->start;

    } else if(flag == RS_MEMSLAB_PAGEALLOC) {
        /* pagealloc memory */
        ps = (mem_size / RS_MEMSLAB_CHUNK_SIZE) * sizeof(char *);
        p = (char *) malloc(sizeof(rs_memslab_t) + ps);

        if(p == NULL) {
            rs_log_err(rs_errno, "malloc(%u) failed", 
                    sizeof(rs_memslab_t) + ps);
            return NULL;
        }

        ms = (rs_memslab_t *) p;
        ms->start = p + sizeof(rs_memslab_t);
        rs_memzero(ms->start, ps);
        ms->cur_page = 0;
        ms->max_page = mem_size / RS_MEMSLAB_CHUNK_SIZE;
    
    } else {
        rs_log_err(0, "unknown slab flag %d", flag);
        return NULL;
    }

    ms->flag = flag;
    ms->max_size = mem_size;
    ms->free_size = mem_size;
    ms->used_size = 0;
    rs_memzero(ms->slab_class, sizeof(ms->slab_class));

    for(i = 0; i < RS_MEMSLAB_CLASS_IDX_MAX && 
            init_size < RS_MEMSLAB_CHUNK_SIZE; i++) 
    {
        rs_log_debug(0, "memslab align init_size %u", init_size);
        ms->slab_class[i].size = init_size;
        ms->slab_class[i].num = RS_MEMSLAB_CHUNK_SIZE / init_size;

        ms->slab_class[i].chunk = NULL;
        ms->slab_class[i].free_chunk = NULL;
        ms->slab_class[i].slab = NULL;

        ms->slab_class[i].used_slab = 0;
        ms->slab_class[i].total_slab = 0;

        ms->slab_class[i].free = 0;
        ms->slab_class[i].total_free = 0;
        ms->slab_class[i].used_free = 0;

        rs_log_debug(0, "slab class id= %d, chunk size = %u, num = %u",
                i, ms->slab_class[i].size, ms->slab_class[i].num);

        init_size = rs_align((uint32_t) (init_size * factor), RS_ALIGNMENT);
    }

    ms->max_class = i;
    ms->slab_class[i].size = RS_MEMSLAB_CHUNK_SIZE;
    ms->slab_class[i].num = 1;

    return ms;
}

char *rs_alloc_memslab_chunk(rs_memslab_t *ms, uint32_t size, int id)
{
    rs_memslab_class_t  *c;
    char                *p;

    p = NULL;
    c = NULL;

    // use malloc()
    if(id == RS_SLAB_OVERFLOW) {
        p = (char *) malloc(size);

        if(p == NULL) {
            rs_log_err(rs_errno, "malloc(%u) failed", size);
            return NULL;
        }

        return p;
    }

    c = &(ms->slab_class[id]);

    if(!(c->chunk != NULL || c->used_free > 0 || rs_new_memslab(ms, id) 
                == RS_OK))
    {
        return NULL;
    } else if(c->used_free > 0) {
        p = c->free_chunk[--c->used_free];
        rs_log_debug(0, "used free chunk num %u", c->used_free);
    } else {
        p = c->chunk;

        if(--c->free > 0) {
            c->chunk = c->chunk + c->size;
        } else {
            c->chunk = NULL;
        }

        rs_log_debug(0, "free chunk num = %u", c->free);
    }

    return p;
}

void rs_free_memslab_chunk(rs_memslab_t *ms, char *data, int id)
{
    rs_memslab_class_t  *c;
    char                **free_chunk;
    uint32_t            tf;
    
    c = NULL;
    free_chunk = NULL;

    if(id == RS_SLAB_OVERFLOW) {
       free(data); 
       return;
    }

    c = &(ms->slab_class[id]);

    if (c->used_free == c->total_free) {
        tf = (c->total_free == 0) ? 16 : c->total_free * 2;

        free_chunk = realloc(c->free_chunk, tf * sizeof(char *));

        if(free_chunk == NULL) {
            rs_log_err(rs_errno, "realloc(%u) failed", tf * sizeof(char *));
            return;
        }

        c->free_chunk = free_chunk;
        c->total_free = tf;
    }

    c->free_chunk[c->used_free++] = data;

    rs_log_debug(0, "used free chunk num = %u", c->used_free);
}

void rs_free_memslabs(rs_memslab_t *ms)
{
    uint32_t    i;
    char        *p;

    for(i = 0; i <= ms->max_class; i++) {

        if(ms->slab_class[i].slab != NULL) {
            free(ms->slab_class[i].slab);
        }

        if(ms->slab_class[i].free_chunk != NULL) {
            free(ms->slab_class[i].free_chunk);
        }
    }

    if(ms->flag == RS_MEMSLAB_PREALLOC) {
        free(ms);
    } else if(ms->flag == RS_MEMSLAB_PAGEALLOC) {
        for(i = 0; i < ms->max_page; i++) {
            p = ms->start + sizeof(char *);
            if(p != NULL) {
                free(p);
            }
        } 
    }
}
