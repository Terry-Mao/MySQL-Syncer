
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

rs_dm_table_func rs_dm_table_funcs[] = {
    { "test.test", rs_dm_test_test },
    { NULL, NULL } /* terminated, don't delete */
};

int rs_register_tables(rs_slave_info_t *si) 
{
    uint32_t            i;
    rs_dm_table_func   f;

    i = 0;

    for(f = rs_dm_table_funcs[i]; f.key != NULL && f.handle != NULL; 
            f = rs_dm_table_funcs[++i]);

    si->table_func = rs_create_shash(si->pool, i * 2);
    
    if(si->table_func == NULL) {
        return RS_ERR;
    }

    i = 0;

    for(f = rs_dm_table_funcs[i]; f.key != NULL && f.handle != NULL; 
            f = rs_dm_table_funcs[++i]) 
    {
        rs_shash_add(si->table_func, f.key, f.handle);
    }

    return RS_OK;
}
