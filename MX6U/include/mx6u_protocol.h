#ifndef MX6U_PROTOCOL_H
#define MX6U_PROTOCOL_H

typedef struct
{
    int node_id;
    float temp;
    float hum;
    float soil;
    int light;
    int co2;
} env_report_t;

typedef struct
{
    int node_id;
    int fan_on;
    int pump_on;
    int fill_light_on;
} status_report_t;

typedef enum
{
    MX6U_REPORT_NONE = 0,
    MX6U_REPORT_ENV,
    MX6U_REPORT_STATUS
} mx6u_report_type_t;

typedef struct
{
    mx6u_report_type_t type;
    union
    {
        env_report_t env;
        status_report_t status;
    } data;
} mx6u_report_t;

int mx6u_protocol_parse_line(const char *line, mx6u_report_t *report);

#endif
