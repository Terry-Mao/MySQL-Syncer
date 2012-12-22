
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


static void rs_parse_test_test(char *p, uint32_t dl, uint32_t i, void *obj);
static int rs_insert_test_test(rs_slave_info_t *si, void *obj);
static int rs_before_update_test_test(rs_slave_info_t *si, void *obj);
static int rs_update_test_test(rs_slave_info_t *si, void *obj);
static int rs_delete_test_test(rs_slave_info_t *si, void *obj);

/* test */
void rs_parse_test_test(char *p, uint32_t dl, uint32_t i, void *obj)
{
    rs_mysql_test_t         *test;

    test = (rs_mysql_test_t *) obj;

    switch(i) {
        /* col1 */
        case 0:
            rs_mysql_test_t_init(test);
            rs_memcpy(&(test->col1), p, dl);
            break;
        /* col2 */
        case 1:
            rs_memcpy(&(test->col2), p, dl);
            break;
        /* col3 */
        case 2:
            rs_memcpy(&(test->col3), p, dl);
            break;
        case 3:
            rs_memcpy(&(test->col4), p, dl);
            break;
        case 4:
            rs_memcpy(&(test->col5), p, dl);
            break;
        case 5:
            rs_memcpy(&(test->col6), p, dl);
            break;
        case 6:
            rs_memcpy(test->col7, p, dl);
            break;
        case 7:
            rs_memcpy(test->col8, p, dl);
            break;
        case 8:
            rs_memcpy(test->col9, p, dl);
            break;
        case 9:
            rs_memcpy(test->col10, p, dl);
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
            break;

        default:
            break;
    }
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
{/*{{{*/
    return rs_insert_test_test(si, obj);
}/*}}}*/

int rs_delete_test_test(rs_slave_info_t *si, void *obj)
{/*{{{*/
    return RS_OK;
}/*}}}*/


int rs_dml_test_test(rs_slave_info_t *si, char *e, uint32_t rl, char t) {
    /* test.test */
    rs_mysql_test_t test;

    return rs_dml_binlog_row(si, r, rl, t,
                rs_insert_test_test, 
                rs_before_update_test_test,
                rs_update_test_test, 
                rs_delete_test_test, 
                rs_parse_test_test, &test);
}
