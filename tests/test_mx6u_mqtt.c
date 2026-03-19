#include <stdio.h>
#include <string.h>
#include <time.h>

#include "mx6u_decision.h"
#include "mx6u_mqtt.h"
#include "mx6u_protocol.h"
#include "mx6u_state.h"
#include "mx6u_vision.h"

static int tests_failed = 0;

static void expect_true(int condition, const char *message)
{
    if (!condition)
    {
        printf("FAIL: %s\n", message);
        tests_failed++;
    }
}

static void update_env(mx6u_greenhouse_state_t *state,
                       mx6u_decision_context_t *decision,
                       const char *line,
                       time_t now)
{
    mx6u_report_t report;

    expect_true(mx6u_protocol_parse_line(line, &report) == 1, "env frame should parse");
    expect_true(mx6u_state_update(state, &report, now) == 1, "env state update should succeed");
    mx6u_state_refresh_presence(state, now);
    mx6u_decision_observe_report(decision, &report);
}

static void update_stat(mx6u_greenhouse_state_t *state,
                        mx6u_decision_context_t *decision,
                        const char *line,
                        time_t now)
{
    mx6u_report_t report;

    expect_true(mx6u_protocol_parse_line(line, &report) == 1, "stat frame should parse");
    expect_true(mx6u_state_update(state, &report, now) == 1, "stat state update should succeed");
    mx6u_state_refresh_presence(state, now);
    mx6u_decision_observe_report(decision, &report);
}

static void test_builds_state_payload_with_env_decision_and_vision(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_decision_context_t decision;
    mx6u_vision_result_t vision;
    mx6u_mqtt_client_t mqtt;
    char buffer[MX6U_MQTT_PAYLOAD_MAX_BYTES];

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    mx6u_mqtt_init(&mqtt,
                   MX6U_MQTT_DEFAULT_BROKER,
                   MX6U_MQTT_DEFAULT_PORT,
                   MX6U_MQTT_DEFAULT_TOPIC,
                   "mx6u-test-client");

    update_env(&state, &decision, "ENV,N=1,T=22.2,H=59.0,SOIL=35.0,L=1100,CO2=350", 10);
    update_stat(&state, &decision, "STAT,N=4,F=OFF,P=ON,LED=OFF", 11);
    memset(&vision, 0, sizeof(vision));
    vision.available = 1;
    vision.brightness = 25.0f;
    vision.green_ratio = 0.35f;
    vision.motion_flag = 1;
    vision.vision_ok = 1;
    mx6u_decision_set_vision_result(&decision, &vision);
    mx6u_decision_evaluate(&decision, &state, 11, 9);

    expect_true(mx6u_mqtt_build_payload(&mqtt,
                                        &state,
                                        mx6u_decision_result(&decision),
                                        &vision,
                                        11,
                                        buffer,
                                        sizeof(buffer)) == 1,
                "payload should serialize");
    expect_true(strstr(buffer, "\"device\":\"mx6u\"") != NULL, "payload should include device");
    expect_true(strstr(buffer, "\"node_id\":1") != NULL, "payload should include node id");
    expect_true(strstr(buffer, "\"temp\":22.2") != NULL, "payload should include env temp");
    expect_true(strstr(buffer, "\"pump\":\"ON\"") != NULL, "payload should include decision");
    expect_true(strstr(buffer, "\"brightness\":25.0") != NULL, "payload should include vision brightness");
    expect_true(strstr(buffer, "\"sensor_1\":\"ONLINE\"") != NULL, "payload should include presence");
    expect_true(strstr(buffer, "\"executor_4\":\"ONLINE\"") != NULL, "payload should include executor presence");
}

static void test_builds_payload_without_stat_or_vision(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_decision_context_t decision;
    mx6u_vision_result_t vision;
    mx6u_mqtt_client_t mqtt;
    char buffer[MX6U_MQTT_PAYLOAD_MAX_BYTES];

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    mx6u_mqtt_init(&mqtt,
                   MX6U_MQTT_DEFAULT_BROKER,
                   MX6U_MQTT_DEFAULT_PORT,
                   MX6U_MQTT_DEFAULT_TOPIC,
                   "mx6u-test-client");

    update_env(&state, &decision, "ENV,N=1,T=23.0,H=60.0,SOIL=40.5,L=1300,CO2=420", 20);
    memset(&vision, 0, sizeof(vision));
    mx6u_decision_set_vision_result(&decision, &vision);
    mx6u_decision_evaluate(&decision, &state, 20, 10);

    expect_true(mx6u_mqtt_build_payload(&mqtt,
                                        &state,
                                        mx6u_decision_result(&decision),
                                        &vision,
                                        20,
                                        buffer,
                                        sizeof(buffer)) == 1,
                "payload should serialize without stat");
    expect_true(strstr(buffer, "\"executor_4\":\"UNKNOWN\"") != NULL, "payload should keep unknown executor presence");
    expect_true(strstr(buffer, "\"available\":false") != NULL, "payload should mark vision unavailable");
    expect_true(strstr(buffer, "\"led\":\"OFF\"") != NULL, "payload should keep decision output stable");
}

static void test_throttles_publish_to_once_per_ten_seconds(void)
{
    mx6u_mqtt_client_t mqtt;

    mx6u_mqtt_init(&mqtt,
                   MX6U_MQTT_DEFAULT_BROKER,
                   MX6U_MQTT_DEFAULT_PORT,
                   MX6U_MQTT_DEFAULT_TOPIC,
                   "mx6u-test-client");

    expect_true(mx6u_mqtt_should_publish(&mqtt, 100) == 1, "first publish time should be allowed");
    mqtt.last_publish_attempt_at = 100;
    expect_true(mx6u_mqtt_should_publish(&mqtt, 105) == 0, "publish inside ten-second window should be skipped");
    expect_true(mx6u_mqtt_should_publish(&mqtt, 110) == 1, "publish at ten-second boundary should be allowed");
}

int main(void)
{
    test_builds_state_payload_with_env_decision_and_vision();
    test_builds_payload_without_stat_or_vision();
    test_throttles_publish_to_once_per_ten_seconds();

    if (tests_failed != 0)
    {
        return 1;
    }

    printf("PASS\n");
    return 0;
}
