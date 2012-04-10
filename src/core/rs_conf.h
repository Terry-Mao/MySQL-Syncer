
#ifndef _RS_CONF_H_INCLUDED_
#define _RS_CONF_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>

#define RS_CONF_BUFFER_LEN  10

#define RS_CONF_NULL      0
#define RS_CONF_INT32     1 
#define RS_CONF_UINT32    2
#define RS_CONF_STR       3


#define RS_CONF_MODULE_MAX_LEN  6
#define RS_CONF_KEY_MAX_LEN     12
#define RS_CONF_VALUE_MAX_LEN   PATH_MAX


#define RS_CONF_COMMENT         '#'
#define RS_CONF_COMMENT_END     '\n'

#define RS_CONF_MODULE          '['
#define RS_CONF_MODULE_END      ']'

#define rs_conf_value(d, t)     { (void *) &d, t }
#define rs_conf_v_set(v, vp, t)                                              \
    (v)->data = (void *) &(vp); (v)->type = t

#define rs_null_conf            { NULL, RS_CONF_NULL }


typedef struct {
    void            *data;
    char            type;
} rs_conf_v_t;


typedef struct {
    rs_str_t        k;
    rs_conf_v_t     v;
} rs_conf_kv_t;


int rs_init_conf(char *path, char *module, rs_conf_kv_t *kv);
void rs_free_conf(rs_conf_kv_t *kv);

#endif
