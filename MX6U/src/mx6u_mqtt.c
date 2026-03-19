#ifndef _WIN32
#define _POSIX_C_SOURCE 200112L
#endif

#include "mx6u_mqtt.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET mx6u_socket_native_t;
#define MX6U_INVALID_SOCKET INVALID_SOCKET
#define mx6u_socket_close closesocket
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int mx6u_socket_native_t;
#define MX6U_INVALID_SOCKET (-1)
#define mx6u_socket_close close
#endif

#define MX6U_MQTT_CONNECT_PACKET_TYPE 0x10
#define MX6U_MQTT_CONNACK_PACKET_TYPE 0x20
#define MX6U_MQTT_PUBLISH_PACKET_TYPE 0x30
#define MX6U_MQTT_PINGREQ_PACKET_TYPE 0xC0
#define MX6U_MQTT_PINGRESP_PACKET_TYPE 0xD0

static void mx6u_mqtt_copy_string(char *dest, size_t dest_size, const char *src)
{
    if ((dest == NULL) || (dest_size == 0))
    {
        return;
    }

    if (src == NULL)
    {
        dest[0] = '\0';
        return;
    }

    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

static void mx6u_mqtt_set_error(mx6u_mqtt_client_t *client, const char *format, ...)
{
    va_list args;

    if (client == NULL)
    {
        return;
    }

    va_start(args, format);
    vsnprintf(client->last_error, sizeof(client->last_error), format, args);
    va_end(args);
}

static const char *mx6u_mqtt_presence_name(mx6u_node_presence_t presence)
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

static const char *mx6u_mqtt_action_name(mx6u_action_t action)
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

static const char *mx6u_mqtt_trend_name(mx6u_trend_t trend)
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

static int mx6u_mqtt_encode_remaining_length(unsigned char *buffer, size_t buffer_size, size_t length)
{
    int index = 0;

    do
    {
        unsigned char encoded = (unsigned char)(length % 128);

        length /= 128;
        if (length > 0)
        {
            encoded |= 0x80;
        }

        if ((size_t)index >= buffer_size)
        {
            return -1;
        }

        buffer[index++] = encoded;
    } while (length > 0);

    return index;
}

static int mx6u_mqtt_wait_socket(mx6u_socket_native_t socket_fd, int for_write, int timeout_ms)
{
    fd_set fds;
    struct timeval timeout;

    FD_ZERO(&fds);
    FD_SET(socket_fd, &fds);
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    return select((int)socket_fd + 1,
                  for_write ? NULL : &fds,
                  for_write ? &fds : NULL,
                  NULL,
                  &timeout);
}

static int mx6u_mqtt_set_nonblocking(mx6u_socket_native_t socket_fd, int enabled)
{
#ifdef _WIN32
    u_long mode = enabled ? 1UL : 0UL;
    return ioctlsocket(socket_fd, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(socket_fd, F_GETFL, 0);

    if (flags < 0)
    {
        return 0;
    }

    if (enabled)
    {
        flags |= O_NONBLOCK;
    }
    else
    {
        flags &= ~O_NONBLOCK;
    }

    return fcntl(socket_fd, F_SETFL, flags) == 0;
#endif
}

static int mx6u_mqtt_socket_error(mx6u_socket_native_t socket_fd)
{
    int error_code = 0;
    socklen_t error_size = (socklen_t)sizeof(error_code);

    if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, (char *)&error_code, &error_size) != 0)
    {
        return -1;
    }

    return error_code;
}

static int mx6u_mqtt_send_all(mx6u_socket_native_t socket_fd, const unsigned char *buffer, size_t length)
{
    size_t offset = 0;

    while (offset < length)
    {
#ifdef _WIN32
        int sent = send(socket_fd, (const char *)(buffer + offset), (int)(length - offset), 0);
#else
        ssize_t sent = send(socket_fd, buffer + offset, length - offset, 0);
#endif
        if (sent <= 0)
        {
            return 0;
        }

        offset += (size_t)sent;
    }

    return 1;
}

static int mx6u_mqtt_recv_exact(mx6u_socket_native_t socket_fd, unsigned char *buffer, size_t length, int timeout_ms)
{
    size_t offset = 0;

    while (offset < length)
    {
        int ready = mx6u_mqtt_wait_socket(socket_fd, 0, timeout_ms);

        if (ready <= 0)
        {
            return 0;
        }

#ifdef _WIN32
        int received = recv(socket_fd, (char *)(buffer + offset), (int)(length - offset), 0);
#else
        ssize_t received = recv(socket_fd, buffer + offset, length - offset, 0);
#endif
        if (received <= 0)
        {
            return 0;
        }

        offset += (size_t)received;
    }

    return 1;
}

static void mx6u_mqtt_disconnect(mx6u_mqtt_client_t *client)
{
    if ((client == NULL) || (client->socket_fd == (int)MX6U_INVALID_SOCKET))
    {
        return;
    }

    mx6u_socket_close((mx6u_socket_native_t)client->socket_fd);
    client->socket_fd = (int)MX6U_INVALID_SOCKET;
    client->connected = 0;
}

static int mx6u_mqtt_write_utf(unsigned char *buffer, size_t buffer_size, const char *value)
{
    size_t value_length = value ? strlen(value) : 0;

    if (buffer_size < (value_length + 2))
    {
        return -1;
    }

    buffer[0] = (unsigned char)((value_length >> 8) & 0xFF);
    buffer[1] = (unsigned char)(value_length & 0xFF);
    if (value_length > 0)
    {
        memcpy(buffer + 2, value, value_length);
    }

    return (int)(value_length + 2);
}

void mx6u_mqtt_init(mx6u_mqtt_client_t *client,
                    const char *broker,
                    int port,
                    const char *topic,
                    const char *client_id)
{
    const char *env_enabled = getenv("MX6U_MQTT_ENABLE");
#ifdef _WIN32
    static int winsock_initialized = 0;
#endif

    if (client == NULL)
    {
        return;
    }

    memset(client, 0, sizeof(*client));
    mx6u_mqtt_copy_string(client->broker, sizeof(client->broker), broker ? broker : MX6U_MQTT_DEFAULT_BROKER);
    mx6u_mqtt_copy_string(client->topic, sizeof(client->topic), topic ? topic : MX6U_MQTT_DEFAULT_TOPIC);
    mx6u_mqtt_copy_string(client->client_id, sizeof(client->client_id), client_id ? client_id : MX6U_MQTT_DEFAULT_CLIENT_ID);
    client->port = (port > 0) ? port : MX6U_MQTT_DEFAULT_PORT;
    client->socket_fd = (int)MX6U_INVALID_SOCKET;
    client->enabled = !((env_enabled != NULL) && (strcmp(env_enabled, "0") == 0));
    client->last_publish_ok = 0;
    client->last_publish_attempt_at = 0;
    mx6u_mqtt_set_error(client, "not connected");
    client->publish_interval_seconds = MX6U_MQTT_PUBLISH_INTERVAL_SECONDS;

#ifdef _WIN32
    if (!winsock_initialized)
    {
        WSADATA wsa_data;

        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0)
        {
            winsock_initialized = 1;
        }
        else
        {
            client->enabled = 0;
            mx6u_mqtt_set_error(client, "winsock startup failed");
        }
    }
#endif
}

void mx6u_mqtt_shutdown(mx6u_mqtt_client_t *client)
{
    if (client == NULL)
    {
        return;
    }

    mx6u_mqtt_disconnect(client);
}

int mx6u_mqtt_should_publish(const mx6u_mqtt_client_t *client, time_t now)
{
    if (client == NULL)
    {
        return 0;
    }

    if (client->last_publish_attempt_at == 0)
    {
        return 1;
    }

    return (now - client->last_publish_attempt_at) >= client->publish_interval_seconds;
}

int mx6u_mqtt_connect(mx6u_mqtt_client_t *client)
{
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    struct addrinfo *entry = NULL;
    char port_buffer[16];
    int getaddrinfo_result = 0;
    unsigned char packet[256];
    unsigned char connack[4];

    if (client == NULL)
    {
        return 0;
    }

    if (!client->enabled)
    {
        mx6u_mqtt_set_error(client, "disabled");
        return 0;
    }

    if (client->connected)
    {
        return 1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(port_buffer, sizeof(port_buffer), "%d", client->port);

    getaddrinfo_result = getaddrinfo(client->broker, port_buffer, &hints, &result);
    if (getaddrinfo_result != 0)
    {
        mx6u_mqtt_set_error(client, "dns failed for %s", client->broker);
        return 0;
    }

    for (entry = result; entry != NULL; entry = entry->ai_next)
    {
        mx6u_socket_native_t socket_fd = socket(entry->ai_family, entry->ai_socktype, entry->ai_protocol);

        if (socket_fd == MX6U_INVALID_SOCKET)
        {
            continue;
        }

        if (!mx6u_mqtt_set_nonblocking(socket_fd, 1))
        {
            mx6u_socket_close(socket_fd);
            continue;
        }

        if (connect(socket_fd, entry->ai_addr, (socklen_t)entry->ai_addrlen) != 0)
        {
            int ready = mx6u_mqtt_wait_socket(socket_fd, 1, MX6U_MQTT_CONNECT_TIMEOUT_MS);
            int socket_error = 0;

            if (ready <= 0)
            {
                mx6u_socket_close(socket_fd);
                continue;
            }

            socket_error = mx6u_mqtt_socket_error(socket_fd);
            if (socket_error != 0)
            {
                mx6u_socket_close(socket_fd);
                continue;
            }
        }

        if (!mx6u_mqtt_set_nonblocking(socket_fd, 0))
        {
            mx6u_socket_close(socket_fd);
            continue;
        }

        client->socket_fd = (int)socket_fd;
        break;
    }

    freeaddrinfo(result);

    if (client->socket_fd == (int)MX6U_INVALID_SOCKET)
    {
        mx6u_mqtt_set_error(client, "tcp connect failed");
        return 0;
    }

    {
        int offset = 0;
        int utf_length = 0;
        int remaining_length = 0;
        int encoded_length = 0;

        packet[offset++] = MX6U_MQTT_CONNECT_PACKET_TYPE;
        remaining_length = 10 + 2 + (int)strlen(client->client_id);
        encoded_length = mx6u_mqtt_encode_remaining_length(packet + offset, sizeof(packet) - (size_t)offset, (size_t)remaining_length);
        if (encoded_length < 0)
        {
            mx6u_mqtt_disconnect(client);
            mx6u_mqtt_set_error(client, "connect packet too large");
            return 0;
        }
        offset += encoded_length;

        utf_length = mx6u_mqtt_write_utf(packet + offset, sizeof(packet) - (size_t)offset, "MQTT");
        if (utf_length < 0)
        {
            mx6u_mqtt_disconnect(client);
            mx6u_mqtt_set_error(client, "protocol header failed");
            return 0;
        }
        offset += utf_length;
        packet[offset++] = 0x04;
        packet[offset++] = 0x02;
        packet[offset++] = 0x00;
        packet[offset++] = MX6U_MQTT_KEEPALIVE_SECONDS;
        utf_length = mx6u_mqtt_write_utf(packet + offset, sizeof(packet) - (size_t)offset, client->client_id);
        if (utf_length < 0)
        {
            mx6u_mqtt_disconnect(client);
            mx6u_mqtt_set_error(client, "client id failed");
            return 0;
        }
        offset += utf_length;

        if (!mx6u_mqtt_send_all((mx6u_socket_native_t)client->socket_fd, packet, (size_t)offset) ||
            !mx6u_mqtt_recv_exact((mx6u_socket_native_t)client->socket_fd, connack, sizeof(connack), MX6U_MQTT_IO_TIMEOUT_MS))
        {
            mx6u_mqtt_disconnect(client);
            mx6u_mqtt_set_error(client, "connect handshake failed");
            return 0;
        }
    }

    if ((connack[0] != MX6U_MQTT_CONNACK_PACKET_TYPE) ||
        (connack[1] != 0x02) ||
        (connack[3] != 0x00))
    {
        mx6u_mqtt_disconnect(client);
        mx6u_mqtt_set_error(client, "broker rejected connection");
        return 0;
    }

    client->connected = 1;
    client->last_io_at = time(NULL);
    mx6u_mqtt_set_error(client, "ready");
    return 1;
}

int mx6u_mqtt_build_payload(const mx6u_mqtt_client_t *client,
                            const mx6u_greenhouse_state_t *state,
                            const mx6u_decision_result_t *decision,
                            const mx6u_vision_result_t *vision,
                            time_t now,
                            char *buffer,
                            size_t buffer_size)
{
    const mx6u_node_state_t *sensor_1 = NULL;
    const mx6u_node_state_t *sensor_2 = NULL;
    const mx6u_node_state_t *sensor_3 = NULL;
    const mx6u_node_state_t *executor_4 = NULL;
    const mx6u_node_state_t *executor_5 = NULL;
    env_report_t empty_env;
    mx6u_vision_result_t empty_vision;
    int written = 0;

    (void)client;

    if ((state == NULL) || (decision == NULL) || (buffer == NULL) || (buffer_size == 0))
    {
        return 0;
    }

    memset(&empty_env, 0, sizeof(empty_env));
    memset(&empty_vision, 0, sizeof(empty_vision));

    sensor_1 = mx6u_state_get_node(state, 1);
    sensor_2 = mx6u_state_get_node(state, 2);
    sensor_3 = mx6u_state_get_node(state, 3);
    executor_4 = mx6u_state_get_node(state, 4);
    executor_5 = mx6u_state_get_node(state, 5);
    if (vision == NULL)
    {
        vision = &empty_vision;
    }

    written = snprintf(
        buffer,
        buffer_size,
        "{\"device\":\"mx6u\",\"node_id\":1,\"timestamp\":%ld,"
        "\"env\":{\"temp\":%.1f,\"hum\":%.1f,\"soil\":%.1f,\"light\":%d,\"co2\":%d},"
        "\"decision\":{\"fan\":\"%s\",\"pump\":\"%s\",\"led\":\"%s\",\"temp_trend\":\"%s\",\"light_trend\":\"%s\","
        "\"preemptive_fan_on\":%s,\"preemptive_led_on\":%s,\"vision_fill_light_support\":%s,"
        "\"anomalies\":{\"offline\":%s,\"fan_no_effect\":%s,\"pump_no_effect\":%s,\"light_no_effect\":%s}},"
        "\"vision\":{\"available\":%s,\"brightness\":%.1f,\"green_ratio\":%.3f,\"motion_flag\":%s,\"vision_ok\":%s},"
        "\"presence\":{\"sensor_1\":\"%s\",\"sensor_2\":\"%s\",\"sensor_3\":\"%s\",\"executor_4\":\"%s\",\"executor_5\":\"%s\"}}",
        (long)now,
        (sensor_1 && sensor_1->has_report && sensor_1->last_report.type == MX6U_REPORT_ENV) ? sensor_1->last_report.data.env.temp : empty_env.temp,
        (sensor_1 && sensor_1->has_report && sensor_1->last_report.type == MX6U_REPORT_ENV) ? sensor_1->last_report.data.env.hum : empty_env.hum,
        (sensor_1 && sensor_1->has_report && sensor_1->last_report.type == MX6U_REPORT_ENV) ? sensor_1->last_report.data.env.soil : empty_env.soil,
        (sensor_1 && sensor_1->has_report && sensor_1->last_report.type == MX6U_REPORT_ENV) ? sensor_1->last_report.data.env.light : empty_env.light,
        (sensor_1 && sensor_1->has_report && sensor_1->last_report.type == MX6U_REPORT_ENV) ? sensor_1->last_report.data.env.co2 : empty_env.co2,
        mx6u_mqtt_action_name(decision->fan.action),
        mx6u_mqtt_action_name(decision->pump.action),
        mx6u_mqtt_action_name(decision->fill_light.action),
        mx6u_mqtt_trend_name(decision->temp_trend),
        mx6u_mqtt_trend_name(decision->light_trend),
        decision->preemptive_fan_on ? "true" : "false",
        decision->preemptive_led_on ? "true" : "false",
        decision->vision_supports_fill_light ? "true" : "false",
        (decision->anomalies & MX6U_ANOMALY_NODE_OFFLINE) ? "true" : "false",
        (decision->anomalies & MX6U_ANOMALY_FAN_NO_EFFECT) ? "true" : "false",
        (decision->anomalies & MX6U_ANOMALY_PUMP_NO_EFFECT) ? "true" : "false",
        (decision->anomalies & MX6U_ANOMALY_LIGHT_NO_EFFECT) ? "true" : "false",
        vision->available ? "true" : "false",
        vision->brightness,
        vision->green_ratio,
        vision->motion_flag ? "true" : "false",
        vision->vision_ok ? "true" : "false",
        sensor_1 ? mx6u_mqtt_presence_name(sensor_1->presence) : "UNKNOWN",
        sensor_2 ? mx6u_mqtt_presence_name(sensor_2->presence) : "UNKNOWN",
        sensor_3 ? mx6u_mqtt_presence_name(sensor_3->presence) : "UNKNOWN",
        executor_4 ? mx6u_mqtt_presence_name(executor_4->presence) : "UNKNOWN",
        executor_5 ? mx6u_mqtt_presence_name(executor_5->presence) : "UNKNOWN");

    return (written > 0) && ((size_t)written < buffer_size);
}

static int mx6u_mqtt_send_ping(mx6u_mqtt_client_t *client)
{
    unsigned char pingreq[2] = {MX6U_MQTT_PINGREQ_PACKET_TYPE, 0x00};
    unsigned char pingresp[2];

    if ((client == NULL) || !client->connected)
    {
        return 0;
    }

    if (!mx6u_mqtt_send_all((mx6u_socket_native_t)client->socket_fd, pingreq, sizeof(pingreq)) ||
        !mx6u_mqtt_recv_exact((mx6u_socket_native_t)client->socket_fd, pingresp, sizeof(pingresp), MX6U_MQTT_IO_TIMEOUT_MS))
    {
        mx6u_mqtt_disconnect(client);
        mx6u_mqtt_set_error(client, "ping failed");
        return 0;
    }

    if ((pingresp[0] != MX6U_MQTT_PINGRESP_PACKET_TYPE) || (pingresp[1] != 0x00))
    {
        mx6u_mqtt_disconnect(client);
        mx6u_mqtt_set_error(client, "invalid ping response");
        return 0;
    }

    client->last_io_at = time(NULL);
    return 1;
}

int mx6u_mqtt_publish_state(mx6u_mqtt_client_t *client,
                            const mx6u_greenhouse_state_t *state,
                            const mx6u_decision_result_t *decision,
                            const mx6u_vision_result_t *vision,
                            time_t now)
{
    char payload[MX6U_MQTT_PAYLOAD_MAX_BYTES];
    unsigned char packet[1400];
    int topic_length = 0;
    int payload_length = 0;
    int offset = 0;
    int encoded_length = 0;

    if (client == NULL)
    {
        return 0;
    }

    if (!client->enabled)
    {
        client->last_publish_ok = 0;
        mx6u_mqtt_set_error(client, "disabled");
        return 0;
    }

    if (!mx6u_mqtt_should_publish(client, now))
    {
        mx6u_mqtt_set_error(client, "throttled");
        return 1;
    }

    client->last_publish_attempt_at = now;

    if (!mx6u_mqtt_build_payload(client, state, decision, vision, now, payload, sizeof(payload)))
    {
        client->last_publish_ok = 0;
        mx6u_mqtt_set_error(client, "payload serialization failed");
        return 0;
    }

    if (!client->connected && !mx6u_mqtt_connect(client))
    {
        client->last_publish_ok = 0;
        return 0;
    }

    if ((client->last_io_at != 0) && ((now - client->last_io_at) >= MX6U_MQTT_KEEPALIVE_SECONDS))
    {
        if (!mx6u_mqtt_send_ping(client))
        {
            client->last_publish_ok = 0;
            if (!mx6u_mqtt_connect(client))
            {
                return 0;
            }
        }
    }

    topic_length = (int)strlen(client->topic);
    payload_length = (int)strlen(payload);
    packet[offset++] = MX6U_MQTT_PUBLISH_PACKET_TYPE;
    encoded_length = mx6u_mqtt_encode_remaining_length(packet + offset,
                                                       sizeof(packet) - (size_t)offset,
                                                       (size_t)(2 + topic_length + payload_length));
    if (encoded_length < 0)
    {
        client->last_publish_ok = 0;
        mx6u_mqtt_set_error(client, "publish packet too large");
        return 0;
    }
    offset += encoded_length;
    packet[offset++] = (unsigned char)((topic_length >> 8) & 0xFF);
    packet[offset++] = (unsigned char)(topic_length & 0xFF);
    memcpy(packet + offset, client->topic, (size_t)topic_length);
    offset += topic_length;
    memcpy(packet + offset, payload, (size_t)payload_length);
    offset += payload_length;

    if (!mx6u_mqtt_send_all((mx6u_socket_native_t)client->socket_fd, packet, (size_t)offset))
    {
        client->last_publish_ok = 0;
        mx6u_mqtt_disconnect(client);
        mx6u_mqtt_set_error(client, "publish failed");
        return 0;
    }

    client->connected = 1;
    client->last_publish_ok = 1;
    client->last_io_at = now;
    mx6u_mqtt_set_error(client, "publish ok");
    return 1;
}

void mx6u_mqtt_print_snapshot(const mx6u_mqtt_client_t *client, FILE *stream)
{
    if ((client == NULL) || (stream == NULL))
    {
        return;
    }

    fprintf(stream,
            "MQTT SNAPSHOT\nbroker=%s port=%d topic=%s connected=%s last_publish=%s reason=%s\n",
            client->broker,
            client->port,
            client->topic,
            client->connected ? "YES" : "NO",
            client->last_publish_ok ? "OK" : "FAIL",
            client->last_error[0] ? client->last_error : "none");
}
