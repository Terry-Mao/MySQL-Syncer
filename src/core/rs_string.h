
#ifndef _RS_STRING_H_INCLUDED_
#define _RS_STRING_H_INCLUDED_


#include <rs_config.h>
#include <rs_core.h>

typedef struct {
    size_t len;
    char *data;
} rs_str_t;


#define RS_TIME_CONVERT_FORMAT      "%Y-%m-%d %H:%M:%S"


#define rs_string(str)     { sizeof(str) - 1, (char *) str }
#define rs_null_string     { 0, NULL }
#define rs_str_set(str, text)                                               \
    (str)->len = sizeof(text) - 1; (str)->data = (char *) text

#define rs_strncmp(s1, s2, n)  strncmp((const char *) s1, (const char *) s2, n)
#define rs_strncasecmp(s1, s2, n)  strncmp((const char *) s1, (const char *) \
        s2, n)


/* msvc and icc7 compile strcmp() to inline loop */
#define rs_strcmp(s1, s2)  strcmp((const char *) s1, (const char *) s2)
#define rs_memcmp(s1, s2, n)  memcmp((const char *) s1, (const char *) s2, n)

#define rs_strstr(s1, s2)  strstr((const char *) s1, (const char *) s2)
#define rs_strlen(s)       strlen((const char *) s)

#define rs_strchr(s1, c)   strchr((const char *) s1, (int) c)

#define rs_memzero(buf, n)       (void) memset(buf, 0, n)
#define rs_memset(buf, c, n)     (void) memset(buf, c, n)
#define rs_memcpy(dst, src, n)   (void) memcpy(dst, src, n)
#define rs_cpymem(dst, src, n)   (((char *) memcpy(dst, src, n)) + (n))

#define rs_utf8_char_len(s)                                                 \
    ((*(u_char *) s < 0x80 ? 1 : (*(u_char *) s < 0xE0) ? 2 : \
      (*(u_char *) s < 0xF0) ? 3 : -1))

uint16_t rs_str_to_uint16(char *p);
uint32_t rs_str_to_uint32(char *p);
uint64_t rs_str_to_uint64(char *p);
int32_t rs_str_to_int32(char *p);

uint32_t rs_estr_to_uint32(char *s);
u_char rs_uint64_to_str(uint64_t n, char *buf);
int64_t rs_timestr_to_msec(char *time);

char *rs_cp_utf8_str(char *dst, char *src);

char *rs_cp_binary_str(char *dst, uint32_t *len, char *src);
char *rs_strstr_end(char *src, char *sea, uint32_t len);

void rs_convert_to_hex(char *buf, char *src, uint32_t len);
char *rs_ncp_str_till(char *dst, char *src, char es, size_t len);

#endif
