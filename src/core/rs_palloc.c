
#include <rs_config.h>
#include <rs_core.h>


static char *rs_pmemalloc(rs_pool_t *p, uint32_t size);
static int rs_palloc_grow(rs_pool_t *p, int id);
static int rs_palloc_new(rs_pool_t *p, int id);

static int rs_palloc_grow(rs_pool_t *p, int id)
{
    rs_pool_class_t *c;
    uint32_t        size;

    c = &(p->slab_class[id]);

    /* no more slabs */
    if(c->used_slab == c->total_slab) {
        size = (c->total_slab == 0) ? 16 : c->total_slab * 2; 

        /* resize slabs, unchange old data */
        c->slab = realloc(c->slab, size * sizeof(char *));

        if(c->slab == NULL) {
            rs_log_err(rs_errno, "realloc(%u) failed", size * sizeof(char *));
            return RS_ERR;
        }

        c->total_slab = size;
    }

    return RS_OK;
}

static int rs_palloc_new(rs_pool_t *p, int id)
{
    uint32_t        len;
    rs_pool_class_t *c;
    char            *t;

    t = NULL;
    c = &(p->slab_class[id]);
    len = c->size * c->num;

    /* add a new slabs */
    if(rs_palloc_grow(p, id) != RS_OK) {
        return RS_ERR;
    }

    /* allocate memory to new slab */
    if((t = rs_pmemalloc(p, len)) == NULL) {
        return RS_ERR;
    }

    rs_memzero(t, len);
    c->chunk = t;
    c->free = c->num;
    c->slab[c->used_slab++] = t;

    rs_log_core(0, "palloc new slab id %d, used slab num = %u, "
            "total slab num = %u", 
            id,
            c->used_slab, 
            c->total_slab);

    return RS_OK;
}

static char *rs_pmemalloc(rs_pool_t *p, uint32_t size)
{
    char *t;

    t = NULL;

    if(size > p->free_size) 
    {
        rs_log_err(0, "rs_pmemalloc() failed, no more memory");
        return NULL;
    }

    if(p->flag == RS_POOL_PAGEALLOC) {
        /* no prelocate */
        t = malloc(size);

        if(t == NULL) {
            rs_log_err(rs_errno, "malloc(%u) failed", size);
            return NULL;
        }
    } else if(p->flag == RS_POOL_PREALLOC) {
        /* prealloc */
        t = p->cur; 
        p->cur = p->cur + size;
    }

    p->free_size -= size;
    p->used_size += size;

    rs_log_core(0, "pmemalloc pool alloc_size = %u, free_size = %u, "
            "used_size = %u", 
            size, p->free_size, p->used_size);

    return t;
}

int rs_palloc_id(rs_pool_t *p, uint32_t size)
{
    int low, high, mid;
    
    low = 0;
    high = p->cur_idx;
    mid = 0;

    if(size == 0) {
        rs_log_err(0, "rs_palloc_id() faield, size must great than zero");
        return RS_ERR;
    }

    /* if > 1MB use malloc() */
    if(size > p->chunk_size) {
        rs_log_core(0, "palloc size %u overflow %u", size, 
                size - p->chunk_size); 
        return RS_SLAB_OVERFLOW;
    }

    while(low <= high) {
        
        mid = low + ((high - low) >> 1);

        if(p->slab_class[mid].size > size) {
            high = mid -1;
        } else if(p->slab_class[mid].size < size) {
            low = mid + 1;
        } else {
            rs_log_core(0, "pool class size %u, clsid %d", size, low);
            return low;
        }
    }

    if(low > -1 && low <= p->cur_idx) {
        rs_log_core(0, "pool class size %u, clsid %d", size, low);
        return low;
    }

    rs_log_core(0, "palloc size %u overflow %u", size, 
            size - p->chunk_size); 
    return RS_SLAB_OVERFLOW;
}

rs_pool_t *rs_create_pool(uint32_t init_size, uint32_t mem_size, 
        uint32_t chunk_size, uint32_t max_idx, double factor, int32_t flag)
{
    int             i;
    uint32_t        ps;
    rs_pool_t       *p;
    rs_pool_class_t *c;

    mem_size = rs_align(mem_size, RS_ALIGNMENT); 
    init_size = rs_align(init_size, RS_ALIGNMENT); 

    rs_log_core(0, "pool align mem_size %u", mem_size);

    if(flag == RS_POOL_PREALLOC) {
        /* prealloc memory */
        p = malloc(sizeof(rs_pool_t) + mem_size + 
                sizeof(rs_pool_class_t) * (max_idx + 1));

        if(p == NULL) {
            rs_log_err(rs_errno, "malloc(%u) failed", 
                    sizeof(rs_pool_t) + mem_size);
            return NULL;
        }

    } else if(flag == RS_POOL_PAGEALLOC) {
        /* pagealloc memory */
        ps = (mem_size / chunk_size) * sizeof(char *);
        p = malloc(sizeof(rs_pool_t) + ps + 
                sizeof(rs_pool_class_t) * (max_idx + 1));

        if(p == NULL) {
            rs_log_err(rs_errno, "malloc(%u) failed", 
                    sizeof(rs_pool_t) + ps);
            return NULL;
        }

    } else {
        rs_log_err(0, "unknown slab flag %d", flag);
        return NULL;
    }

    p->slab_class = (rs_pool_class_t *) ((char *) p + sizeof(rs_pool_t));
    p->start = (char *) p->slab_class + sizeof(rs_pool_class_t) * (max_idx + 1);
    p->cur = p->start;
    p->flag = flag;
    p->max_size = mem_size;
    p->free_size = mem_size;
    p->chunk_size = chunk_size;
    p->max_idx = max_idx;
    p->used_size = 0;

    for(i = 0; i < p->max_idx && init_size < p->chunk_size; i++) {
        rs_log_core(0, "pool align init_size %u", init_size);
        c = &(p->slab_class[i]);
        
        rs_pool_class_t_init(c);
        c->size = init_size;
        c->num = chunk_size / init_size;

        rs_log_core(0, "slab class id= %d, chunk size = %u, num = %u",
                i, p->slab_class[i].size, p->slab_class[i].num);

        init_size = rs_align((uint32_t) (init_size * factor), RS_ALIGNMENT);
    }

    p->cur_idx = i;

    c = &(p->slab_class[i]);
    rs_pool_class_t_init(c);

    p->slab_class[i].size = chunk_size;
    p->slab_class[i].num = 1;

    return p;
}

void *rs_palloc(rs_pool_t *p, uint32_t size, int id)
{
    rs_pool_class_t *c;
    char            *t;

    t = NULL;
    c = NULL;

    // use malloc()
    if(id == RS_SLAB_OVERFLOW) {
        t = (char *) malloc(size);

        if(t == NULL) {
            rs_log_err(rs_errno, "malloc(%u) failed", size);
            return NULL;
        }

        return t;
    }

    c = &(p->slab_class[id]);

    if(!(c->chunk != NULL || c->used_free > 0 || rs_palloc_new(p, id) == RS_OK))
    {
        return NULL;
    } else if(c->used_free > 0) {
        t = c->free_chunk[--c->used_free];
        rs_log_core(0, "palloc used free chunk num %u", c->used_free);
    } else {
        t = c->chunk;

        if(--c->free > 0) {
            c->chunk = c->chunk + c->size;
        } else {
            c->chunk = NULL;
        }

        rs_log_core(0, "palloc free chunk num = %u", c->free);
    }

    return (void *) t;
}

void rs_pfree(rs_pool_t *p, void *data, int id)
{
    rs_pool_class_t *c;
    char            **free_chunk;
    uint32_t        tf;
    
    c = NULL;
    free_chunk = NULL;

    if(id == RS_SLAB_OVERFLOW) {
       free(data); 
       return;
    }

    c = &(p->slab_class[id]);

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

    rs_log_core(0, "used free chunk num = %u", c->used_free);
}

void rs_destroy_pool(rs_pool_t *p)
{
    int32_t i, j;

    for(i = 0; i <= p->cur_idx; i++) {

        if(p->slab_class[i].slab != NULL) {

            if(p->flag == RS_POOL_PAGEALLOC) { 
                for(j = 0; j < (int) p->slab_class[i].used_slab; j++) {
                    free(p->slab_class[i].slab[j]);
                }
            }

            free(p->slab_class[i].slab);
        }

        if(p->slab_class[i].free_chunk != NULL) {
            free(p->slab_class[i].free_chunk);
        }
    }

    free(p);
}
