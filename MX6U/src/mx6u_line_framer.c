#include "mx6u_line_framer.h"

#include <string.h>

void mx6u_line_framer_init(mx6u_line_framer_t *framer)
{
    if (framer == NULL)
    {
        return;
    }

    framer->length = 0;
    framer->overflowed = 0;
    framer->buffer[0] = '\0';
}

int mx6u_line_framer_push_byte(mx6u_line_framer_t *framer,
                               unsigned char byte,
                               char *line_out,
                               size_t line_out_size)
{
    if ((framer == NULL) || (line_out == NULL) || (line_out_size == 0U))
    {
        return -1;
    }

    if ((byte == '\r') || (byte == '\n'))
    {
        if (framer->overflowed)
        {
            mx6u_line_framer_init(framer);
            return 0;
        }

        if (framer->length == 0U)
        {
            return 0;
        }

        if (framer->length >= line_out_size)
        {
            mx6u_line_framer_init(framer);
            return -1;
        }

        memcpy(line_out, framer->buffer, framer->length);
        line_out[framer->length] = '\0';
        mx6u_line_framer_init(framer);
        return 1;
    }

    if (framer->overflowed)
    {
        return -1;
    }

    if (framer->length >= (sizeof(framer->buffer) - 1U))
    {
        framer->overflowed = 1;
        framer->length = 0;
        framer->buffer[0] = '\0';
        return -1;
    }

    framer->buffer[framer->length++] = (char)byte;
    framer->buffer[framer->length] = '\0';
    return 0;
}

int mx6u_line_framer_flush(mx6u_line_framer_t *framer,
                           char *line_out,
                           size_t line_out_size)
{
    if ((framer == NULL) || (line_out == NULL) || (line_out_size == 0U))
    {
        return -1;
    }

    if (framer->overflowed)
    {
        mx6u_line_framer_init(framer);
        return 0;
    }

    if (framer->length == 0U)
    {
        return 0;
    }

    if (framer->length >= line_out_size)
    {
        mx6u_line_framer_init(framer);
        return -1;
    }

    memcpy(line_out, framer->buffer, framer->length);
    line_out[framer->length] = '\0';
    mx6u_line_framer_init(framer);
    return 1;
}
