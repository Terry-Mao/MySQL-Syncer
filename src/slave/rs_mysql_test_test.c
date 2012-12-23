
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

typedef struct {
    int32_t col1;
    int32_t col2;
    int32_t col3;
    int32_t col4;
    int64_t col5;
    uint32_t col6;
    char col7[255 * 3 + 1];
    char col8[256 * 3 + 1];
    char col9[20 * 3 + 1];
    char col10[30 * 3 + 1];

} rs_mysql_test_t;

int32_t rs_mysql_test_pos[] = {
    offsetof(rs_mysql_test_t, col1),
    offsetof(rs_mysql_test_t, col2),
    offsetof(rs_mysql_test_t, col3),
    offsetof(rs_mysql_test_t, col4),
    offsetof(rs_mysql_test_t, col5),
    offsetof(rs_mysql_test_t, col6),
    offsetof(rs_mysql_test_t, col7),
    offsetof(rs_mysql_test_t, col8),
    offsetof(rs_mysql_test_t, col9),
    offsetof(rs_mysql_test_t, col10)
};


#define rs_mysql_test_t_init(test)                                           \
    (test)->col1 = 0;                                                        \
    (test)->col2 = 0;                                                        \
    (test)->col3 = 0;                                                        \
    (test)->col4 = 0;                                                        \
    (test)->col5 = 0;                                                        \
    (test)->col6 = 0;                                                        \
    rs_memzero((test)->col7, 255 * 3 + 1);                                   \
    rs_memzero((test)->col8, 256 * 3 + 1);                                   \
    rs_memzero((test)->col9, 20 * 3 + 1);                                    \
    rs_memzero((test)->col10, 30 * 3 + 1)


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
            "col1 : %d\n"
            "col2 : %d\n"
            "col3 : %d\n"
            "col4 : %d\n"
            "col5 : %ld\n"
            "col6 : %u\n"
            "col7 : %s\n"
            "col8 : %s\n"
            "col9 : %s\n"
            "col10 : %s\n"
            "\n==========================\n",
            test->col1,
            test->col2,
            test->col3,
            test->col4,
            test->col5,
            test->col6,
            test->col7,
            test->col8,
            test->col9,
            test->col10
                );
}

int rs_insert_test_test(rs_slave_info_t *si, void *obj)
{
    rs_mysql_test_t *test;
    uint32_t            cmdn;

    if(si == NULL || obj == NULL) {
        return RS_ERR;
    }  

    test = (rs_mysql_test_t *) obj;
    cmdn = 0;

    if(rs_redis_append_command(si, "SET test_%d %d", test->col1, 
                test->col2) != RS_OK)
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
