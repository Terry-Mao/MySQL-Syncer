
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

int rs_tables_handle(rs_slave_info_t *si, char *p, uint32_t tl, char *e, 
        uint32_t rl, char t) 
{
    if(rs_strncmp(p, RS_FILTER_TABLE_TEST_TEST, tl) == 0) {

        if(rs_dml_test_test(si, e, rl, t) != RS_OK) {
            return RS_ERR;
        }
    } else {
        rs_log_err(0, "unknown row-based binlog filter table");
        return RS_ERR;
    }

    return RS_OK;
}

