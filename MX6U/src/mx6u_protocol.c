#include "mx6u_protocol.h"

#include <stdio.h>
#include <string.h>

static int mx6u_protocol_parse_switch(const char *value, int *state)
{
    if ((value == NULL) || (state == NULL))
    {
        return 0;
    }

    if (strcmp(value, "ON") == 0)
    {
        *state = 1;
        return 1;
    }

    if (strcmp(value, "OFF") == 0)
    {
        *state = 0;
        return 1;
    }

    return 0;
}

static int mx6u_protocol_parse_env(const char *line, mx6u_report_t *report)
{
    env_report_t parsed;
    int matched = 0;

    matched = sscanf(line,
                     "ENV,N=%d,T=%f,H=%f,SOIL=%f,L=%d,CO2=%d",
                     &parsed.node_id,
                     &parsed.temp,
                     &parsed.hum,
                     &parsed.soil,
                     &parsed.light,
                     &parsed.co2);

    if ((matched != 6) || (parsed.node_id <= 0))
    {
        return 0;
    }

    report->type = MX6U_REPORT_ENV;
    report->data.env = parsed;
    return 1;
}

static int mx6u_protocol_parse_status(const char *line, mx6u_report_t *report)
{
    status_report_t parsed;
    char fan[4];
    char pump[4];
    char led[4];
    int matched = 0;

    matched = sscanf(line,
                     "STAT,N=%d,F=%3[^,],P=%3[^,],LED=%3s",
                     &parsed.node_id,
                     fan,
                     pump,
                     led);

    if ((matched != 4) || (parsed.node_id <= 0))
    {
        return 0;
    }

    if (!mx6u_protocol_parse_switch(fan, &parsed.fan_on) ||
        !mx6u_protocol_parse_switch(pump, &parsed.pump_on) ||
        !mx6u_protocol_parse_switch(led, &parsed.fill_light_on))
    {
        return 0;
    }

    report->type = MX6U_REPORT_STATUS;
    report->data.status = parsed;
    return 1;
}

int mx6u_protocol_parse_line(const char *line, mx6u_report_t *report)
{
    if ((line == NULL) || (report == NULL))
    {
        return 0;
    }

    report->type = MX6U_REPORT_NONE;

    if (strncmp(line, "ENV,", 4) == 0)
    {
        return mx6u_protocol_parse_env(line, report);
    }

    if (strncmp(line, "STAT,", 5) == 0)
    {
        return mx6u_protocol_parse_status(line, report);
    }

    return 0;
}
