
#include <rs_config.h>
#include <rs_core.h>


static char rs_ansi_escape_char(char src);

uint32_t rs_str_to_uint32(char *p)
{
    uint32_t i;

    i = 0;

    while(*p >= '0' && *p <= '9') {
        i = i * 10 + *p++ - '0';
    }

    return i;
}

int32_t rs_str_to_int32(char *p)
{
    int32_t i;

    i = 0;

    while(*p >= '0' && *p <= '9') {
        i = i * 10 + *p++ - '0';
    }

    return i;
}

uint16_t rs_str_to_uint16(char *p)
{
    uint16_t i;
    i = 0;

    while(*p >= '0' && *p <= '9') {
        i = i * 10 + *p++ - '0';
    }

    return i;
}

uint64_t rs_str_to_uint64(char *p)
{
    uint64_t i;
    i = 0;

    while(*p >= '0' && *p <= '9') {
        i = i * 10 + *p++ - '0';
    }

    return i;
}

double rs_str_to_double(char *p)
{
    double i, j;
    i = 0;
    j = 0.1;

    while(*p >= '0' && *p <= '9') {
        i = i * 10 + *p++ - '0';
    }

    if(*p++ == '.') {
        while(*p >= '0' && *p <= '9') {
            i += (*p++ - '0') * j;
            j *= 0.1;
        }
    }

    return i;
}

uint32_t rs_estr_to_uint32(char *s) 
{
    uint32_t i, n;

    i = 0;
    n = 0;

    while(*s >= '0' && *s <='9') {
        i += (uint32_t) pow(10, n++) * (*s-- - '0');
    }

    return i;
}

char *rs_cp_utf8_str(char *dst, char *src) 
{
    int cl;

    for( ;; ) {

        if((cl = rs_utf8_char_len(src)) != 1) {
            dst = rs_cpymem(dst, src, cl);
            src += cl;
        } else {
            if(*src == '\\') {
                src++;

                /* dst = rs_cpymem(dst, &t, 1); */
                *dst++ = rs_ansi_escape_char(*src);
            } else if(*src == '\'' || *src == '\0') {
                break;
            } else { 
                /* dst = rs_cpymem(dst, src, 1); */
                *dst++ = *src;
            }

            src++;
        }
    }

    return src;
}

char *rs_strstr_end(char *src, char *sea, uint32_t len) 
{
    if((src = rs_strstr(src, sea)) == NULL) {
        return NULL;     
    }

    return src + len;
}


void rs_convert_to_hex(char *dst, char *src, uint32_t len) 
{
    static      u_char hex[] = "0123456789abcdef";

    while (len--) {
        *dst++ = hex[(u_char) *src >> 4];
        *dst++ = hex[(u_char) *src++ & 0xf];
    }

}

char *rs_cp_binary_str(char *dst, uint32_t *len, char *src) 
{
    *len = 0;

    for( ;; ) {
        if(*src == '\\') {
            src++;

            *dst++ = rs_ansi_escape_char(*src);
            /* dst = rs_cpymem(dst, &t, 1); */

            *len += 1;
        } else if(*src == '\'' || *src == '\0') {
            break;
        } else { 
            /* dst = rs_cpymem(dst, src, 1); */
            *dst++ = *src;
            *len += 1;
        }

        src++;
    }

    return src;
}


char *rs_ncp_str_till(char *dst, char *src, char es, size_t len) 
{
    size_t n;

    n = len;

    while(*src != es && n) {
        *dst++ = *src++; 
        n--;
    }

    *dst = '\0';
    src++;

    return src;
}

void rs_uint32_to_str(uint32_t n, char *buf) 
{
    uint32_t    p;
    char        b[UINT32_LEN + 1], *t, *s;

    s = buf;
    t = b + UINT32_LEN;
    p = 0;

    do {
        *--t = (n % 10 + '0');
        p += 1;
    } while (n /= 10);

    s = rs_cpymem(s, t, p);
    *s = '\0';
}

int64_t rs_timestr_to_msec(char *time) 
{
    struct tm tm;
    int64_t tsec;

    rs_memzero(&tm, sizeof(struct tm));
    tsec = 0;

    if(strptime(time, RS_TIME_CONVERT_FORMAT, &tm) == 0) {
        rs_log_err(rs_errno, "strptime(\"%s\",&tm) failed", time);
        return RS_ERR;
    }

    tsec = (int64_t) mktime(&tm);

    if(tsec == -1) {
        rs_log_err(rs_errno, "mktime() failed");
        return RS_ERR;
    }

    return tsec * 1000;
}

static char rs_ansi_escape_char(char src) 
{
    char t;

    switch(src) {
        case '0':
            t = 0;
            break;

        case 'a':
            t = 7;
            break;

        case 'b':
            t = 8;
            break;

        case 't':
            t = 9;
            break;

        case 'n':
            t = 10;
            break;

        case 'v':
            t = 11;
            break;

        case 'f':
            t = 12;
            break;

        case 'r':
            t = 13;
            break;

        case 'Z':
            t = 26;
            break;

        default:
            t = src;
    }

    return t;
}

