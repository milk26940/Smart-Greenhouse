#include "mx6u_state.h"

#include <stddef.h>

static mx6u_node_kind_t mx6u_state_kind_for_node_id(int node_id)
{
    if ((node_id >= MX6U_SENSOR_NODE_MIN) && (node_id <= MX6U_SENSOR_NODE_MAX))
    {
        return MX6U_NODE_SENSOR;
    }

    return MX6U_NODE_EXECUTOR;
}

static int mx6u_state_validate_report_node(const mx6u_report_t *report)
{
    if (report->type == MX6U_REPORT_ENV)
    {
        return (report->data.env.node_id >= MX6U_SENSOR_NODE_MIN) &&
               (report->data.env.node_id <= MX6U_SENSOR_NODE_MAX);
    }

    if (report->type == MX6U_REPORT_STATUS)
    {
        return (report->data.status.node_id >= MX6U_EXECUTOR_NODE_MIN) &&
               (report->data.status.node_id <= MX6U_EXECUTOR_NODE_MAX);
    }

    return 0;
}

static int mx6u_state_index_for_node_id(int node_id)
{
    if ((node_id >= MX6U_SENSOR_NODE_MIN) && (node_id <= MX6U_SENSOR_NODE_MAX))
    {
        return node_id - MX6U_SENSOR_NODE_MIN;
    }

    if ((node_id >= MX6U_EXECUTOR_NODE_MIN) && (node_id <= MX6U_EXECUTOR_NODE_MAX))
    {
        return (node_id - MX6U_EXECUTOR_NODE_MIN) + (MX6U_SENSOR_NODE_MAX - MX6U_SENSOR_NODE_MIN + 1);
    }

    return -1;
}

static const char *mx6u_state_kind_name(mx6u_node_kind_t kind)
{
    return (kind == MX6U_NODE_SENSOR) ? "SENSOR" : "EXECUTOR";
}

static const char *mx6u_state_presence_name(mx6u_node_presence_t presence)
{
    switch (presence)
    {
    case MX6U_NODE_ONLINE:
        return "ONLINE";
    case MX6U_NODE_OFFLINE:
        return "OFFLINE";
    default:
        return "UNKNOWN";
    }
}

void mx6u_state_init(mx6u_greenhouse_state_t *state)
{
    int i;

    if (state == NULL)
    {
        return;
    }

    for (i = 0; i < MX6U_NODE_COUNT; ++i)
    {
        mx6u_node_state_t *node = &state->nodes[i];

        node->node_id = (i < 3) ? (MX6U_SENSOR_NODE_MIN + i) : (MX6U_EXECUTOR_NODE_MIN + i - 3);
        node->kind = mx6u_state_kind_for_node_id(node->node_id);
        node->has_report = 0;
        node->presence = MX6U_NODE_UNKNOWN;
        node->last_seen_at = 0;
        node->last_report.type = MX6U_REPORT_NONE;
    }
}

int mx6u_state_update(mx6u_greenhouse_state_t *state, const mx6u_report_t *report, time_t now)
{
    int node_id = 0;
    int index = -1;
    mx6u_node_state_t *node = NULL;

    if ((state == NULL) || (report == NULL))
    {
        return 0;
    }

    if (!mx6u_state_validate_report_node(report))
    {
        return 0;
    }

    node_id = (report->type == MX6U_REPORT_ENV) ? report->data.env.node_id : report->data.status.node_id;
    index = mx6u_state_index_for_node_id(node_id);
    if (index < 0)
    {
        return 0;
    }

    node = &state->nodes[index];
    node->has_report = 1;
    node->presence = MX6U_NODE_ONLINE;
    node->last_seen_at = now;
    node->last_report = *report;
    return 1;
}

void mx6u_state_refresh_presence(mx6u_greenhouse_state_t *state, time_t now)
{
    int i;

    if (state == NULL)
    {
        return;
    }

    for (i = 0; i < MX6U_NODE_COUNT; ++i)
    {
        mx6u_node_state_t *node = &state->nodes[i];

        if (!node->has_report)
        {
            node->presence = MX6U_NODE_UNKNOWN;
            continue;
        }

        if ((now - node->last_seen_at) > MX6U_OFFLINE_TIMEOUT_SECONDS)
        {
            node->presence = MX6U_NODE_OFFLINE;
            continue;
        }

        node->presence = MX6U_NODE_ONLINE;
    }
}

const mx6u_node_state_t *mx6u_state_get_node(const mx6u_greenhouse_state_t *state, int node_id)
{
    int index = mx6u_state_index_for_node_id(node_id);

    if ((state == NULL) || (index < 0))
    {
        return NULL;
    }

    return &state->nodes[index];
}

void mx6u_state_print_snapshot(const mx6u_greenhouse_state_t *state, FILE *stream)
{
    int i;

    if ((state == NULL) || (stream == NULL))
    {
        return;
    }

    fputs("STATE SNAPSHOT\n", stream);
    for (i = 0; i < MX6U_NODE_COUNT; ++i)
    {
        const mx6u_node_state_t *node = &state->nodes[i];

        if ((!node->has_report) || (node->last_report.type == MX6U_REPORT_NONE))
        {
            fprintf(stream,
                    "%s %d %s\n",
                    mx6u_state_kind_name(node->kind),
                    node->node_id,
                    mx6u_state_presence_name(node->presence));
            continue;
        }

        if (node->last_report.type == MX6U_REPORT_ENV)
        {
            fprintf(stream,
                    "%s %d %s T=%.1f H=%.1f SOIL=%.1f L=%d CO2=%d\n",
                    mx6u_state_kind_name(node->kind),
                    node->node_id,
                    mx6u_state_presence_name(node->presence),
                    node->last_report.data.env.temp,
                    node->last_report.data.env.hum,
                    node->last_report.data.env.soil,
                    node->last_report.data.env.light,
                    node->last_report.data.env.co2);
            continue;
        }

        fprintf(stream,
                "%s %d %s F=%s P=%s LED=%s\n",
                mx6u_state_kind_name(node->kind),
                node->node_id,
                mx6u_state_presence_name(node->presence),
                node->last_report.data.status.fan_on ? "ON" : "OFF",
                node->last_report.data.status.pump_on ? "ON" : "OFF",
                node->last_report.data.status.fill_light_on ? "ON" : "OFF");
    }
}
