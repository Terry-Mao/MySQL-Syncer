
#include <rs_config.h>
#include <rs_core.h>
#include <rs_slave.h>


int rs_dml_binlog_row(rs_slave_info_t *si, void *data, 
        uint32_t len, char type, rs_dml_binlog_row_pt write_handle, 
        rs_dml_binlog_row_pt before_update_handle,
        rs_dml_binlog_row_pt update_handle,
        rs_dml_binlog_row_pt  delete_handle,
        rs_parse_binlog_row_pt parse_handle, void *obj)
{
    char                        *p;
    uint32_t                    i, j, un, nn, before, ml, cn, cl, cmdn;
    int                         cmd;
    rs_dml_binlog_row_pt        handle;

    p = data;
    before = 0;
    cmdn = 0;
    handle = NULL;

    if(si == NULL || data == NULL || obj == NULL || parse_handle == NULL)
    {
        rs_log_err(0, "rs_dml_binlog_row() failed, args can't be null");
        return RS_ERR;
    }

    /* get column number */
    rs_memcpy(&cn, p, 4);
    p += 4;

    u_char ct[cn];
    /* get column type */
    rs_memcpy(ct, p, cn);
    p += cn;

    /* get column meta length */
    rs_memcpy(&ml, p, 4);
    p += 4;

    u_char cm[ml];
    /* get column meta */
    rs_memcpy(cm, p, ml);
    p += ml;

    /* skip table id and reserved id */
    p += 8;

    /* get column number */
    cn = (uint32_t) rs_parse_packed_integer(p, &cl);
    p += cl;

    /* get used bits */
    un = (cn + 7) / 8;

    char use_bits[un], use_bits_after[un];

    rs_memcpy(use_bits, p, un);
    p += un;

    /* only for update event */
    if(type == RS_UPDATE_ROWS_EVENT) {
        rs_memcpy(use_bits_after, p, un);
        p += un;    
    }

    /* get column value */
    while(p < (char *) data + len) {

        j = 0;    
        cmd = 0;
        nn = (un * 8 + 7) / 8;
        char null_bits[nn];

        rs_memcpy(null_bits, p, nn);
        p += nn;

        for(i = 0; i < cn; i++) {

            /* used */
            if((type == RS_UPDATE_ROWS_EVENT && before == 0) ||
                    type != RS_UPDATE_ROWS_EVENT) 
            {
                if(!(use_bits[i / 8] >> (i % 8))  & 0x01) {
                    continue;
                }
            } else if(type == RS_UPDATE_ROWS_EVENT && before == 1) {
                if(!(use_bits_after[i / 8] >> (i % 8))  & 0x01) {
                    continue;
                }
            }

            /* null */
            if(!((null_bits[j / 8] >> (j % 8)) & 0x01)) {
                /* parse */
                p = parse_handle(p, ct, cm, i, obj);
            }

            j++;
        }

        /* append redis cmd */
        if(type == RS_WRITE_ROWS_EVENT) {
            handle = write_handle;
        } else if(type == RS_UPDATE_ROWS_EVENT) {
            if(before++ == 1) {
                handle = update_handle;
                before = 0;
            } else {
                handle = before_update_handle;
            }
        } else if(type == RS_UPDATE_ROWS_EVENT) {
            handle = delete_handle;
        } else {
            return RS_ERR;
        }

        if(handle == NULL) {
            continue;
        }

        if((cmd = handle(si, obj)) == RS_ERR) {
            rs_log_err(rs_errno, "handle() failed");
            return RS_ERR;
        }

        cmdn += (uint32_t) cmd;

    }

    si->cmdn += cmdn;

    return RS_OK;
}
