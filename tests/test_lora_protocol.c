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

static void test_build_environment_frame_formats_all_fields(void)
{
    char frame[128];
    SensorData data = {
        .temperature_tenths = 253,
        .humidity_tenths = 601,
        .soil_tenths = 412,
        .light_raw = 1234,
        .co2_ppm = 567,
    };

    int written = LoraProtocol_BuildEnvironmentFrame(frame, sizeof(frame), 1U, &data);

    expect_true(written > 0, "environment uplink frame should be written");
    expect_true(strcmp(frame, "ENV,N=1,T=25.3,H=60.1,SOIL=41.2,L=1234,CO2=567") == 0,
                "environment frame should contain node id and ordered sensor fields including co2");
}

static void test_build_status_frame_formats_all_fields(void)
{
    char frame[128];
    ActuatorStatus status = {
        .fan_on = 1U,
        .pump_on = 0U,
        .fill_light_on = 1U,
    };

    int written = LoraProtocol_BuildStatusFrame(frame, sizeof(frame), 2U, &status);

    expect_true(written > 0, "status uplink frame should be written");
    expect_true(strcmp(frame, "STAT,N=2,F=ON,P=OFF,LED=ON") == 0,
                "status frame should contain node id and actuator states");
}

static void test_build_frame_rejects_invalid_arguments(void)
{
    char frame[8];
    SensorData data = {0};
    ActuatorStatus status = {0};

    expect_true(LoraProtocol_BuildEnvironmentFrame(NULL, sizeof(frame), 1U, &data) < 0,
                "null environment buffer should be rejected");
    expect_true(LoraProtocol_BuildEnvironmentFrame(frame, sizeof(frame), 0U, &data) < 0,
                "environment frame should reject node id 0");
    expect_true(LoraProtocol_BuildEnvironmentFrame(frame, sizeof(frame), 1U, &data) < 0,
                "too-small environment buffer should be rejected");
    expect_true(LoraProtocol_BuildStatusFrame(NULL, sizeof(frame), 1U, &status) < 0,
                "null status buffer should be rejected");
    expect_true(LoraProtocol_BuildStatusFrame(frame, sizeof(frame), 1U, &status) < 0,
                "too-small status buffer should be rejected");
}

static void test_parse_downlink_command_accepts_start_speed(void)
{
    LoraMotorCommand command;
    int ok = LoraProtocol_ParseDownlinkCommand("CTRL,M=ON,S=80", &command);

    expect_true(ok == 1, "valid compatibility control command should parse");
    expect_true(command.enabled == 1, "parsed command should enable motor");
    expect_true(command.speed_percent == 80, "parsed command should keep speed");
}

int main(void)
{
    test_build_environment_frame_formats_all_fields();
    test_build_status_frame_formats_all_fields();
    test_build_frame_rejects_invalid_arguments();
    test_parse_downlink_command_accepts_start_speed();

    if (tests_failed != 0)
    {
        return 1;
    }

    printf("PASS\n");
    return 0;
}
