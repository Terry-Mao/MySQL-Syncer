
#include <rs_config.h>
#include <rs_core.h>
#include <rs_master.h>

int rs_read_binlog(rs_request_dump_t *rd) 
{
    int                 r, err;
    char                key[3];
    rs_event_handler    *handler;

    if(rd == NULL) {
        return RS_ERR;
    }

    for( ;; ) {

        /* HEADER */
        if((r = rs_binlog_header_event(rd)) != RS_OK) {
            goto free;
        }    

        /* get event_key */
        rs_uint32_to_str(rd->binlog_info.t, key);

        err = rs_get_shash(rd->event_handler, key, (void *) &handler);

        if(err = RS_KEY_NOT_EXISTS) {
            rs_log_err(0, "unknow binlog evnet");
            return RS_ERR;
        }

        /* event handler */
        if((r = handler(rd)) != RS_OK) {
            goto free;
        }

        /* FINISH */
        if((r = rs_binlog_finish_event(rd)) != RS_OK) {
            break; 
        }
    }

free: 

    /* clean old file io buffer */
    rd->io_buf.pos = rd->io_buf.last;
    return r;
}

int rs_eof_read_binlog2(rs_request_dump_t *rd, void *buf, size_t size) 
{
    int         wd1, wd2, err, r;
    size_t      ms, n, ls, tl; 
    ssize_t     l;
    struct      inotify_event *e;
    char        eb[1024], *p, *f;

    if(rd == NULL) {
        return RS_ERR;
    }

    n = 0;
    wd1 = -1;
    wd2 = -2;
    f = buf;
    r = RS_ERR;
    ms = rs_min((uint32_t) (rd->io_buf.last - rd->io_buf.pos), size);

    /* use io_buf */
    if(ms > 0) {
        f = rs_cpymem(f, rd->io_buf.pos, ms);
        rd->io_buf.pos += ms;
    }

    while(ms < size) {

        /* feed io_buf */
        n = fread(rd->io_buf.start, 1, rd->io_buf.size, rd->binlog_fp);

        if(n > 0) {
            ls = rs_min(n, size - ms);

            f = rs_cpymem(f, rd->io_buf.start, ls);

            rd->io_buf.pos = rd->io_buf.start + ls; 
            rd->io_buf.last = rd->io_buf.start + n;

            ms += ls;
        } else {

            if(feof(rd->binlog_fp) == 0) {
                if((err = ferror(rd->binlog_fp)) != 0) {
                    /* file error */
                    rs_log_err(err, "ferror(\"%s\") failed", rd->dump_file);
                    goto free;
                }
            }

            /* set inotify */
            if(rd->notify_fd == -1) {
                rd->notify_fd = rs_init_io_watch();

                if(rd->notify_fd < 0) {
                    goto free;
                }

                wd1 = rs_add_io_watch(rd->notify_fd, rd->dump_file, 
                        RS_IN_MODIFY);

                wd2 = inotify_add_watch(rd->notify_fd, 
                        rs_master_info->binlog_idx_file, RS_IN_MODIFY);

                if(wd1 < 0 || wd2 < 0) {
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

            /* sleep wait, release cpu */
            err = rs_timed_select(rd->notify_fd, RS_BINLOG_EOF_WAIT_SEC, 0);

            if(err == RS_ERR) {
                goto free;
            } else if(err == RS_TIMEDOUT) {
                continue;
            }

            l = rs_read(rd->notify_fd, eb, 1024);

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
            } // end while
        }
    }

    r = RS_OK; 
    clearerr(rd->binlog_fp);

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
    char        *s, *p;
    FILE        *fp;

    if(rd == NULL) {
        return RS_ERR;
    }

    fp = rd->binlog_idx_fp;
    s = rd->dump_tmp_file;
    p = s;

    fseek(fp, 0, SEEK_SET);
    clearerr(fp);

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

    rs_memcpy(rd->dump_file, p, rs_strlen(p) + 1);

    rd->dump_file[rs_strlen(rd->dump_file) - 1] = '\0'; /* skip \n */
    rd->dump_num = n;
    rd->dump_pos = 0;

    rs_log_debug(0, "rs_has_next_binlog(), binlog = %s, binlog_num : %u, "
            "binlog_pos = %u",
            rd->dump_file, rd->dump_num, rd->dump_pos);

    return RS_OK;
}
