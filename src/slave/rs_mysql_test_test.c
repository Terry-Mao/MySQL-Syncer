
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

typedef struct {
    int32_t     id;
    rs_pstr_t   col;
} rs_mysql_test_t;

rs_dm_pos_alloc_t rs_mysql_test_pas[] = {
    { offsetof(rs_mysql_test_t, id), RS_DM_DATA_STACK, RS_DM_TYPE_DEF },
    { offsetof(rs_mysql_test_t, col), RS_DM_DATA_POOL, RS_DM_TYPE_DEF }
};


#define rs_mysql_test_t_init(test)                                           \
    (test)->id = 0;                                                          \
    rs_pstr_t_init(&(test->col))


static void rs_init_test_test(void *obj);
static void rs_print_test_test(void *obj);

static int rs_insert_test_test(rs_slave_info_t *si, void *obj);
static int rs_before_update_test_test(rs_slave_info_t *si, void *obj);
static int rs_update_test_test(rs_slave_info_t *si, void *obj);
static int rs_delete_test_test(rs_slave_info_t *si, void *obj);
static int rs_free_test_test(rs_slave_info_t *si, void *obj);

/* test */
static void rs_init_test_test(void *obj)
{
    rs_mysql_test_t         *test;
    test = (rs_mysql_test_t *) obj;

    rs_mysql_test_t_init(test);
}

static void rs_print_test_test(void *obj)
{
    rs_mysql_test_t         *test;


    test = (rs_mysql_test_t *) obj;


    rs_log_error(RS_LOG_DEBUG, 0,
            "\n========== test ==========\n"
            "id  : %d\n"
            "col : %.*s\n"
            "\n==========================\n",
            test->id,
            test->col.len,
            test->col.data
            );
}

static int rs_insert_test_test(rs_slave_info_t *si, void *obj)
{
    rs_mysql_test_t *test;

    test = (rs_mysql_test_t *) obj;

    if(rs_redis_append_command(si, "SET test_%d %s", test->id, test->col) 
            != RS_OK)
    {
        return RS_ERR;
    }

    return RS_OK;
}

static int rs_before_update_test_test(rs_slave_info_t *si, void *obj)
{
    return RS_OK;
}

static int rs_update_test_test(rs_slave_info_t *si, void *obj)
{
    return rs_insert_test_test(si, obj);
}

static int rs_delete_test_test(rs_slave_info_t *si, void *obj)
{
    return RS_OK;
}

static int rs_free_test_test(rs_slave_info_t *si, void *obj)
{
    rs_mysql_test_t *test;

    test = (rs_mysql_test_t *) obj;

    if(test->col.data != NULL) {
        rs_pfree(si->dpool, test->col.data, test->col.id);
    }

    return RS_OK;
}

int rs_dm_test_test(rs_slave_info_t *si, char *r, uint32_t rl, char t) {
    /* test.test */
    int             err;
    rs_mysql_test_t test;

    err = rs_dm_binlog_row(si, r, rl, t,
                rs_init_test_test,
                rs_print_test_test,
                rs_insert_test_test, 
                rs_before_update_test_test,
                rs_update_test_test, 
                rs_delete_test_test, 
                rs_free_test_test,
                rs_mysql_test_pas, &test);

    return err;
}
