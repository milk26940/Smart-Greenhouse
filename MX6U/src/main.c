#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mx6u_app.h"
#include "mx6u_line_framer.h"
#include "mx6u_decision.h"
#include "mx6u_protocol.h"
#include "mx6u_mqtt.h"
#include "mx6u_serial.h"
#include "mx6u_state.h"
#include "mx6u_vision.h"

static void mx6u_app_print_parsed_report(const mx6u_report_t *report)
{
    if (report->type == MX6U_REPORT_ENV)
    {
        printf("ENV node=%d temp=%.1f hum=%.1f soil=%.1f light=%d co2=%d\n",
               report->data.env.node_id,
               report->data.env.temp,
               report->data.env.hum,
               report->data.env.soil,
               report->data.env.light,
               report->data.env.co2);
        return;
    }

    if (report->type == MX6U_REPORT_STATUS)
    {
        printf("STAT node=%d fan=%s pump=%s led=%s\n",
               report->data.status.node_id,
               report->data.status.fan_on ? "ON" : "OFF",
               report->data.status.pump_on ? "ON" : "OFF",
               report->data.status.fill_light_on ? "ON" : "OFF");
    }
}

static int mx6u_app_current_hour(time_t now)
{
    struct tm *local_time = localtime(&now);

    if (local_time == NULL)
    {
        return 12;
    }

    return local_time->tm_hour;
}

static void mx6u_app_handle_line(mx6u_greenhouse_state_t *state,
                                 mx6u_decision_context_t *decision,
                                 mx6u_vision_context_t *vision,
                                 mx6u_mqtt_client_t *mqtt,
                                 const char *line,
                                 time_t now)
{
    mx6u_report_t report;

    if (!mx6u_protocol_parse_line(line, &report))
    {
        puts("Parse error: unsupported or invalid frame.");
        fflush(stdout);
        return;
    }

    if (!mx6u_state_update(state, &report, now))
    {
        puts("State update error: unsupported node id for parsed frame.");
        fflush(stdout);
        return;
    }

    mx6u_state_refresh_presence(state, now);
    mx6u_decision_observe_report(decision, &report);
    mx6u_vision_sample_if_due(vision, now);
    mx6u_decision_set_vision_result(decision, mx6u_vision_result(vision));
    mx6u_decision_evaluate(decision, state, now, mx6u_app_current_hour(now));
    mx6u_mqtt_publish_state(mqtt, state, mx6u_decision_result(decision), mx6u_vision_result(vision), now);
    mx6u_app_print_parsed_report(&report);
    mx6u_state_print_snapshot(state, stdout);
    mx6u_vision_print_snapshot(vision, stdout);
    mx6u_decision_print_snapshot(decision, stdout);
    mx6u_mqtt_print_snapshot(mqtt, stdout);
    fflush(stdout);
}

static int mx6u_app_run_stdin_loop(void)
{
    mx6u_decision_context_t decision;
    mx6u_greenhouse_state_t state;
    mx6u_vision_context_t vision;
    mx6u_mqtt_client_t mqtt;
    char line[256];

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    mx6u_vision_init(&vision, MX6U_VISION_DEVICE, MX6U_VISION_INTERVAL_SECONDS, MX6U_VISION_WIDTH, MX6U_VISION_HEIGHT);
    mx6u_mqtt_init(&mqtt,
                   MX6U_MQTT_DEFAULT_BROKER,
                   MX6U_MQTT_DEFAULT_PORT,
                   MX6U_MQTT_DEFAULT_TOPIC,
                   MX6U_MQTT_DEFAULT_CLIENT_ID);
    mx6u_mqtt_connect(&mqtt);
    puts("MX6U parser ready. Reading from stdin test mode.");
    fflush(stdout);
    while (fgets(line, sizeof(line), stdin) != NULL)
    {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0')
        {
            continue;
        }

        mx6u_app_handle_line(&state, &decision, &vision, &mqtt, line, time(NULL));
    }

    mx6u_mqtt_shutdown(&mqtt);
    mx6u_vision_shutdown(&vision);
    return 0;
}

static int mx6u_app_run_serial_loop(const char *device_path)
{
    mx6u_decision_context_t decision;
    mx6u_greenhouse_state_t state;
    mx6u_vision_context_t vision;
    mx6u_mqtt_client_t mqtt;
    mx6u_line_framer_t framer;
    char line[256];
    int fd = -1;

    fd = mx6u_serial_open(device_path);
    if (fd < 0)
    {
        fprintf(stderr, "Failed to open serial device: %s\n", device_path);
        return 1;
    }

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    mx6u_vision_init(&vision, MX6U_VISION_DEVICE, MX6U_VISION_INTERVAL_SECONDS, MX6U_VISION_WIDTH, MX6U_VISION_HEIGHT);
    mx6u_mqtt_init(&mqtt,
                   MX6U_MQTT_DEFAULT_BROKER,
                   MX6U_MQTT_DEFAULT_PORT,
                   MX6U_MQTT_DEFAULT_TOPIC,
                   MX6U_MQTT_DEFAULT_CLIENT_ID);
    mx6u_mqtt_connect(&mqtt);
    mx6u_line_framer_init(&framer);
    printf("MX6U parser ready. Reading from serial device: %s\n", device_path);
    fflush(stdout);

    for (;;)
    {
        int result = mx6u_serial_read_frame(fd, &framer, line, sizeof(line));

        if (result < 0)
        {
            continue;
        }

        if (result == 0)
        {
            continue;
        }

        mx6u_app_handle_line(&state, &decision, &vision, &mqtt, line, time(NULL));
    }
}

void mx6u_app_print_report(const char *line)
{
    mx6u_report_t report;

    if (!mx6u_protocol_parse_line(line, &report))
    {
        puts("Parse error: unsupported or invalid frame.");
        fflush(stdout);
        return;
    }

    if (report.type == MX6U_REPORT_ENV)
    {
        mx6u_app_print_parsed_report(&report);
    }
    else if (report.type == MX6U_REPORT_STATUS)
    {
        mx6u_app_print_parsed_report(&report);
    }
    fflush(stdout);
}

int mx6u_app_run(void)
{
    const char *device_path = getenv("MX6U_SERIAL_DEVICE");

    if ((device_path != NULL) && (strcmp(device_path, "stdin") == 0))
    {
        return mx6u_app_run_stdin_loop();
    }

    if ((device_path == NULL) || (device_path[0] == '\0'))
    {
        device_path = "/dev/ttymxc2";
    }

    return mx6u_app_run_serial_loop(device_path);
}

int main(void)
{
    return mx6u_app_run();
}
