#ifndef MX6U_VISION_H
#define MX6U_VISION_H

#include <stdio.h>
#include <time.h>

#define MX6U_VISION_DEVICE "/dev/video2"
#define MX6U_VISION_INTERVAL_SECONDS 10
#define MX6U_VISION_WIDTH 320
#define MX6U_VISION_HEIGHT 240

typedef struct
{
    int available;
    float brightness;
    float green_ratio;
    int motion_flag;
    int vision_ok;
} mx6u_vision_result_t;

typedef struct
{
    void *impl;
    mx6u_vision_result_t last_result;
    time_t last_capture_at;
    int initialized;
    int interval_seconds;
    int width;
    int height;
} mx6u_vision_context_t;

#ifdef __cplusplus
extern "C" {
#endif

void mx6u_vision_init(mx6u_vision_context_t *context,
                      const char *device,
                      int interval_seconds,
                      int width,
                      int height);
void mx6u_vision_shutdown(mx6u_vision_context_t *context);
void mx6u_vision_sample_if_due(mx6u_vision_context_t *context, time_t now);
const mx6u_vision_result_t *mx6u_vision_result(const mx6u_vision_context_t *context);
void mx6u_vision_print_snapshot(const mx6u_vision_context_t *context, FILE *stream);

#ifdef __cplusplus
}
#endif

#endif
