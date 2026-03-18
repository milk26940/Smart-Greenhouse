#include <stdio.h>
#include <string.h>

#include "lora_protocol.h"

static int tests_failed = 0;

static void expect_true(int condition, const char *message)
{
    if (!condition)
    {
        printf("FAIL: %s\n", message);
        tests_failed++;
    }
}

static void test_build_uplink_frame_formats_all_fields(void)
{
    char frame[128];
    SensorData data = {
        .temperature_tenths = 253,
        .humidity_tenths = 601,
        .light_raw = 1234,
        .co2_ppm = 567,
    };
    MotorStatus motor = {
        .enabled = 1,
        .speed_percent = 35,
    };

    int written = LoraProtocol_BuildUplinkFrame(frame, sizeof(frame), &data, &motor);

    expect_true(written > 0, "uplink frame should be written");
    expect_true(strcmp(frame, "ENV,T=25.3,H=60.1,L=1234,CO2=567,M=ON,S=35") == 0,
                "uplink frame should contain all sensor and motor fields");
}

static void test_parse_downlink_command_accepts_start_speed(void)
{
    LoraMotorCommand command;
    int ok = LoraProtocol_ParseDownlinkCommand("CTRL,M=ON,S=80", &command);

    expect_true(ok == 1, "valid control command should parse");
    expect_true(command.enabled == 1, "parsed command should enable motor");
    expect_true(command.speed_percent == 80, "parsed command should keep speed");
}

static void test_parse_downlink_command_rejects_out_of_range_speed(void)
{
    LoraMotorCommand command;
    int ok = LoraProtocol_ParseDownlinkCommand("CTRL,M=ON,S=120", &command);

    expect_true(ok == 0, "speed over 100 should be rejected");
}

int main(void)
{
    test_build_uplink_frame_formats_all_fields();
    test_parse_downlink_command_accepts_start_speed();
    test_parse_downlink_command_rejects_out_of_range_speed();

    if (tests_failed != 0)
    {
        return 1;
    }

    printf("PASS\n");
    return 0;
}
