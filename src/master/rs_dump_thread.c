
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>





void rs_destory_dump_thread(rs_dump_thread_t *t) 
{   
    int err;

    rs_free_all_dump_thread(t);

    t->thread_n = 0;
    t->free_thread_n = 0;
    t->free_threads = NULL;

    if((err = pthread_mutex_destroy(&(t->dump_thread_mutex))) != 0) {
        rs_log_err(err, "pthread_mutex_destroy failed, dump_thread_mutex");
    }

    free(t->threads);
    free(t);
}
