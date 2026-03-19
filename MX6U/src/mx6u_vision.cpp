#include "mx6u_vision.h"

#include <stdio.h>
#include <string.h>

#if MX6U_ENABLE_OPENCV
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <new>
#include <string>

struct mx6u_vision_impl_t
{
    cv::VideoCapture capture;
    cv::Mat previous_gray;
    std::string device;
    int width;
    int height;
};

static int mx6u_vision_open(mx6u_vision_context_t *context)
{
    mx6u_vision_impl_t *impl = static_cast<mx6u_vision_impl_t *>(context->impl);
    int device_index = -1;

    if (impl == NULL)
    {
        return 0;
    }

    if (impl->capture.isOpened())
    {
        return 1;
    }

    if (sscanf(impl->device.c_str(), "/dev/video%d", &device_index) != 1)
    {
        device_index = -1;
    }

    try
    {
    if ((device_index >= 0) && !impl->capture.open(device_index))
    {
        return 0;
    }

    if ((device_index < 0) && !impl->capture.open(impl->device))
    {
        return 0;
    }
    }
    catch (const cv::Exception &)
    {
        return 0;
    }

    return 1;
}

static float mx6u_vision_green_ratio(const cv::Mat &bgr)
{
    cv::Mat hsv;
    cv::Mat mask;

    cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv,
                cv::Scalar(35, 40, 40),
                cv::Scalar(85, 255, 255),
                mask);
    return static_cast<float>(cv::countNonZero(mask)) /
           static_cast<float>(mask.rows * mask.cols);
}

static int mx6u_vision_motion_flag(mx6u_vision_impl_t *impl, const cv::Mat &gray)
{
    cv::Mat diff;
    cv::Mat binary;
    int motion = 0;

    if (impl->previous_gray.empty())
    {
        gray.copyTo(impl->previous_gray);
        return 0;
    }

    cv::absdiff(gray, impl->previous_gray, diff);
    cv::threshold(diff, binary, 25, 255, cv::THRESH_BINARY);
    motion = cv::countNonZero(binary) > ((binary.rows * binary.cols) / 33);
    gray.copyTo(impl->previous_gray);
    return motion;
}

static int mx6u_vision_ok(const cv::Mat &gray, float brightness, float green_ratio)
{
    cv::Mat laplacian;
    cv::Scalar mean;
    cv::Scalar stddev;
    double variance = 0.0;

    cv::Laplacian(gray, laplacian, CV_64F);
    cv::meanStdDev(laplacian, mean, stddev);
    variance = stddev[0] * stddev[0];

    if (brightness < 25.0f)
    {
        return 0;
    }

    if (variance < 30.0)
    {
        return 0;
    }

    if ((brightness < 35.0f) && (green_ratio < 0.01f))
    {
        return 0;
    }

    return 1;
}
#endif

extern "C" {

void mx6u_vision_init(mx6u_vision_context_t *context,
                      const char *device,
                      int interval_seconds,
                      int width,
                      int height)
{
    if (context == NULL)
    {
        return;
    }

    memset(context, 0, sizeof(*context));
    context->interval_seconds = interval_seconds;
    context->width = width;
    context->height = height;

#if MX6U_ENABLE_OPENCV
    mx6u_vision_impl_t *impl = new (std::nothrow) mx6u_vision_impl_t();
    if (impl == NULL)
    {
        return;
    }

    impl->device = (device != NULL) ? device : MX6U_VISION_DEVICE;
    impl->width = width;
    impl->height = height;
    context->impl = impl;
    context->initialized = 1;
#else
    (void)device;
#endif
}

void mx6u_vision_shutdown(mx6u_vision_context_t *context)
{
    if (context == NULL)
    {
        return;
    }

#if MX6U_ENABLE_OPENCV
    mx6u_vision_impl_t *impl = static_cast<mx6u_vision_impl_t *>(context->impl);
    if ((impl != NULL) && impl->capture.isOpened())
    {
        impl->capture.release();
    }
    delete impl;
#endif

    context->impl = NULL;
    context->initialized = 0;
}

void mx6u_vision_sample_if_due(mx6u_vision_context_t *context, time_t now)
{
    if (context == NULL)
    {
        return;
    }

    if ((context->last_capture_at != 0) &&
        ((now - context->last_capture_at) < context->interval_seconds))
    {
        return;
    }

    context->last_capture_at = now;
    memset(&context->last_result, 0, sizeof(context->last_result));

#if MX6U_ENABLE_OPENCV
    {
        mx6u_vision_impl_t *impl = static_cast<mx6u_vision_impl_t *>(context->impl);
        cv::Mat frame;
        cv::Mat gray;

        if ((impl == NULL) || !mx6u_vision_open(context))
        {
            return;
        }

        try
        {
            if (!impl->capture.read(frame) || frame.empty())
            {
                return;
            }
        }
        catch (const cv::Exception &)
        {
            return;
        }

        cv::resize(frame, frame, cv::Size(context->width, context->height));
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        context->last_result.available = 1;
        context->last_result.brightness = static_cast<float>(cv::mean(gray)[0]);
        context->last_result.green_ratio = mx6u_vision_green_ratio(frame);
        context->last_result.motion_flag = mx6u_vision_motion_flag(impl, gray);
        context->last_result.vision_ok = mx6u_vision_ok(gray,
                                                        context->last_result.brightness,
                                                        context->last_result.green_ratio);
    }
#endif
}

const mx6u_vision_result_t *mx6u_vision_result(const mx6u_vision_context_t *context)
{
    if (context == NULL)
    {
        return NULL;
    }

    return &context->last_result;
}

void mx6u_vision_print_snapshot(const mx6u_vision_context_t *context, FILE *stream)
{
    if ((context == NULL) || (stream == NULL))
    {
        return;
    }

    fprintf(stream,
            "VISION SNAPSHOT\navailable=%s brightness=%.1f green_ratio=%.3f motion_flag=%s vision_ok=%s\n",
            context->last_result.available ? "YES" : "NO",
            context->last_result.brightness,
            context->last_result.green_ratio,
            context->last_result.motion_flag ? "YES" : "NO",
            context->last_result.vision_ok ? "YES" : "NO");
}

} /* extern "C" */
