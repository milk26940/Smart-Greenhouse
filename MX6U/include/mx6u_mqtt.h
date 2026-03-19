#ifndef MX6U_MQTT_H
#define MX6U_MQTT_H

#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include "mx6u_decision.h"
#include "mx6u_state.h"
#include "mx6u_vision.h"

#define MX6U_MQTT_DEFAULT_BROKER "broker.emqx.io"
#define MX6U_MQTT_DEFAULT_PORT 1883
#define MX6U_MQTT_DEFAULT_TOPIC "smart-greenhouse/node1/state"
#define MX6U_MQTT_DEFAULT_CLIENT_ID "mx6u-node1"
#define MX6U_MQTT_CONNECT_TIMEOUT_MS 1000
#define MX6U_MQTT_IO_TIMEOUT_MS 1000
#define MX6U_MQTT_KEEPALIVE_SECONDS 30
#define MX6U_MQTT_PUBLISH_INTERVAL_SECONDS 10
#define MX6U_MQTT_PAYLOAD_MAX_BYTES 1024

typedef struct
{
    char broker[64];
    int port;
    char topic[128];
    char client_id[64];
    int socket_fd;
    int enabled;
    int connected;
    int last_publish_ok;
    time_t last_publish_attempt_at;
    time_t last_io_at;
    int publish_interval_seconds;
    char last_error[128];
} mx6u_mqtt_client_t;

void mx6u_mqtt_init(mx6u_mqtt_client_t *client,
                    const char *broker,
                    int port,
                    const char *topic,
                    const char *client_id);
void mx6u_mqtt_shutdown(mx6u_mqtt_client_t *client);
int mx6u_mqtt_connect(mx6u_mqtt_client_t *client);
int mx6u_mqtt_should_publish(const mx6u_mqtt_client_t *client, time_t now);
int mx6u_mqtt_build_payload(const mx6u_mqtt_client_t *client,
                            const mx6u_greenhouse_state_t *state,
                            const mx6u_decision_result_t *decision,
                            const mx6u_vision_result_t *vision,
                            time_t now,
                            char *buffer,
                            size_t buffer_size);
int mx6u_mqtt_publish_state(mx6u_mqtt_client_t *client,
                            const mx6u_greenhouse_state_t *state,
                            const mx6u_decision_result_t *decision,
                            const mx6u_vision_result_t *vision,
                            time_t now);
void mx6u_mqtt_print_snapshot(const mx6u_mqtt_client_t *client, FILE *stream);

#endif
