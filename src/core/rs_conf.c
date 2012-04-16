

#include <rs_config.h>
#include <rs_core.h>

static int rs_get_conf_info(rs_conf_t *c, char *k, char *t, uint32_t *idx);
static int rs_get_conf_str(char **dst, char *value);
static int rs_get_conf_uint(uint32_t *dst, char *value);
static int rs_get_conf_int(int32_t *dst, char *value);
static int rs_get_conf_double(double *dst, char *value);
static int rs_set_conf_value(rs_conf_v_t *v, char *src, char type); 

char *rs_conf_path = NULL;

/*
 * DESCRIPTION
 *    Init configure file.
 *    *path is the configure file path, *name is the name of config section
 *    *kv is "care about" config key.
 *
 * RETURN VALUE
 *    On Success RS_OK is returned, On error RS_ERR is returned.
 *
 */
int rs_init_conf(rs_conf_t *conf, char *path, char *name) 
{
    int         fd, comment, key, value, module, eof, r;
    uint32_t    i, idx, m_len;
    ssize_t     n;
    char        *s, c, k[RS_MAX_KEY_LEN + 1], v[RS_MAX_VALUE_LEN + 1], 
                m[RS_MAX_MODULE_LEN + 1], t, buf[RS_CONF_BUFFER_LEN], 
                *last, *p;

    i = 0;
    fd = -1;
    comment = 0;
    key = 0;
    value = 0;
    module = 0;
    eof = 0;
    r = RS_ERR;
    last = buf + RS_CONF_BUFFER_LEN;
    m_len = rs_strlen(name);
    p = NULL;

    if(path == NULL) {
        rs_log_err(0, "conf path is null, argument *path");
        return RS_ERR;
    }

    if(name == NULL) {
        rs_log_err(0, "module is null");
        return RS_ERR;
    }

    rs_log_debug(0, "conf file = %s, module = %s", path, name);

    fd = open(path, O_CREAT | O_RDONLY, 00666);

    if(fd == -1) {
        rs_log_err(rs_errno, "rs_init_conf() failed, %s", path);
        goto free;
    }

    for( ;; ) {

        n = rs_read(fd, buf, RS_CONF_BUFFER_LEN);

        if(n == -1) {
            goto free;
        }

        if(n == 0) {
            eof = 1;
        }

        last = buf + n;
        s = buf;

        while(s < last) { 

            c = *s++;

            /* comment start */
            if(c == RS_CONF_COMMENT && comment == 0) {
                comment = 1;
                continue;
            }

            /* comment end */
            if(comment == 1 && (c == RS_CONF_COMMENT_END || eof == 1)) 
            {
                comment = 0;
                continue;
            }

            /* skip comment char */
            if(comment == 1) {
                continue;
            }

            /* module start */
            if(c == RS_CONF_MODULE) {
                module = 1; 
                i = 0;
                value = 0;
                key = 0;
                p = m;
                continue;
            }

            /* module end */
            if(module == 1) {

                if(i > RS_CONF_MODULE_MAX_LEN) {
                    r = RS_ERR;
                    rs_log_err(0, "conf module name is too long, "
                            "max length = %u", RS_CONF_MODULE_MAX_LEN);
                    goto free;
                }

                if(c == '\n' || eof == 1) {
                    rs_log_err(0, "unexcepted end of module, expecting \"]\"");
                    goto free;
                }

                if(c != RS_CONF_MODULE_END) {
                    *p++ = c;
                    i++;
                    continue;
                }

                *p = '\0';
                module = 0;

                if(rs_strncmp(m, name, m_len) == 0) {
                    key = 1;
                    p = k;
                    i = 0;
                }

                continue; 
            }

            /* key */
            if(key == 1) {

                if(p == k && (c == '\n' || c == ' ' || c == '\t')) {
                    continue;
                }

                if(i > RS_CONF_KEY_MAX_LEN) {
                    rs_log_err(0, "conf key too long, max length = %u", 
                            RS_CONF_KEY_MAX_LEN);
                    goto free;
                }

                if((c == '\n' || eof == 1) && p != k) {
                    rs_log_err(0, "unexcepted end of key, expecting \" \"");
                    goto free;
                }

                if(c != ' ') {
                    *p++ = c;
                    i++;
                    continue;
                }

                *p = '\0';
                value = 1;
                key = 0;
                p = v;
                continue;
            }

            /* value */
            if(value == 1) {

                if(p == v && (c == '\n' || c == ' ' || c == '\t')) {
                    continue;
                }

                if(i > RS_CONF_VALUE_MAX_LEN) {
                    rs_log_err(0, "conf value too long, max length = %u",
                            RS_CONF_VALUE_MAX_LEN);
                    goto free;
                }

                if(c != '\n' && eof == 0) {
                    *p++ = c;
                    i++;
                    continue;
                }

                *p = '\0';

                /* get conf index */
                if(rs_get_conf_info(conf, k, &t, &idx) != RS_OK) {
                    rs_log_err(0, "unknow conf key \"%s\"", k);
                    goto free;
                }

                /* set value */
                if(rs_set_conf_value(&(conf->kv[idx].v), v, t) != RS_OK) {
                    goto free;
                }

                value = 0;
                key = 1;
                p = k;
                i = 0;

                rs_log_debug(0, "get conf key = %s, type = %d, value = %s", 
                        conf->kv[idx].k.data, conf->kv[idx].v.type, v);

                continue;
            }
        }

        if(eof == 1) {
            r = RS_OK;
            goto free;
        }

    }

free :

    if(fd) {
        rs_close(fd);
    }

    return r;
}

int rs_add_conf_kv(rs_conf_t *c, char *key, void *value, char type)
{
    uint32_t        size, i;
    size_t          len;
    rs_conf_kv_t    *kv;

    len = rs_strlen(key);

    for(i = 0; i < c->conf_idx; i++) {
        if(c->kv[i].k.len == len && rs_strncmp(key, c->kv[i].k.data, len) 
                == 0) 
        {
            rs_log_err(rs_errno, "config file has a dupication key, %s", key);
            return RS_ERR;
        }
    }

    if(c->kv == NULL || c->conf_n == c->conf_idx) {
        /* resize kv*/    
        size = (c->kv == NULL) ? RS_CONF_DEFAULT_NUM : c->conf_n * 2;
        c->kv = realloc(c->kv, size * sizeof(rs_conf_kv_t));

        if(c->kv == NULL) {
            rs_log_err(rs_errno, "realloc() failed, rs_conf_kv_t");
            return RS_ERR;
        }

        c->conf_n = size;
    }



    kv = &(c->kv[c->conf_idx++]);

    kv->k.data = key;
    kv->k.len = rs_strlen(key);
    kv->v.data = value;
    kv->v.type = type;

    rs_log_debug(0, "key = %s, conf_idx = %u, conf_n = %u", kv->k.data, 
            c->conf_idx, c->conf_n);

    return RS_OK;
}

static int rs_get_conf_info(rs_conf_t *c, char *k, char *t, uint32_t *idx) 
{
    uint32_t    i, found;
    size_t      len;

    found = 0;
    len = rs_strlen(k);

    for(i = 0; i < c->conf_idx; i++) {
        /*
           rs_log_debug(0, "key = %s, ken_len = %u, conf_idx = %u, i = %u", 
           c->kv[i].k.data, c->kv[i].k.len, c->conf_idx, i);
           */

        if(c->kv[i].k.len == len && rs_strncmp(k, c->kv[i].k.data, len) == 0) 
        {
            *idx = i;
            *t = c->kv[i].v.type;
            found = 1;
            break;
        }
    }

    if(found) {
        return RS_OK;
    }

    return RS_ERR;
}

static int rs_set_conf_value(rs_conf_v_t *v, char *src, char type) 
{
    int     r;

    r = RS_OK;

    switch(type) {

        case RS_CONF_INT32:
            r = rs_get_conf_int(v->data, src);
            break;

        case RS_CONF_UINT32:
            r = rs_get_conf_uint(v->data, src);
            break;

        case RS_CONF_STR:
            r = rs_get_conf_str(v->data, src);
            break;

        case RS_CONF_DOUBLE:
            r = rs_get_conf_double(v->data, src);
            break;

        default:
            r = RS_ERR;
    }

    return r;
}

static int rs_get_conf_str(char **dst, char *value) 
{
    size_t  len;
    char    *p;

    len = rs_strlen(value);
    p = malloc(len + 1);

    if(p == NULL) {
        rs_log_err(rs_errno, "malloc() failed");
        return RS_ERR;
    }

    rs_memcpy(p, value, len + 1);
    *dst = p;

    return RS_OK;
}

static int rs_get_conf_uint(uint32_t *dst, char *value) 
{
    *dst = rs_str_to_uint32(value);
    return RS_OK;
}

static int rs_get_conf_double(double *dst, char *value) 
{
    *dst = rs_str_to_double(value);
    return RS_OK;
}


static int rs_get_conf_int(int32_t *dst, char *value) 
{
    *dst = rs_str_to_int32(value);
    return RS_OK;
}

void rs_free_conf(rs_conf_t *c)
{
    int     i;
    void    *p;

    p = NULL;

    if(c != NULL || c->kv != NULL) {
        for(i = 0; c->kv[i].k.data; i++) {
            if(c->kv[i].v.type == RS_CONF_STR) {
                p = (void *) *((void **) c->kv[i].v.data);
                if(p != NULL) {
                    free(p);
                }
            }
        }
    }
}
