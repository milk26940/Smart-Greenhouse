#include "mx6u_decision.h"

#include <string.h>

static void mx6u_decision_reset_result(mx6u_decision_result_t *result)
{
    memset(result, 0, sizeof(*result));
    result->fan.action = MX6U_ACTION_NONE;
    result->pump.action = MX6U_ACTION_NONE;
    result->fill_light.action = MX6U_ACTION_NONE;
    result->fan.status = MX6U_RECOMMENDATION_NO_STATUS;
    result->pump.status = MX6U_RECOMMENDATION_NO_STATUS;
    result->fill_light.status = MX6U_RECOMMENDATION_NO_STATUS;
    result->temp_trend = MX6U_TREND_INSUFFICIENT_DATA;
    result->light_trend = MX6U_TREND_INSUFFICIENT_DATA;
}

void mx6u_decision_set_vision_result(mx6u_decision_context_t *context, const mx6u_vision_result_t *vision)
{
    if ((context == NULL) || (vision == NULL))
    {
        return;
    }

    context->vision = *vision;
}

static const char *mx6u_action_name(mx6u_action_t action)
{
    switch (action)
    {
    case MX6U_ACTION_ON:
        return "ON";
    case MX6U_ACTION_OFF:
        return "OFF";
    default:
        return "NONE";
    }
}

static const char *mx6u_recommendation_status_name(mx6u_recommendation_status_t status)
{
    switch (status)
    {
    case MX6U_RECOMMENDATION_MATCHED:
        return "MATCHED";
    case MX6U_RECOMMENDATION_ACTION_NEEDED:
        return "ACTION_NEEDED";
    default:
        return "NO_STATUS";
    }
}

static const char *mx6u_trend_name(mx6u_trend_t trend)
{
    switch (trend)
    {
    case MX6U_TREND_RISING:
        return "RISING";
    case MX6U_TREND_FALLING:
        return "FALLING";
    case MX6U_TREND_STABLE:
        return "STABLE";
    default:
        return "INSUFFICIENT_DATA";
    }
}

static void mx6u_decision_apply_status(mx6u_action_recommendation_t *recommendation,
                                       const mx6u_node_state_t *executor_node,
                                       int current_on)
{
    if ((recommendation == NULL) || (executor_node == NULL))
    {
        return;
    }

    if ((executor_node->presence != MX6U_NODE_ONLINE) ||
        (!executor_node->has_report) ||
        (executor_node->last_report.type != MX6U_REPORT_STATUS) ||
        (recommendation->action == MX6U_ACTION_NONE))
    {
        recommendation->status = MX6U_RECOMMENDATION_NO_STATUS;
        return;
    }

    if (((recommendation->action == MX6U_ACTION_ON) && current_on) ||
        ((recommendation->action == MX6U_ACTION_OFF) && !current_on))
    {
        recommendation->status = MX6U_RECOMMENDATION_MATCHED;
        return;
    }

    recommendation->status = MX6U_RECOMMENDATION_ACTION_NEEDED;
}

static mx6u_trend_t mx6u_decision_temperature_trend(const mx6u_env_history_t *history)
{
    if ((history == NULL) || (history->count < MX6U_ENV_HISTORY_CAPACITY))
    {
        return MX6U_TREND_INSUFFICIENT_DATA;
    }

    if ((history->samples[0].temp < history->samples[1].temp) &&
        (history->samples[1].temp < history->samples[2].temp))
    {
        return MX6U_TREND_RISING;
    }

    if ((history->samples[0].temp > history->samples[1].temp) &&
        (history->samples[1].temp > history->samples[2].temp))
    {
        return MX6U_TREND_FALLING;
    }

    return MX6U_TREND_STABLE;
}

static mx6u_trend_t mx6u_decision_light_trend(const mx6u_env_history_t *history)
{
    if ((history == NULL) || (history->count < MX6U_ENV_HISTORY_CAPACITY))
    {
        return MX6U_TREND_INSUFFICIENT_DATA;
    }

    if ((history->samples[0].light > history->samples[1].light) &&
        (history->samples[1].light > history->samples[2].light))
    {
        return MX6U_TREND_FALLING;
    }

    if ((history->samples[0].light < history->samples[1].light) &&
        (history->samples[1].light < history->samples[2].light))
    {
        return MX6U_TREND_RISING;
    }

    return MX6U_TREND_STABLE;
}

static int mx6u_decision_should_turn_on_fill_light(const mx6u_decision_context_t *context,
                                                   const env_report_t *env,
                                                   int hour_of_day)
{
    if ((context == NULL) || (env == NULL))
    {
        return 0;
    }

    if ((hour_of_day < MX6U_DAY_START_HOUR) ||
        (hour_of_day >= MX6U_DAY_END_HOUR) ||
        (env->light >= MX6U_LIGHT_LED_ON_THRESHOLD))
    {
        return 0;
    }

    if (!context->vision.available)
    {
        return 1;
    }

    return context->vision.brightness < MX6U_VISION_DARK_BRIGHTNESS_THRESHOLD;
}

static void mx6u_decision_detect_offline_anomalies(mx6u_decision_result_t *result,
                                                   const mx6u_greenhouse_state_t *state)
{
    int i;

    for (i = 0; i < MX6U_NODE_COUNT; ++i)
    {
        if (state->nodes[i].presence == MX6U_NODE_OFFLINE)
        {
            result->anomalies |= MX6U_ANOMALY_NODE_OFFLINE;
            return;
        }
    }
}

void mx6u_decision_init(mx6u_decision_context_t *context)
{
    if (context == NULL)
    {
        return;
    }

    memset(context, 0, sizeof(*context));
    mx6u_decision_reset_result(&context->result);
}

void mx6u_decision_observe_report(mx6u_decision_context_t *context, const mx6u_report_t *report)
{
    int i;

    if ((context == NULL) || (report == NULL))
    {
        return;
    }

    if ((report->type != MX6U_REPORT_ENV) || (report->data.env.node_id != 1))
    {
        return;
    }

    if (context->env_history.count < MX6U_ENV_HISTORY_CAPACITY)
    {
        context->env_history.samples[context->env_history.count++] = report->data.env;
        return;
    }

    for (i = 1; i < MX6U_ENV_HISTORY_CAPACITY; ++i)
    {
        context->env_history.samples[i - 1] = context->env_history.samples[i];
    }

    context->env_history.samples[MX6U_ENV_HISTORY_CAPACITY - 1] = report->data.env;
}

void mx6u_decision_evaluate(mx6u_decision_context_t *context,
                            const mx6u_greenhouse_state_t *state,
                            time_t now,
                            int hour_of_day)
{
    const mx6u_node_state_t *sensor = NULL;
    const mx6u_node_state_t *executor = NULL;
    const env_report_t *env = NULL;

    (void)now;

    if ((context == NULL) || (state == NULL))
    {
        return;
    }

    mx6u_decision_reset_result(&context->result);
    sensor = mx6u_state_get_node(state, 1);
    executor = mx6u_state_get_node(state, 4);

    if ((sensor == NULL) || (!sensor->has_report) || (sensor->last_report.type != MX6U_REPORT_ENV))
    {
        mx6u_decision_detect_offline_anomalies(&context->result, state);
        return;
    }

    env = &sensor->last_report.data.env;

    if (env->temp > MX6U_TEMPERATURE_FAN_ON_THRESHOLD)
    {
        context->result.fan.action = MX6U_ACTION_ON;
    }
    else if (env->temp < MX6U_TEMPERATURE_FAN_OFF_THRESHOLD)
    {
        context->result.fan.action = MX6U_ACTION_OFF;
    }

    if (env->soil < MX6U_SOIL_PUMP_ON_THRESHOLD)
    {
        context->result.pump.action = MX6U_ACTION_ON;
    }
    else
    {
        context->result.pump.action = MX6U_ACTION_OFF;
    }

    if (mx6u_decision_should_turn_on_fill_light(context, env, hour_of_day))
    {
        context->result.fill_light.action = MX6U_ACTION_ON;
    }
    else
    {
        context->result.fill_light.action = MX6U_ACTION_OFF;
    }

    context->result.temp_trend = mx6u_decision_temperature_trend(&context->env_history);
    context->result.light_trend = mx6u_decision_light_trend(&context->env_history);

    if ((context->result.temp_trend == MX6U_TREND_RISING) &&
        (env->temp >= (MX6U_TEMPERATURE_FAN_ON_THRESHOLD - MX6U_PREEMPTIVE_TEMPERATURE_DELTA)))
    {
        context->result.preemptive_fan_on = 1;
    }

    if ((context->result.light_trend == MX6U_TREND_FALLING) &&
        (env->light <= (MX6U_LIGHT_LED_ON_THRESHOLD + MX6U_PREEMPTIVE_LIGHT_DELTA)))
    {
        context->result.preemptive_led_on = 1;
    }

    if ((context->result.fill_light.action == MX6U_ACTION_ON) &&
        context->vision.available &&
        (context->vision.brightness < MX6U_VISION_DARK_BRIGHTNESS_THRESHOLD))
    {
        context->result.vision_supports_fill_light = 1;
    }

    if (executor != NULL)
    {
        int fan_on = 0;
        int pump_on = 0;
        int led_on = 0;

        if ((executor->has_report) && (executor->last_report.type == MX6U_REPORT_STATUS))
        {
            fan_on = executor->last_report.data.status.fan_on;
            pump_on = executor->last_report.data.status.pump_on;
            led_on = executor->last_report.data.status.fill_light_on;
        }

        mx6u_decision_apply_status(&context->result.fan, executor, fan_on);
        mx6u_decision_apply_status(&context->result.pump, executor, pump_on);
        mx6u_decision_apply_status(&context->result.fill_light, executor, led_on);

        if ((executor->presence == MX6U_NODE_ONLINE) &&
            (executor->has_report) &&
            (executor->last_report.type == MX6U_REPORT_STATUS) &&
            (context->env_history.count >= MX6U_ENV_HISTORY_CAPACITY))
        {
            if (executor->last_report.data.status.fan_on &&
                (context->env_history.samples[2].temp >= context->env_history.samples[0].temp))
            {
                context->result.anomalies |= MX6U_ANOMALY_FAN_NO_EFFECT;
            }

            if (executor->last_report.data.status.pump_on &&
                (context->env_history.samples[2].soil <= context->env_history.samples[0].soil))
            {
                context->result.anomalies |= MX6U_ANOMALY_PUMP_NO_EFFECT;
            }

            if (executor->last_report.data.status.fill_light_on &&
                (context->env_history.samples[2].light <= context->env_history.samples[0].light))
            {
                context->result.anomalies |= MX6U_ANOMALY_LIGHT_NO_EFFECT;
            }
        }
    }

    mx6u_decision_detect_offline_anomalies(&context->result, state);
}

const mx6u_decision_result_t *mx6u_decision_result(const mx6u_decision_context_t *context)
{
    if (context == NULL)
    {
        return NULL;
    }

    return &context->result;
}

void mx6u_decision_print_snapshot(const mx6u_decision_context_t *context, FILE *stream)
{
    const mx6u_decision_result_t *result = NULL;

    if ((context == NULL) || (stream == NULL))
    {
        return;
    }

    result = &context->result;

    fputs("DECISION SNAPSHOT\n", stream);
    fprintf(stream,
            "RECOMMEND fan=%s(%s) pump=%s(%s) led=%s(%s)\n",
            mx6u_action_name(result->fan.action),
            mx6u_recommendation_status_name(result->fan.status),
            mx6u_action_name(result->pump.action),
            mx6u_recommendation_status_name(result->pump.status),
            mx6u_action_name(result->fill_light.action),
            mx6u_recommendation_status_name(result->fill_light.status));
    fprintf(stream,
            "TREND temp=%s light=%s preemptive_fan=%s preemptive_led=%s vision_fill_light_support=%s\n",
            mx6u_trend_name(result->temp_trend),
            mx6u_trend_name(result->light_trend),
            result->preemptive_fan_on ? "YES" : "NO",
            result->preemptive_led_on ? "YES" : "NO",
            result->vision_supports_fill_light ? "YES" : "NO");
    fprintf(stream,
            "ANOMALY offline=%s fan_no_effect=%s pump_no_effect=%s light_no_effect=%s\n",
            (result->anomalies & MX6U_ANOMALY_NODE_OFFLINE) ? "YES" : "NO",
            (result->anomalies & MX6U_ANOMALY_FAN_NO_EFFECT) ? "YES" : "NO",
            (result->anomalies & MX6U_ANOMALY_PUMP_NO_EFFECT) ? "YES" : "NO",
            (result->anomalies & MX6U_ANOMALY_LIGHT_NO_EFFECT) ? "YES" : "NO");
}
