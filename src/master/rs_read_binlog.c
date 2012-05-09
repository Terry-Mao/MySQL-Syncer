
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

int rs_read_binlog(rs_request_dump_t *rd) 
{
    int     r, err, i;

    i = 0;

    for( ;; ) {

        /* HEADER */
        if((r = rs_binlog_header_event(rd)) != RS_OK) {
            goto free;
        }    

        switch(rd->binlog_info.t) {
        /* ROTATE_EVENT | STOP_EVENT */
        case RS_ROTATE_EVENT:
        case RS_STOP_EVENT:
            for( ;; ) {
                if((err = rs_has_next_binlog(rd)) == RS_OK) {
                    r = RS_HAS_BINLOG;
                    goto free;
                }

                if(err == RS_ERR) {
                    r = RS_ERR;
                    goto free;
                }

                if(i % 60 == 0) {
                    rs_log_info("no new binlog file"); 
                    i = 0;
                }

                sleep(RS_BINLOG_EOF_WAIT_SEC);
                
                i += RS_BINLOG_EOF_WAIT_SEC;
            }

            break;
        case RS_QUERY_EVENT:
            /* QUERY_EVENT */
            if((r = rs_binlog_query_event(rd)) != RS_OK) {
                goto free;
            }
            break;
            /* INTVAR EVNET */
        case RS_INTVAR_EVENT:
            if((r = rs_binlog_intvar_event(rd)) != RS_OK) {
                goto free;
            }
            break;
        case RS_XID_EVENT:
            /* COMMIT EVENT */
            if((r = rs_binlog_xid_event(rd)) != RS_OK) {
                goto free;
            }
            break;
        case RS_TABLE_MAP_EVENT:
            /* TABLE_MAP EVENT */
            if((r = rs_binlog_table_map_event(rd)) != RS_OK) {
                goto free;
            }
            break;
        case RS_WRITE_ROWS_EVENT:
            /* WRITE_ROWS EVENT */
            if((r = rs_binlog_write_rows_event(rd)) != RS_OK) {
                goto free;
            }
            break;
        default:
            /* SKIP EVENTS */
            break;
        }

        /* FINISH */
        if((r = rs_binlog_finish_event(rd)) != RS_OK) {
            break; 
        }
    }

free:

    return r;
}

int rs_eof_read_binlog(rs_request_dump_t *rd, void *buf, size_t size) 
{
    int         r, err, mfd, wd1, wd2, ready, i;
    size_t      n, tl;
    ssize_t     l;
    int64_t     cpos;
    struct      timeval tv;
    struct      inotify_event *e;
    char        eb[1024], *p;
    fd_set      rset, tset;

    i = 0;
    mfd = 0;
    r = RS_ERR;
    wd1 = -1;
    wd2 = -1;
    cpos = ftell(rd->binlog_fp);

    if(cpos == -1) {
        rs_log_err(rs_errno, "ftell() failed, binlog file"); 
        goto free;
    }

    for( ;; ) {

        n = fread(buf, size, 1, rd->binlog_fp);

        /* read finish */
        if(n == 1) {
            break;
        }

        /* file eof */
        if(feof(rd->binlog_fp) != 0) {

            if(fseek(rd->binlog_fp, cpos, SEEK_SET) == -1) {
                rs_log_err(rs_errno, "fseek() failed, binlog file"); 
                goto free;
            }

            /* set inotify */
            if(rd->notify_fd == -1) {
                rd->notify_fd = rs_init_io_watch();

                if(rd->notify_fd < 0) {
                    goto free;
                }

                wd1 = rs_add_io_watch(rd->notify_fd, rd->dump_file, 
                        RS_IN_MODIFY);

                if(wd1 < 0) {
                    goto free;
                }

                wd2 = inotify_add_watch(rd->notify_fd, 
                        rs_master_info->binlog_idx_file, RS_IN_MODIFY);

                if(wd2 < 0) {
                    goto free;
                }
            }

            /* test has binlog */
            if((err = rs_has_next_binlog(rd)) == RS_OK) {
                r = RS_HAS_BINLOG;
                goto free;
            }

            if(err == RS_ERR) {
                goto free;
            }

            /* use select to monitor inotify */
            FD_ZERO(&rset);
            FD_ZERO(&tset);
            FD_SET(rd->notify_fd, &rset);
            tset = rset;
            mfd = rs_max(rd->notify_fd, mfd);
            tv.tv_sec = RS_BINLOG_EOF_WAIT_SEC;
            tv.tv_usec = 0;

            ready = select(mfd + 1, &tset, NULL, NULL, &tv);

            if(ready == 0) {
                /* select timeout*/
                if(i % 60 == 0) {
                    i = 0;
                    rs_log_info("no new binlog file or cur binlog file eof");
                }

                i += RS_BINLOG_EOF_WAIT_SEC;
                continue;
            }

            if(ready == -1) {
                if(rs_errno == EINTR) {
                    continue;
                }

                rs_log_err(rs_errno, "select failed(), inotify");
                goto free;
            }

            if(!FD_ISSET(rd->notify_fd, &tset)) {
                goto free;
            }

            l = rs_read(rd->notify_fd, eb, 1024);

            if(l <= 0) {
                rs_log_debug(0, "rs_read() failed, notify fd");
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
            } // end while

            FD_CLR(rd->notify_fd, &rset);
        } else if((err = ferror(rd->binlog_fp)) != 0) {
            /* file error */
            rs_log_err(err, "ferror(\"%s\") failed", rd->dump_file);
            goto free;
        }

    } // end for( ;; ) 

    r = RS_OK; 
    /* clearerr(fp); */

free:

    if(rd->notify_fd != -1) {
        if(rs_close(rd->notify_fd) != RS_OK) {
            r = RS_ERR;
        }

        rd->notify_fd = -1;
    }

    return r;
}

int rs_has_next_binlog(rs_request_dump_t *rd) 
{
    uint32_t    n;
    char        path[PATH_MAX + 1], *s;
    FILE        *fp;

    s = path;
    fp = rd->binlog_idx_fp;

    do {
        if((s = fgets(s, PATH_MAX, fp)) == NULL) {
            if(feof(fp) != 0) {
                return RS_NO_BINLOG;
            }

            if(ferror(fp) != 0) {
                rs_log_err(rs_errno, "fgets() failed, binlog_idx_file");
                return RS_ERR;
            }
        }

        n = rs_estr_to_uint32(s + rs_strlen(s) - 2);  /* skip /n */
    } while(n <= rd->dump_num);

    rs_memcpy(rd->dump_file, path, rs_strlen(path) + 1);

    rd->dump_file[rs_strlen(rd->dump_file) - 1] = '\0'; /* skip \n */
    rd->dump_num = n;
    rd->dump_pos = 0;

    clearerr(fp);

    rs_log_debug(0, "binlog = %s, binlog_num : %u, binlog_pos = %u",
            rd->dump_file, rd->dump_num, rd->dump_pos);

    return RS_OK;
}
