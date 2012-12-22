
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

int rs_register_tables(rs_slave_info_t *si) 
{
    si->table_func = rs_create_shash(si->pool, RS_TABLE_FUNC_NUM);
    
    if(si->table_func == NULL) {
        return RS_ERR;
    }

    if(rs_shash_add(si->table_func, "test.test", (void *) rs_mysql_test_test) 
            != RS_OK) 
    {
        return RS_ERR;
    }

    return RS_OK;
}

