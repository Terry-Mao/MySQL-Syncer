
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

typedef struct {
    int32_t id;
    char    col[10 * 3 + 1];
} rs_mysql_test_t;

int32_t rs_mysql_test_pos[] = {
    offsetof(rs_mysql_test_t, id),
    offsetof(rs_mysql_test_t, col)
};


#define rs_mysql_test_t_init(test)                                           \
    (test)->id = 0;                                                          \
    rs_memzero((test)->col, 10 * 3 + 1)


void rs_init_test_test(void *obj);
void rs_print_test_test(void *obj);

static int rs_insert_test_test(rs_slave_info_t *si, void *obj);
static int rs_before_update_test_test(rs_slave_info_t *si, void *obj);
static int rs_update_test_test(rs_slave_info_t *si, void *obj);
static int rs_delete_test_test(rs_slave_info_t *si, void *obj);

/* test */
void rs_init_test_test(void *obj)
{
    rs_mysql_test_t         *test;
    test = (rs_mysql_test_t *) obj;

    rs_mysql_test_t_init(test);
}

void rs_print_test_test(void *obj)
{
    rs_mysql_test_t         *test;
    test = (rs_mysql_test_t *) obj;
    rs_log_debug(0,
            "\n========== test ==========\n"
            "id  : %d\n"
            "col : %s\n"
            "\n==========================\n",
            test->id,
            test->col
                );
}

int rs_insert_test_test(rs_slave_info_t *si, void *obj)
{
    rs_mysql_test_t *test;
    uint32_t            cmdn;

    test = (rs_mysql_test_t *) obj;
    cmdn = 0;

    if(rs_redis_append_command(si, "SET test_%d %s", test->id, 
                test->col) != RS_OK)
    {
        return RS_ERR;
    }

    cmdn++;

    return cmdn;
}

int rs_before_update_test_test(rs_slave_info_t *si, void *obj)
{
    return RS_OK;
}

int rs_update_test_test(rs_slave_info_t *si, void *obj)
{
    return rs_insert_test_test(si, obj);
}

int rs_delete_test_test(rs_slave_info_t *si, void *obj)
{
    return RS_OK;
}


int rs_dml_test_test(rs_slave_info_t *si, char *r, uint32_t rl, char t) {
    /* test.test */
    rs_mysql_test_t test;

    return rs_dml_binlog_row(si, r, rl, t,
                rs_init_test_test,
                rs_print_test_test,
                rs_insert_test_test, 
                rs_before_update_test_test,
                rs_update_test_test, 
                rs_delete_test_test, 
                rs_mysql_test_pos, &test);
}
