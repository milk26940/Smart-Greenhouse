#ifndef MX6U_STATE_H
#define MX6U_STATE_H

#include <stdio.h>
#include <time.h>

#include "mx6u_protocol.h"

#define MX6U_SENSOR_NODE_MIN 1
#define MX6U_SENSOR_NODE_MAX 3
#define MX6U_EXECUTOR_NODE_MIN 4
#define MX6U_EXECUTOR_NODE_MAX 5
#define MX6U_NODE_COUNT 5
#define MX6U_OFFLINE_TIMEOUT_SECONDS 15

typedef enum
{
    MX6U_NODE_SENSOR = 0,
    MX6U_NODE_EXECUTOR
} mx6u_node_kind_t;

typedef enum
{
    MX6U_NODE_UNKNOWN = 0,
    MX6U_NODE_ONLINE,
    MX6U_NODE_OFFLINE
} mx6u_node_presence_t;

typedef struct
{
    int node_id;
    mx6u_node_kind_t kind;
    int has_report;
    mx6u_node_presence_t presence;
    time_t last_seen_at;
    mx6u_report_t last_report;
} mx6u_node_state_t;

typedef struct
{
    mx6u_node_state_t nodes[MX6U_NODE_COUNT];
} mx6u_greenhouse_state_t;

void mx6u_state_init(mx6u_greenhouse_state_t *state);
int mx6u_state_update(mx6u_greenhouse_state_t *state, const mx6u_report_t *report, time_t now);
void mx6u_state_refresh_presence(mx6u_greenhouse_state_t *state, time_t now);
const mx6u_node_state_t *mx6u_state_get_node(const mx6u_greenhouse_state_t *state, int node_id);
void mx6u_state_print_snapshot(const mx6u_greenhouse_state_t *state, FILE *stream);

#endif
