#ifndef MX6U_LINE_FRAMER_H
#define MX6U_LINE_FRAMER_H

#include <stddef.h>

typedef struct
{
    char buffer[256];
    size_t length;
    int overflowed;
} mx6u_line_framer_t;

void mx6u_line_framer_init(mx6u_line_framer_t *framer);
int mx6u_line_framer_push_byte(mx6u_line_framer_t *framer,
                               unsigned char byte,
                               char *line_out,
                               size_t line_out_size);
int mx6u_line_framer_flush(mx6u_line_framer_t *framer,
                           char *line_out,
                           size_t line_out_size);

#endif
