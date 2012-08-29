

#ifndef _RS_PROCESS_H_INCLUDED_
#define _RS_PROCESS_H_INCLUDED_

#include <rs_config.h>
#include <rs_core.h>

int rs_init_daemon(rs_core_info_t *ci);

int rs_init_signals(sigset_t *waitset);
void rs_sig_handle(int signum);

int rs_create_pidfile(char *name); 
void rs_delete_pidfile(char *name);

int rs_init_uid(char *user);
int rs_init_gid(char *grp);

#endif
