#include <stdio.h>
#include <string.h>

#include "mx6u_protocol.h"
#include "mx6u_state.h"

static int tests_failed = 0;

static void expect_true(int condition, const char *message)
{
    if (!condition)
    {
        printf("FAIL: %s\n", message);
        tests_failed++;
    }
}

static void test_state_initializes_all_nodes_as_unknown(void)
{
    mx6u_greenhouse_state_t state;
    const mx6u_node_state_t *node = NULL;

    mx6u_state_init(&state);

    node = mx6u_state_get_node(&state, 1);
    expect_true(node != NULL, "sensor node 1 should exist");
    expect_true(node->kind == MX6U_NODE_SENSOR, "sensor node kind should match");
    expect_true(node->presence == MX6U_NODE_UNKNOWN, "sensor node should start unknown");

    node = mx6u_state_get_node(&state, 4);
    expect_true(node != NULL, "executor node 4 should exist");
    expect_true(node->kind == MX6U_NODE_EXECUTOR, "executor node kind should match");
    expect_true(node->presence == MX6U_NODE_UNKNOWN, "executor node should start unknown");
}

static void test_env_update_marks_sensor_online(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_report_t report;
    const mx6u_node_state_t *node = NULL;

    mx6u_state_init(&state);
    expect_true(mx6u_protocol_parse_line(
                    "ENV,N=1,T=25.3,H=60.1,SOIL=41.2,L=1234,CO2=567",
                    &report) == 1,
                "env frame should parse for state update");

    expect_true(mx6u_state_update(&state, &report, 10) == 1, "env update should be accepted");
    node = mx6u_state_get_node(&state, 1);
    expect_true(node->presence == MX6U_NODE_ONLINE, "sensor should be online after env update");
    expect_true(node->has_report == 1, "sensor should remember first report");
    expect_true(node->last_report.data.env.co2 == 567, "sensor cache should keep env payload");
}

static void test_stat_update_marks_executor_online(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_report_t report;
    const mx6u_node_state_t *node = NULL;

    mx6u_state_init(&state);
    expect_true(mx6u_protocol_parse_line(
                    "STAT,N=4,F=ON,P=OFF,LED=ON",
                    &report) == 1,
                "stat frame should parse for state update");

    expect_true(mx6u_state_update(&state, &report, 20) == 1, "stat update should be accepted");
    node = mx6u_state_get_node(&state, 4);
    expect_true(node->presence == MX6U_NODE_ONLINE, "executor should be online after stat update");
    expect_true(node->last_report.data.status.fan_on == 1, "executor cache should keep fan state");
    expect_true(node->last_report.data.status.pump_on == 0, "executor cache should keep pump state");
}

static void test_rejects_reports_for_unknown_node_ids(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_report_t report;

    mx6u_state_init(&state);
    expect_true(mx6u_protocol_parse_line(
                    "ENV,N=9,T=25.3,H=60.1,SOIL=41.2,L=1234,CO2=567",
                    &report) == 1,
                "env frame should parse before cache validation");
    expect_true(mx6u_state_update(&state, &report, 30) == 0, "unknown sensor id should be rejected");
}

static void test_refresh_marks_stale_nodes_offline(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_report_t report;
    const mx6u_node_state_t *node = NULL;

    mx6u_state_init(&state);
    expect_true(mx6u_protocol_parse_line(
                    "ENV,N=2,T=25.3,H=60.1,SOIL=41.2,L=1234,CO2=567",
                    &report) == 1,
                "env frame should parse before offline test");
    expect_true(mx6u_state_update(&state, &report, 100) == 1, "env update should be accepted");

    mx6u_state_refresh_presence(&state, 116);
    node = mx6u_state_get_node(&state, 2);
    expect_true(node->presence == MX6U_NODE_OFFLINE, "node should become offline after timeout");
}

static void test_new_update_restores_offline_node_to_online(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_report_t report;
    const mx6u_node_state_t *node = NULL;

    mx6u_state_init(&state);
    expect_true(mx6u_protocol_parse_line(
                    "STAT,N=5,F=OFF,P=ON,LED=OFF",
                    &report) == 1,
                "stat frame should parse before restore test");
    expect_true(mx6u_state_update(&state, &report, 200) == 1, "initial stat update should be accepted");
    mx6u_state_refresh_presence(&state, 216);
    node = mx6u_state_get_node(&state, 5);
    expect_true(node->presence == MX6U_NODE_OFFLINE, "node should become offline after timeout");

    expect_true(mx6u_protocol_parse_line(
                    "STAT,N=5,F=ON,P=ON,LED=ON",
                    &report) == 1,
                "second stat frame should parse");
    expect_true(mx6u_state_update(&state, &report, 217) == 1, "second stat update should be accepted");
    node = mx6u_state_get_node(&state, 5);
    expect_true(node->presence == MX6U_NODE_ONLINE, "node should return online after a fresh report");
    expect_true(node->last_report.data.status.fill_light_on == 1, "latest executor state should overwrite cache");
}

int main(void)
{
    test_state_initializes_all_nodes_as_unknown();
    test_env_update_marks_sensor_online();
    test_stat_update_marks_executor_online();
    test_rejects_reports_for_unknown_node_ids();
    test_refresh_marks_stale_nodes_offline();
    test_new_update_restores_offline_node_to_online();

    if (tests_failed != 0)
    {
        return 1;
    }

    printf("PASS\n");
    return 0;
}
