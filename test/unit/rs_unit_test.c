
#include <rs_config.h>
#include <rs_core.h>
#include <CUnit/Basic.h>


/* static int rs_get_options(int argc, char *const *argv); */
static int rs_init_suite(void);
static int rs_clean_suite(void);


static void rs_pool_test(void);
static void rs_conf_test(void);
static void rs_shash_test(void);
static void rs_buf_test(void);


/* static rs_core_info_t  *ci; */
int main(int argc, char *const *argv)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* add a suite to the registry */
    pSuite = CU_add_suite("rs_unit_test", rs_init_suite, rs_clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* add the tests to the suite */
    /* NOTE - ORDER IS IMPORTANT - MUST TEST fread() AFTER fprintf() */
    if ((NULL == CU_add_test(pSuite, "test of rs_pool_test", 
                    rs_pool_test)) || 
            (NULL == CU_add_test(pSuite, "test of rs_conf_test",
                                 rs_conf_test)) ||
            (NULL == CU_add_test(pSuite, "test of rs_shash_test", 
                                 rs_shash_test)) ||
            (NULL == CU_add_test(pSuite, "test of rs_buf_test", 
                                 rs_buf_test))
       )
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}

static int rs_init_suite(void)
{
    /* init rs_errno */
    if(rs_init_strerror() != RS_OK) {
        return 1;
    }

#if 0
    /* init core info */
    if((ci = rs_init_core_info(NULL)) == NULL) {
        return 1;
    }

    rs_core_info = ci;
#endif

    return 0;
}

static int rs_clean_suite(void)
{
    rs_free_strerr();
    return 0;
}


static void rs_shash_test(void) 
{
    rs_pool_t       *p;
    rs_shash_t      *h;  

    CU_ASSERT((p = rs_create_pool(40, 1024 * 1024 * 10, rs_pagesize, 
                    RS_POOL_CLASS_IDX, 1.5, RS_POOL_PREALLOC)) != NULL);
    CU_ASSERT((h = rs_create_shash(p, 3)) != NULL);


    int val = 10, val1 = 11, val2 = 12, val3= 13;
    CU_ASSERT(RS_OK == rs_shash_add(h, "test1", (void *) &val));
    CU_ASSERT(RS_OK == rs_shash_add(h, "test2", (void *) &val1));
    CU_ASSERT(RS_EXISTS == rs_shash_add(h, "test2", (void *) &val1));
    CU_ASSERT(RS_OK == rs_shash_add(h, "test3", (void *) &val2));
    CU_ASSERT(RS_OK == rs_shash_add(h, "test4", (void *) &val3));

    int *v1, *v2, *v3, *v4;

    CU_ASSERT(RS_OK == rs_shash_get(h, "test1", (void *) &v1));
    CU_ASSERT(RS_OK == rs_shash_get(h, "test2", (void *) &v2));
    CU_ASSERT(RS_OK == rs_shash_get(h, "test3", (void *) &v3));
    CU_ASSERT(RS_OK == rs_shash_get(h, "test4", (void *) &v4));

    CU_ASSERT(*v1 == 10);
    CU_ASSERT(*v2 == 11);
    CU_ASSERT(*v3 == 12);
    CU_ASSERT(*v4 == 13);

    rs_destroy_shash(h);
    rs_destroy_pool(p);
}

static void rs_conf_test(void)
{
    return;
    rs_conf_t   conf;
    char        *conf_path;
    uint32_t    test, test2;
    char        *test1;

    conf_path = "etc/test.cf";
    test1= NULL;
    test = 0;
    test2= 0;

    rs_conf_t_init(&conf);

    CU_ASSERT(rs_add_conf_kv(&conf, "test", &test, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test1", &test1, RS_CONF_STR) == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test2", &test2, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test3", &test2, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test4", &test2, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test5", &test2, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test6", &test2, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test7", &test2, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test8", &test2, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test9", &test2, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test10", &test2, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test11", &test2, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test12", &test2, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test13", &test2, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test14", &test2, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test15", &test2, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test16", &test2, RS_CONF_UINT32) 
            == RS_OK);
    CU_ASSERT(rs_add_conf_kv(&conf, "test16", &test2, RS_CONF_UINT32) 
            == RS_ERR);

    CU_ASSERT(rs_init_conf(&conf, conf_path, RS_CORE_MODULE_NAME) == RS_OK);
    CU_ASSERT(test == 1);
    CU_ASSERT(rs_strcmp("abc", test1) == 0);
    CU_ASSERT(test2 == 1234);

    rs_free_conf(&conf);
}

static void rs_pool_test(void)
{
    rs_pool_t       *p;
    int             id, i;
    char            *t;

    t = NULL;

    // overflow test
    /* init slab  */
    CU_ASSERT((p = rs_create_pool(100, rs_pagesize, rs_pagesize, 
                    RS_POOL_CLASS_IDX, 1.5, RS_POOL_PREALLOC)) != NULL);

    /* alloc slab */
    CU_ASSERT((id = rs_palloc_id(p, 104)) >= 0);

    for(i = 0; i < 39; i++) {
        CU_ASSERT((t = rs_palloc(p, 104, id)) != NULL); 
    }

    CU_ASSERT((t = rs_palloc(p, 104, id)) == NULL); 

    rs_destroy_pool(p);
 
    // new slab test
    /* init slab  */
    CU_ASSERT((p = rs_create_pool(100, rs_pagesize * 2, rs_pagesize, 
                    RS_POOL_CLASS_IDX, 1.5, RS_POOL_PREALLOC)) != NULL);

    /* alloc slab */
    CU_ASSERT((id = rs_palloc_id(p, 104)) >= 0);

    for(i = 0; i < 40; i++) {
        CU_ASSERT((t = rs_palloc(p, 104, id)) != NULL); 
    }

    CU_ASSERT(p->slab_class[id].used_slab == 2);
    CU_ASSERT(p->slab_class[id].total_slab == 16);

    CU_ASSERT((id = rs_palloc_id(p, 1024*1024*1 + 1)) == RS_SLAB_OVERFLOW);
    CU_ASSERT((t = rs_palloc(p, 1024*1024*1 + 1, id)) != NULL);
    rs_pfree(p, t, id);


    rs_destroy_pool(p);

    // pagealloc test
    /* init slab  */
    CU_ASSERT((p = rs_create_pool(100, rs_pagesize * 2, rs_pagesize, 
                    RS_POOL_CLASS_IDX, 1.5, RS_POOL_PAGEALLOC)) != NULL);

    /* alloc slab */
    CU_ASSERT((id = rs_palloc_id(p, 104)) >= 0);

    for(i = 0; i < 40; i++) {
        CU_ASSERT((t = rs_palloc(p, 104, id)) != NULL); 
    }

    CU_ASSERT(p->slab_class[id].used_slab == 2);
    CU_ASSERT(p->slab_class[id].total_slab == 16);

    CU_ASSERT((id = rs_palloc_id(p, 1024*1024*1 + 1)) == RS_SLAB_OVERFLOW);
    CU_ASSERT((t = rs_palloc(p, 1024*1024*1 + 1, id)) != NULL);
    rs_pfree(p, t, id);


    rs_destroy_pool(p);
}

static void rs_buf_test(void)
{
    int                 i;
    rs_buf_t            *b;
    rs_ringbuf_t        *rb;
    rs_ringbuf_data_t   *d;
    rs_pool_t           *p;

    b = NULL;
    rb = NULL;
    d = NULL;
    p = NULL;

    CU_ASSERT((b = rs_create_tmpbuf(100)) != NULL);
    rs_destroy_tmpbuf(b);

    CU_ASSERT((p = rs_create_pool(40, 1024 * 1024 * 10, rs_pagesize, 
                    RS_POOL_CLASS_IDX, 1.5, RS_POOL_PREALLOC)) != NULL);
    CU_ASSERT((rb = rs_create_ringbuf(p, 10)) != NULL);

    /* get ring buffer2 empty */
    CU_ASSERT(rs_ringbuf_get(rb, &d) == RS_EMPTY);

    /* set ring buffer2 */
    for(i = 0; i < 10; i++) {
        CU_ASSERT(rs_ringbuf_set(rb, &d) == RS_OK);
        rs_ringbuf_set_advance(rb);
    }

    /* set ring buffer2 full */
    CU_ASSERT(rs_ringbuf_set(rb, &d) == RS_FULL);

    /* get ring buffer2 */
    for(i = 0; i < 10; i++) {
        CU_ASSERT(rs_ringbuf_get(rb, &d) == RS_OK);
        rs_ringbuf_get_advance(rb);
    }

    CU_ASSERT(rs_ringbuf_get(rb, &d) == RS_EMPTY);

    rs_destroy_ringbuf(rb);
    rs_destroy_pool(p);
}

#if 0
static int rs_get_options(int argc, char *const *argv) 
{
    char   *p;
    int     i;

    for (i = 1; i < argc; i++) {

        p = argv[i];

        if (*p++ != '-') {
            rs_log_stderr(0, "invalid option: \"%s\"", argv[i]);
            return RS_ERR;
        }

        while (*p) {

            switch (*p++) {
                case 'c':
                    if (*p) {
                        rs_conf_path = p;
                        goto next;
                    }

                    if (argv[++i]) {
                        rs_conf_path = argv[i];
                        goto next;
                    }

                    rs_log_stderr(0, "option \"-c\" requires config file");
                    return RS_ERR;

                default:
                    rs_log_stderr(0, "invalid option: \"%c\"", *(p - 1));
                    return RS_ERR;
            }
        }

next:

        continue;
    }

    return RS_OK;
}
#endif
