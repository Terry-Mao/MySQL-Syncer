
#include <rs_config.h>
#include <rs_core.h>

static uint32_t rs_bkd_hash(char *str);
static inline void rs_shash_after_node(rs_shash_node_t *c, rs_shash_node_t *n);


static uint32_t rs_bkd_hash(char *str)
{
    uint32_t seed, hash;
    seed = 131;
    hash = 0;

    while(*str) {
        hash = hash * seed + (*str++);
    }
    return hash & 0x7FFFFFFF;
}

static inline void rs_shash_after_node(rs_shash_node_t *c, rs_shash_node_t *n)
{
    n->next = c->next;
    c->next = n;
    n->prev = &c->next;

    if(n->next)
        n->next->prev = &n->next;
}

rs_shash_t *rs_shash_init(rs_pool_t *p, uint32_t num)
{
    int32_t     id;
    rs_shash_t  *h;
    char        *t;

    h = NULL;
    t = NULL; 

    if(num == 0) {
        rs_log_err(0, "rs_shash_init() failed, num must great than zero");
        return NULL;
    }
    
    id = rs_palloc_id(p, sizeof(rs_shash_t) + sizeof(rs_shash_head_t) * num);
    t = (char *) rs_palloc(p, sizeof(rs_shash_t) + sizeof(rs_shash_head_t), id);

    if(t == NULL) {
        return NULL;
    }

    h = (rs_shash_t *) t;
    h->num = num;
    h->id = id;
    h->pool = p;
    h->ht = (rs_shash_head_t *) t + sizeof(rs_shash_t);

    rs_memzero(h->ht, sizeof(rs_shash_head_t) * num);

    return h;
}

int rs_shash_add(rs_shash_t *h, char *key, void *val) 
{
    uint32_t            i;
    int32_t             id;
    rs_shash_node_t     *n, *p;

    n = NULL;
    p = NULL;

    if(key == NULL) {
        rs_log_err(0, "rs_shash_add() failed, key is null");
        return RS_ERR;
    }

    i = rs_bkd_hash(key) % h->num;
    rs_log_debug(0, "rs_bdk_hash(%s), index = %u", key, i);
    
    for(n = h->ht[i].first; n != NULL; n = n->next) {
        if(rs_strncmp(key, n->key, rs_strlen(key)) == 0) {
            return RS_EXISTS; 
        }
    }

    id = rs_palloc_id(h->pool, sizeof(rs_shash_node_t));
    p = (rs_shash_node_t *) rs_palloc(h->pool, sizeof(rs_shash_node_t), id);

    if(p == NULL) {
        return RS_ERR;
    }

    p->id = id;
    p->key = key;
    p->val = val;
    p->next = NULL;
    p->prev = NULL;

    if(n != NULL)
        rs_shash_after_node(n, p);
    else
        h->ht[i].first = p;

    return RS_OK;
}

int rs_shash_get(rs_shash_t *h, char *key, void **val) 
{
    uint32_t        i;
    rs_shash_node_t *p;    

    if(key == NULL || val == NULL) {
        rs_log_err(0, "rs_shash_get() failed, (key|val) is NULL");
        return RS_ERR;
    }

    i = rs_bkd_hash(key) % h->num;
    rs_log_debug(0, "rs_bdk_hash(%s), index = %u", key, i);

    for(p = h->ht[i].first; p != NULL; p = p->next) {
        if(p->key != NULL && rs_strncmp(p->key, key, rs_strlen(key)) == 0) {
           *val = p->val; 
           return RS_OK;
        }
    }

    return RS_KEY_NOT_FOUND;
}

void rs_shash_free(rs_shash_t *h)
{
    uint32_t        i;  
    rs_shash_node_t *p;    

    for(i = 0; i < h->num; i++) {
        for(p = h->ht[i].first; p != NULL; p = p->next) {
            rs_pfree(h->pool, (void *) p, p->id);
        }
    }

    rs_pfree(h->pool, h, h->id);
}
