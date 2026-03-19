#ifndef MX6U_SERIAL_H
#define MX6U_SERIAL_H

#include <stddef.h>

#include "mx6u_line_framer.h"

int mx6u_serial_open(const char *device_path);
int mx6u_serial_read_frame(int fd,
                           mx6u_line_framer_t *framer,
                           char *line_out,
                           size_t line_out_size);
void mx6u_serial_close(int fd);

#endif
