#ifndef MX6U_DECISION_H
#define MX6U_DECISION_H

#include <stdio.h>
#include <time.h>

#include "mx6u_protocol.h"
#include "mx6u_state.h"
#include "mx6u_vision.h"

#define MX6U_ENV_HISTORY_CAPACITY 3
#define MX6U_DAY_START_HOUR 6
#define MX6U_DAY_END_HOUR 18
#define MX6U_TEMPERATURE_FAN_ON_THRESHOLD 35.0f
#define MX6U_TEMPERATURE_FAN_OFF_THRESHOLD 30.0f
#define MX6U_SOIL_PUMP_ON_THRESHOLD 40.0f
#define MX6U_LIGHT_LED_ON_THRESHOLD 1200
#define MX6U_VISION_DARK_BRIGHTNESS_THRESHOLD 60.0f
#define MX6U_PREEMPTIVE_TEMPERATURE_DELTA 1.0f
#define MX6U_PREEMPTIVE_LIGHT_DELTA 100

typedef enum
{
    MX6U_ACTION_NONE = 0,
    MX6U_ACTION_OFF,
    MX6U_ACTION_ON
} mx6u_action_t;

typedef enum
{
    MX6U_RECOMMENDATION_NO_STATUS = 0,
    MX6U_RECOMMENDATION_MATCHED,
    MX6U_RECOMMENDATION_ACTION_NEEDED
} mx6u_recommendation_status_t;

typedef enum
{
    MX6U_TREND_INSUFFICIENT_DATA = 0,
    MX6U_TREND_STABLE,
    MX6U_TREND_RISING,
    MX6U_TREND_FALLING
} mx6u_trend_t;

typedef enum
{
    MX6U_ANOMALY_NONE = 0,
    MX6U_ANOMALY_NODE_OFFLINE = 1 << 0,
    MX6U_ANOMALY_FAN_NO_EFFECT = 1 << 1,
    MX6U_ANOMALY_PUMP_NO_EFFECT = 1 << 2,
    MX6U_ANOMALY_LIGHT_NO_EFFECT = 1 << 3
} mx6u_anomaly_flags_t;

typedef struct
{
    mx6u_action_t action;
    mx6u_recommendation_status_t status;
} mx6u_action_recommendation_t;

typedef struct
{
    env_report_t samples[MX6U_ENV_HISTORY_CAPACITY];
    int count;
} mx6u_env_history_t;

typedef struct
{
    mx6u_action_recommendation_t fan;
    mx6u_action_recommendation_t pump;
    mx6u_action_recommendation_t fill_light;
    mx6u_trend_t temp_trend;
    mx6u_trend_t light_trend;
    int preemptive_fan_on;
    int preemptive_led_on;
    int vision_supports_fill_light;
    unsigned int anomalies;
} mx6u_decision_result_t;

typedef struct
{
    mx6u_env_history_t env_history;
    mx6u_vision_result_t vision;
    mx6u_decision_result_t result;
} mx6u_decision_context_t;

void mx6u_decision_init(mx6u_decision_context_t *context);
void mx6u_decision_observe_report(mx6u_decision_context_t *context, const mx6u_report_t *report);
void mx6u_decision_set_vision_result(mx6u_decision_context_t *context, const mx6u_vision_result_t *vision);
void mx6u_decision_evaluate(mx6u_decision_context_t *context,
                            const mx6u_greenhouse_state_t *state,
                            time_t now,
                            int hour_of_day);
const mx6u_decision_result_t *mx6u_decision_result(const mx6u_decision_context_t *context);
void mx6u_decision_print_snapshot(const mx6u_decision_context_t *context, FILE *stream);

#endif
