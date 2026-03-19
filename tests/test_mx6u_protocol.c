#include <stdio.h>
#include <string.h>

#include "mx6u_protocol.h"

static int tests_failed = 0;

static void expect_true(int condition, const char *message)
{
    if (!condition)
    {
        printf("FAIL: %s\n", message);
        tests_failed++;
    }
}

static void test_parse_env_frame_succeeds(void)
{
    mx6u_report_t report;
    int ok = mx6u_protocol_parse_line(
        "ENV,N=1,T=25.3,H=60.1,SOIL=41.2,L=1234,CO2=567",
        &report);

    expect_true(ok == 1, "env frame should parse");
    expect_true(report.type == MX6U_REPORT_ENV, "env frame type should match");
    expect_true(report.data.env.node_id == 1, "env node id should parse");
    expect_true(report.data.env.temp == 25.3f, "env temperature should parse");
    expect_true(report.data.env.hum == 60.1f, "env humidity should parse");
    expect_true(report.data.env.soil == 41.2f, "env soil should parse");
    expect_true(report.data.env.light == 1234, "env light should parse");
    expect_true(report.data.env.co2 == 567, "env co2 should parse");
}

static void test_parse_stat_frame_succeeds(void)
{
    mx6u_report_t report;
    int ok = mx6u_protocol_parse_line(
        "STAT,N=2,F=ON,P=OFF,LED=ON",
        &report);

    expect_true(ok == 1, "stat frame should parse");
    expect_true(report.type == MX6U_REPORT_STATUS, "stat frame type should match");
    expect_true(report.data.status.node_id == 2, "status node id should parse");
    expect_true(report.data.status.fan_on == 1, "fan state should parse");
    expect_true(report.data.status.pump_on == 0, "pump state should parse");
    expect_true(report.data.status.fill_light_on == 1, "fill light state should parse");
}

static void test_parse_rejects_unknown_frame_type(void)
{
    mx6u_report_t report;
    int ok = mx6u_protocol_parse_line("CTRL,M=ON,S=80", &report);

    expect_true(ok == 0, "unknown frame type should be rejected");
}

static void test_parse_rejects_missing_fields(void)
{
    mx6u_report_t report;
    int ok = mx6u_protocol_parse_line("ENV,N=1,T=25.3,H=60.1,L=1234,CO2=567", &report);

    expect_true(ok == 0, "env frame missing SOIL should be rejected");
}

int main(void)
{
    test_parse_env_frame_succeeds();
    test_parse_stat_frame_succeeds();
    test_parse_rejects_unknown_frame_type();
    test_parse_rejects_missing_fields();

    if (tests_failed != 0)
    {
        return 1;
    }

    printf("PASS\n");
    return 0;
}
