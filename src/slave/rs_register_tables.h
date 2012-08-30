
#ifndef _RS_REGISTER_TABLES_H_INCLUDED_
#define _RS_REGISTER_TABLES_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

/* MYSQL : test.test */
#define RS_FILTER_TABLE_TEST_TEST   "test.test"
int rs_dml_test_test(rs_slave_info_t *si, char *e, uint32_t rl, char t);

#endif
