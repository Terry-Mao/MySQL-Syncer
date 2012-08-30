
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>

typedef struct {
    uint32_t    id;
    char        col[30];
} rs_mysql_test_t;


#define rs_mysql_test_t_init(test)                                           \
    (test)->id = 0;                                                          \
    rs_memzero((test)->col, 30)

static char *rs_parse_test_test(char *p, u_char *ct, u_char *cm, uint32_t i, 
        void *obj);
static int rs_insert_test_test(rs_slave_info_t *si, void *obj);
static int rs_before_update_test_test(rs_slave_info_t *si, void *obj);
static int rs_update_test_test(rs_slave_info_t *si, void *obj);
static int rs_delete_test_test(rs_slave_info_t *si, void *obj);

/* test */
char *rs_parse_test_test(char *p, u_char *ct, u_char *cm, uint32_t i, 
        void *obj)
{/*{{{*/
    uint32_t        cl, sn;
    rs_mysql_test_t *test;

    cl = 0;
    sn = 0;
    test = (rs_mysql_test_t *) obj;

    switch(i) {
        /* ID */
        case 0:
            rs_mysql_test_t_init(test);
            rs_memcpy(&(test->id), p, 4);
            cl = 4;
            break;
        /* COL */
        case 1:
            rs_memcpy(&sn, p, 1);
            p += 1;

            rs_memcpy(test->col, p, sn);

            rs_log_debug(0,
                    "\n========== test ==========\n"
                    "ID             : %lu\n"
                    "COL            : %s\n"
                    "\n==============================\n",
                    test->id,
                    test->col
                    );

            cl = sn;

            break;

        default:
            break;
    }

    return p + cl;
}/*}}}*/

int rs_insert_test_test(rs_slave_info_t *si, void *obj)
{/*{{{*/
    rs_mysql_test_t *test;
    uint32_t            cmdn;

    if(si == NULL || obj == NULL) {
        return RS_ERR;
    }  

    test = (rs_mysql_test_t *) obj;
    cmdn = 0;

    if(rs_redis_append_command(si, "SET test_%u %s", test->id, 
                test->col) != RS_OK)
    {
        return RS_ERR;
    }

    cmdn++;

    return cmdn;
}/*}}}*/

int rs_before_update_test_test(rs_slave_info_t *si, void *obj)
{/*{{{*/
    return RS_OK;
}/*}}}*/

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

    return rs_dml_binlog_row(si, e, rl, t,
                rs_insert_test_test, 
                rs_before_update_test_test,
                rs_update_test_test, 
                rs_delete_test_test, 
                rs_parse_test_test, &test);
}
