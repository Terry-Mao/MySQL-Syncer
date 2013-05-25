
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

/*
    TODO
    Read binlog file, save data in rs_binlog_info_t
*/
//int rs_read_binlog(rs_binlog_ifo_t *bi) 
//{
//    int r;
//
//    for( ;; ) {
//        if((r = rs_binlog_header_handler(bi)) != RS_OK) {
//            goto free;
//        }    
//
//        if((r = rs_binlog_funcs[idx](bi)) != RS_OK) {
//            goto free;
//        }
//
//        if((r = rs_binlog_finish_handler(bi)) != RS_OK) {
//            break; 
//        }
//    }
//
//free: 
//    return r;
//}

static int rs_has_next_binlog(rs_binlog_info_t *bi);

int rs_read_binlog(rs_binlog_info_t *bi, void *buf, uint32_t size) 
{
    int         nfd, wd1, wd2, r;
    uint32_t    n, tl; 
    ssize_t     l;
    struct      inotify_event *e;
    char        eb[1024], *p, *f;

    nfd = -1;
    r = RS_ERR;
    // use io buffer
    f = buf + rs_get_tmpbuf(bi->io_buf, buf, size);
    while((f - buf) != size) {
        // read fd and buffer it
        n = rs_read_tmpbuf(bi->io_buf, bi->fd);
        if (n == -1) {
            rs_log_error(RS_LOG_ERR, rs_errno, "rs_read() failed, %s", 
                    bi->dump_file); 
            goto free;
        }
        else if(n > 0) {
            f += rs_get_tmpbuf(bi->io_buf, f, size - ms);
            continue;
        }

        // check has new binlog
        r = rs_has_next_binlog(bi)
        if(r == RS_ERR) {
            goto free;
        } else if(r == RS_OK) {
            r = RS_HAS_BINLOG;
            goto free;
        }

        /* set inotify */
        if(nfd == -1) {
            nfd = rs_init_io_watch();
            if(nfd < 0) {
                goto free;
            }

            wd1 = rs_add_io_watch(nfd, bi->dump_file, RS_IN_MODIFY);
            wd2 = inotify_add_watch(nfd, bi->idx_file, RS_IN_MODIFY);
            if(wd1 < 0 || wd2 < 0) {
                goto free;
            }

            // retry read binlog file and check has new binlog
            // binlog file changed or has new binlog before set inotify
            continue;
        }

        // wait file changed
        if(rs_select(nfd) == RS_ERR) {
            goto free;
        }

        l = rs_read(nfd, eb, 1024);
        if(l <= 0) {
            goto free;
        }

        p = eb;
        e = (struct inotify_event *) eb;
        if(e == NULL) {
            goto free;
        }

        while (((char *) e - eb) < l) {
            if (e->wd == wd1 && e->mask & RS_IN_MODIFY) {
                break;
            } else if(e->wd == wd2 && e->mask & RS_IN_MODIFY) {
                break;
            }

            tl = sizeof(struct inotify_event) + e->len;
            e = (struct inotify_event *) (p + tl); 

            if(e == NULL) {
                break;
            }

            p += tl;
        }
    }

    r = RS_OK; 
free:
    if(nfd > 0) {
        if(close(nfd) != 0) {
            rs_log_error(RS_LOG_ERR, rs_errno, "close() failed"); 
            r = RS_ERR;
        }
    }

    return r;
}

/*
    Get next binlog from binlog index file.
*/
static int rs_has_next_binlog(rs_binlog_info_t *bi)
{
    ssize_t     l;
    char        *p;

    p = bi->dump_file;
    l = rs_read(bi->idx_fd, bi->dump_file, 1);
    if(l == 0) {
        return RS_NO_BINLOG;
    } else if (l == -1) {
        return RS_ERR;
    }

    p += l;
    while(*(p - 1) != '\n') {
        l = rs_read(bi->idx_fd, p, 1);
        if(l == 0) {
            usleep(10000);
            continue;
        } else (l == -1) {
            return RS_ERR;
        }

        p += l;
    }
    
    *(p - 1) = '\0';
    bi->dump_pos = 0;
    return RS_OK;
}
