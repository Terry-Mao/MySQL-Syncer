
#ifndef _RS_CONF_H_INCLUDED_
#define _RS_CONF_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>

#define RS_MAX_KEY_LEN          12
#define RS_MAX_VALUE_LEN        PATH_MAX
#define RS_MAX_MODULE_LEN       6

#define RS_CONF_BUFFER_LEN      1024
#define RS_CONF_DEFAULT_NUM     16

#define RS_CONF_NULL            0
#define RS_CONF_INT32           1 
#define RS_CONF_UINT32          2
#define RS_CONF_STR             3
#define RS_CONF_DOUBLE          4


#define RS_CONF_MODULE_MAX_LEN  20
#define RS_CONF_KEY_MAX_LEN     20
#define RS_CONF_VALUE_MAX_LEN   200


#define RS_CONF_COMMENT         '#'
#define RS_CONF_COMMENT_END     '\n'

#define RS_CONF_MODULE          '['
#define RS_CONF_MODULE_END      ']'

typedef struct {
    int     id;
    void    *data;
    int     data_id;
    int32_t type;
    int32_t found;
} rs_conf_data_t;


typedef rs_shash_t rs_conf_t;

rs_conf_t *rs_create_conf(rs_pool_t *p, uint32_t num);
int rs_init_conf(rs_conf_t *conf, char *path, char *name);
int rs_conf_register(rs_conf_t *c, char *key, void *data, int32_t type);
void rs_destroy_conf(rs_conf_t *c);

#endif
