
#ifndef _RS_REGISTER_TABLES_H_INCLUDED_
#define _RS_REGISTER_TABLES_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>


typedef int (*rs_redis_dml_func) (rs_slave_info_t *, char *, uint32_t, char);

typedef struct {
    char                *key;
    rs_redis_dml_func   handle;

} rs_dml_table_func;


extern rs_dml_table_func rs_dml_table_funcs[];

int rs_register_tables(rs_slave_info_t *si);

/* MYSQL : test.test */
int rs_dml_test_test(rs_slave_info_t *si, char *r, uint32_t rl, char t);

#endif
