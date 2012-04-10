
#ifndef _RS_DUMP_THREAD_H_INCLUDED_
#define _RS_DUMP_THREAD_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

struct rs_dump_thread_data_s {

    pthread_t               thread;     /* thread id */
    int                     cli_fd;     /* cli socket fd*/
    rs_request_dump_t       *req_dump;  /* request dump info */

    rs_dump_thread_data_t   *data;      /* next free thread */
    unsigned                open:1;     /* thread in use*/

};

struct rs_dump_thread_s {

    rs_dump_thread_data_t   *threads;           /* threads */
    rs_dump_thread_data_t   *free_threads;      /* free thread pointer */
    uint32_t                thread_n;           /* total thread number */
    uint32_t                free_thread_n;      /* free thread number */

    pthread_mutex_t         dump_thread_mutex;  /* mutex for alloc thread */

};

int     rs_init_dump_thread(rs_dump_thread_t *t, uint32_t n);
void    rs_free_dump_thread(rs_dump_thread_t *t, rs_dump_thread_data_t *d);
void    rs_free_all_dump_thread(rs_dump_thread_t *t); 
void    rs_destory_dump_thread(rs_dump_thread_t *t); 

rs_dump_thread_data_t   *rs_get_dump_thread(rs_dump_thread_t *t);


#endif
