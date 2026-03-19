#define _DEFAULT_SOURCE

#include "mx6u_serial.h"

#ifdef _WIN32

int mx6u_serial_open(const char *device_path)
{
    (void)device_path;
    return -1;
}

int mx6u_serial_read_frame(int fd,
                           mx6u_line_framer_t *framer,
                           char *line_out,
                           size_t line_out_size)
{
    (void)fd;
    (void)framer;
    (void)line_out;
    (void)line_out_size;
    return -1;
}

void mx6u_serial_close(int fd)
{
    (void)fd;
}

#else

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

static int mx6u_serial_configure(int fd)
{
    struct termios options;

    if (tcgetattr(fd, &options) != 0)
    {
        return -1;
    }

    cfmakeraw(&options);
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag |= CREAD | CLOCAL;
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &options) != 0)
    {
        return -1;
    }

    return 0;
}

int mx6u_serial_open(const char *device_path)
{
    int fd = -1;
    int flags = 0;

    if (device_path == NULL)
    {
        return -1;
    }

    fd = open(device_path, O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
    {
        return -1;
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0)
    {
        (void)fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
    }

    if (mx6u_serial_configure(fd) != 0)
    {
        close(fd);
        return -1;
    }

    return fd;
}

int mx6u_serial_read_frame(int fd,
                           mx6u_line_framer_t *framer,
                           char *line_out,
                           size_t line_out_size)
{
    unsigned char byte = 0;
    int result = 0;

    if ((fd < 0) || (framer == NULL) || (line_out == NULL) || (line_out_size == 0U))
    {
        return -1;
    }

    for (;;)
    {
        ssize_t bytes_read = read(fd, &byte, 1);

        if (bytes_read == 0)
        {
            result = mx6u_line_framer_flush(framer, line_out, line_out_size);
            if (result != 0)
            {
                return result;
            }

            continue;
        }

        if (bytes_read < 0)
        {
            return -1;
        }

        result = mx6u_line_framer_push_byte(framer, byte, line_out, line_out_size);
        if (result != 0)
        {
            return result;
        }
    }
}

void mx6u_serial_close(int fd)
{
    if (fd >= 0)
    {
        close(fd);
    }
}

#endif
