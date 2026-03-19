#include <stdio.h>
#include <string.h>
#include <time.h>

#include "mx6u_decision.h"
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

static void test_recommends_fan_on_when_temperature_is_high(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_decision_context_t decision;
    const mx6u_decision_result_t *result;

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    update_env(&state, &decision, "ENV,N=1,T=36.5,H=60.0,SOIL=45.0,L=1300,CO2=400", 10);

    mx6u_decision_evaluate(&decision, &state, 10, 10);
    result = mx6u_decision_result(&decision);

    expect_true(result->fan.action == MX6U_ACTION_ON, "high temp should recommend fan on");
    expect_true(result->fan.status == MX6U_RECOMMENDATION_NO_STATUS, "no stat should keep fan status pending");
}

static void test_recommends_fan_off_when_temperature_is_low(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_decision_context_t decision;
    const mx6u_decision_result_t *result;

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    update_env(&state, &decision, "ENV,N=1,T=28.0,H=60.0,SOIL=45.0,L=1300,CO2=400", 20);

    mx6u_decision_evaluate(&decision, &state, 20, 10);
    result = mx6u_decision_result(&decision);

    expect_true(result->fan.action == MX6U_ACTION_OFF, "low temp should recommend fan off");
}

static void test_recommends_pump_on_when_soil_is_low(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_decision_context_t decision;
    const mx6u_decision_result_t *result;

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    update_env(&state, &decision, "ENV,N=1,T=30.0,H=60.0,SOIL=35.0,L=1300,CO2=400", 30);

    mx6u_decision_evaluate(&decision, &state, 30, 10);
    result = mx6u_decision_result(&decision);

    expect_true(result->pump.action == MX6U_ACTION_ON, "low soil should recommend pump on");
}

static void test_recommends_led_on_when_light_and_vision_are_both_dark(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_decision_context_t decision;
    mx6u_vision_result_t vision;
    const mx6u_decision_result_t *result;

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    update_env(&state, &decision, "ENV,N=1,T=30.0,H=60.0,SOIL=45.0,L=1100,CO2=400", 40);
    memset(&vision, 0, sizeof(vision));
    vision.available = 1;
    vision.brightness = 25.0f;
    vision.vision_ok = 1;
    mx6u_decision_set_vision_result(&decision, &vision);

    mx6u_decision_evaluate(&decision, &state, 40, 9);
    result = mx6u_decision_result(&decision);

    expect_true(result->fill_light.action == MX6U_ACTION_ON, "low light plus dark vision should recommend led on");
}

static void test_marks_fill_light_as_vision_supported_when_image_is_dark(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_decision_context_t decision;
    mx6u_vision_result_t vision;
    const mx6u_decision_result_t *result;

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    update_env(&state, &decision, "ENV,N=1,T=30.0,H=60.0,SOIL=45.0,L=1100,CO2=400", 45);

    memset(&vision, 0, sizeof(vision));
    vision.available = 1;
    vision.brightness = 25.0f;
    vision.green_ratio = 0.30f;
    vision.motion_flag = 0;
    vision.vision_ok = 1;

    mx6u_decision_set_vision_result(&decision, &vision);
    mx6u_decision_evaluate(&decision, &state, 45, 9);
    result = mx6u_decision_result(&decision);

    expect_true(result->fill_light.action == MX6U_ACTION_ON, "dark vision should keep led recommendation on");
    expect_true(result->vision_supports_fill_light == 1, "dark vision result should support fill light");
}

static void test_does_not_recommend_led_on_when_vision_is_bright(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_decision_context_t decision;
    mx6u_vision_result_t vision;
    const mx6u_decision_result_t *result;

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    update_env(&state, &decision, "ENV,N=1,T=30.0,H=60.0,SOIL=45.0,L=1100,CO2=400", 47);

    memset(&vision, 0, sizeof(vision));
    vision.available = 1;
    vision.brightness = 95.0f;
    vision.vision_ok = 1;
    mx6u_decision_set_vision_result(&decision, &vision);

    mx6u_decision_evaluate(&decision, &state, 47, 9);
    result = mx6u_decision_result(&decision);

    expect_true(result->fill_light.action == MX6U_ACTION_OFF, "bright vision should block led recommendation");
    expect_true(result->vision_supports_fill_light == 0, "bright vision should not support fill light");
}

static void test_keeps_sensor_only_led_fallback_when_vision_is_unavailable(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_decision_context_t decision;
    const mx6u_decision_result_t *result;

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    update_env(&state, &decision, "ENV,N=1,T=30.0,H=60.0,SOIL=45.0,L=1100,CO2=400", 49);

    mx6u_decision_evaluate(&decision, &state, 49, 9);
    result = mx6u_decision_result(&decision);

    expect_true(result->fill_light.action == MX6U_ACTION_ON, "unavailable vision should keep sensor-only led fallback");
    expect_true(result->vision_supports_fill_light == 0, "no vision should not mark fill light as supported");
}

static void test_marks_action_needed_when_stat_disagrees_with_recommendation(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_decision_context_t decision;
    const mx6u_decision_result_t *result;

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    update_env(&state, &decision, "ENV,N=1,T=36.5,H=60.0,SOIL=35.0,L=1100,CO2=400", 50);
    update_stat(&state, &decision, "STAT,N=4,F=OFF,P=OFF,LED=OFF", 51);

    mx6u_decision_evaluate(&decision, &state, 51, 10);
    result = mx6u_decision_result(&decision);

    expect_true(result->fan.status == MX6U_RECOMMENDATION_ACTION_NEEDED, "fan mismatch should request action");
}

static void test_detects_temperature_rising_trend(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_decision_context_t decision;
    const mx6u_decision_result_t *result;

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    update_env(&state, &decision, "ENV,N=1,T=32.0,H=60.0,SOIL=45.0,L=1300,CO2=400", 60);
    update_env(&state, &decision, "ENV,N=1,T=33.0,H=60.0,SOIL=45.0,L=1280,CO2=400", 65);
    update_env(&state, &decision, "ENV,N=1,T=34.0,H=60.0,SOIL=45.0,L=1260,CO2=400", 70);

    mx6u_decision_evaluate(&decision, &state, 70, 10);
    result = mx6u_decision_result(&decision);

    expect_true(result->temp_trend == MX6U_TREND_RISING, "three rising temps should mark rising trend");
    expect_true(result->preemptive_fan_on == 1, "near-threshold rising temp should trigger preemptive fan");
}

static void test_detects_fan_anomaly_when_temperature_does_not_drop(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_decision_context_t decision;
    const mx6u_decision_result_t *result;

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    update_stat(&state, &decision, "STAT,N=4,F=ON,P=OFF,LED=OFF", 80);
    update_env(&state, &decision, "ENV,N=1,T=34.0,H=60.0,SOIL=45.0,L=1300,CO2=400", 81);
    update_env(&state, &decision, "ENV,N=1,T=34.5,H=60.0,SOIL=45.0,L=1290,CO2=400", 86);
    update_env(&state, &decision, "ENV,N=1,T=35.0,H=60.0,SOIL=45.0,L=1280,CO2=400", 91);

    mx6u_decision_evaluate(&decision, &state, 91, 10);
    result = mx6u_decision_result(&decision);

    expect_true((result->anomalies & MX6U_ANOMALY_FAN_NO_EFFECT) != 0, "fan with no cooling effect should be flagged");
}

static void test_detects_offline_anomaly(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_decision_context_t decision;
    const mx6u_decision_result_t *result;

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    update_env(&state, &decision, "ENV,N=1,T=30.0,H=60.0,SOIL=45.0,L=1300,CO2=400", 100);
    mx6u_state_refresh_presence(&state, 116);

    mx6u_decision_evaluate(&decision, &state, 116, 10);
    result = mx6u_decision_result(&decision);

    expect_true((result->anomalies & MX6U_ANOMALY_NODE_OFFLINE) != 0, "offline nodes should be flagged");
}

static void test_insufficient_history_keeps_trend_unknown(void)
{
    mx6u_greenhouse_state_t state;
    mx6u_decision_context_t decision;
    const mx6u_decision_result_t *result;

    mx6u_state_init(&state);
    mx6u_decision_init(&decision);
    update_env(&state, &decision, "ENV,N=1,T=33.0,H=60.0,SOIL=45.0,L=1300,CO2=400", 120);

    mx6u_decision_evaluate(&decision, &state, 120, 10);
    result = mx6u_decision_result(&decision);

    expect_true(result->temp_trend == MX6U_TREND_INSUFFICIENT_DATA, "single sample should not create temp trend");
    expect_true(result->light_trend == MX6U_TREND_INSUFFICIENT_DATA, "single sample should not create light trend");
    expect_true(result->anomalies == MX6U_ANOMALY_NONE, "single sample should not create anomalies");
}

int main(void)
{
    test_recommends_fan_on_when_temperature_is_high();
    test_recommends_fan_off_when_temperature_is_low();
    test_recommends_pump_on_when_soil_is_low();
    test_recommends_led_on_when_light_and_vision_are_both_dark();
    test_marks_fill_light_as_vision_supported_when_image_is_dark();
    test_does_not_recommend_led_on_when_vision_is_bright();
    test_keeps_sensor_only_led_fallback_when_vision_is_unavailable();
    test_marks_action_needed_when_stat_disagrees_with_recommendation();
    test_detects_temperature_rising_trend();
    test_detects_fan_anomaly_when_temperature_does_not_drop();
    test_detects_offline_anomaly();
    test_insufficient_history_keeps_trend_unknown();

    if (tests_failed != 0)
    {
        return 1;
    }

    printf("PASS\n");
    return 0;
}
