

#include <rs_config.h>
#include <rs_core.h>

static int rs_conf_apply(rs_conf_t *c, char *key, char *val); 

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
    uint32_t    i, m_len;
    ssize_t     n;
    char        *s, c, k[RS_MAX_KEY_LEN + 1], v[RS_MAX_VALUE_LEN + 1], 
                m[RS_MAX_MODULE_LEN + 1], buf[RS_CONF_BUFFER_LEN], 
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

                /* set value */
                if(rs_conf_apply(conf, k, v) != RS_OK) {
                    goto free;
                }

                value = 0;
                key = 1;
                p = k;
                i = 0;

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

rs_conf_t *rs_create_conf(rs_pool_t *p, uint32_t num)
{
    return rs_create_shash(p, num);
}

int rs_conf_register(rs_conf_t *c, char *key, void *data, int32_t type)
{
    int             id, err;
    rs_conf_data_t  *d;

    id = rs_palloc_id(c->pool, sizeof(rs_conf_data_t));
    d = rs_palloc(c->pool, sizeof(rs_conf_data_t), id);

    if(d == NULL) {
        return RS_ERR;
    }

    d->id = id;
    d->data = data;
    d->type = type;
    d->found = 0;

    if((err = rs_shash_add(c, key, d)) != RS_OK) {
        if(err == RS_EXISTS) {
            rs_log_err(0, "add a exists conf key \"%s\"", key);
        }

        return RS_ERR;
    }
    
    rs_log_debug(0, "add conf key %s, type %d", key, type);

    return RS_OK;
}

static int rs_conf_apply(rs_conf_t *c, char *key, char *val) 
{
    rs_conf_data_t  *d;
    size_t          len;
    char            *t;

    if(rs_shash_get(c, key, (void *) &d) != RS_OK) {
        rs_log_err(0, "unknown conf key \"%s\"", key);
        return RS_ERR;
    }

    switch(d->type) {
        case RS_CONF_INT32:
            *((int32_t *) d->data) = rs_str_to_int32(val);
            break;

        case RS_CONF_UINT32:
            *((uint32_t *) d->data) = rs_str_to_uint32(val);
            break;

        case RS_CONF_DOUBLE:
            *((double *) d->data) = rs_str_to_double(val);
            break;

        case RS_CONF_STR:
            len = rs_strlen(val) + 1;
            d->data_id = rs_palloc_id(c->pool, len);
            t = rs_palloc(c->pool, len, d->data_id);

            if(t == NULL) {
                return RS_ERR;
            }

            rs_memcpy(t, val, len);

            *((char **) d->data) = t;
            break;

        default:
            return RS_ERR;
    }

    d->found = 1;

    return RS_OK;
}

void rs_destroy_conf(rs_conf_t *c)
{
    uint32_t        i;  
    rs_shash_node_t *p;    
    rs_conf_data_t  *d;

    for(i = 0; i < c->num; i++) {
        for(p = c->ht[i].first; p != NULL; p = p->next) {
            d = (rs_conf_data_t *) p->val;

            if(d->type == RS_CONF_STR && d->found == 1) {
                rs_pfree(c->pool, d->data, d->data_id);
            }

            rs_pfree(c->pool, d, d->id);
        }
    }

    rs_destroy_shash(c);
}
