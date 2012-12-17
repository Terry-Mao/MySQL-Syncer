
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

rs_shash_t *rs_init_shash(uint32_t size)
{

    uint32_t    i;
    rs_shash_t  *h;

    if(size == 0) {
        rs_log_err(0, "rs_init_shash() failed, size must great than zero");
        return NULL;
    }
    
    h = (rs_shash_t *) malloc(sizeof(rs_shash_t));

    if(h == NULL) {
        rs_log_err(0, "malloc() failed, sizeof(rs_shash_t)");
        return NULL;
    }

    h->size = size;

    h->ht = (rs_shash_head_t *) malloc(sizeof(rs_shash_head_t) * size);

    if(h->ht == NULL) {
        rs_log_err(0, "malloc() failed, sizeof(rs_shash_head_t) * size");
        return NULL;
    }

    for(i = 0; i < size; i++) {
        h->ht[i].first = NULL;
    }

    return h;
}

int rs_add_shash(char *key, void *val, rs_shash_t *h) 
{
    uint32_t            i;
    rs_shash_node_t     *n, *p;

    if(h == NULL || key == NULL || val == NULL) {
        rs_log_err(0, "rs_shash_add() failed, (key|val|h) is NULL");
        return RS_ERR;
    }

    i = rs_bkd_hash(key) % h->size;
    rs_log_debug(0, "rs_bdk_hash(%s) = %u, index = %d", key, rs_bkd_hash(key), 
            i);
    
    for(n = h->ht[i].first; n != NULL; n = n->next) {
        if(rs_strncmp(key, n->key, rs_strlen(key)) == 0) {
            return RS_EXISTS; 
        }
    }

    p = (rs_shash_node_t *) malloc(sizeof(rs_shash_node_t));

    if(p == NULL) {
        rs_log_err(rs_errno, "malloc() failed, sizeof(rs_shash_node_t)");
        return RS_ERR;
    }

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

int rs_get_shash(char *key, rs_shash_t *h, void **val) 
{
    uint32_t        i;
    rs_shash_node_t *p;    

    if(h == NULL || key == NULL || val == NULL) {
        rs_log_err(0, "rs_shash_get() failed, (key|h|val) is NULL");
        *val = NULL;
        return RS_ERR;
    }

    i = rs_bkd_hash(key) % h->size;

    rs_log_debug(0, "rs_bdk_hash(%s) = %u, index = %d", key, rs_bkd_hash(key), 
            i);

    for(p = h->ht[i].first; p != NULL; p = p->next) {
        if(p->key != NULL && rs_strncmp(p->key, key, rs_strlen(key)) == 0) {
           *val = p->val; 
           return RS_OK;
        }
    }

    *val = NULL;
    return RS_KEY_NOT_FOUND;
}

void rs_free_shash(rs_shash_t *h) 
{
    if(h != NULL) {
        if(h->ht != NULL) {
            free(h->ht);
        } 

        free(h);
    }
}
