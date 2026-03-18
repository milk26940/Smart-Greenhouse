#include <stdio.h>
#include <string.h>

#include "lora_downlink_framer.h"

static int tests_failed = 0;

static void expect_true(int condition, const char *message)
{
  if (!condition)
  {
    printf("FAIL: %s\n", message);
    tests_failed++;
  }
}

static void test_parser_waits_for_line_ending(void)
{
  LoraDownlinkFramer framer;
  LoraMotorCommand command;

  LoraDownlinkFramer_Init(&framer);
  expect_true(LoraDownlinkFramer_PushByte(&framer, 'C', &command) == 0,
              "single byte should not finish a command");
  expect_true(LoraDownlinkFramer_PushByte(&framer, '\n', &command) == 0,
              "non-control line should be ignored");
}

static void test_parser_emits_valid_command_after_newline(void)
{
  const char *frame = "CTRL,M=ON,S=45\r\n";
  LoraDownlinkFramer framer;
  LoraMotorCommand command;
  int result = 0;
  int saw_command = 0;
  size_t i;

  LoraDownlinkFramer_Init(&framer);
  for (i = 0; frame[i] != '\0'; ++i)
  {
    result = LoraDownlinkFramer_PushByte(&framer, (uint8_t)frame[i], &command);
    if (result == 1)
    {
      saw_command = 1;
    }
  }

  expect_true(saw_command == 1, "valid control frame should produce a command");
  expect_true(command.enabled == 1, "parsed command should enable motor");
  expect_true(command.speed_percent == 45, "parsed command should keep speed");
}

static void test_parser_ignores_non_control_frames(void)
{
  const char *frame = "Power on\r\n";
  LoraDownlinkFramer framer;
  LoraMotorCommand command;
  int result = 0;
  int saw_command = 0;
  size_t i;

  LoraDownlinkFramer_Init(&framer);
  for (i = 0; frame[i] != '\0'; ++i)
  {
    result = LoraDownlinkFramer_PushByte(&framer, (uint8_t)frame[i], &command);
    if (result == 1)
    {
      saw_command = 1;
    }
  }

  expect_true(saw_command == 0, "non-control frame should be ignored");
  expect_true(result == 0 || result == -1, "non-control frame should not produce a command");
}

static void test_parser_resets_after_overflow(void)
{
  LoraDownlinkFramer framer;
  LoraMotorCommand command;
  int result = 0;
  int i;

  LoraDownlinkFramer_Init(&framer);
  for (i = 0; i < 80; ++i)
  {
    result = LoraDownlinkFramer_PushByte(&framer, 'A', &command);
  }

  expect_true(result == -1, "overflow should reject the frame");
  expect_true(LoraDownlinkFramer_PushByte(&framer, '\n', &command) == 0,
              "framer should recover after overflow reset");
}

int main(void)
{
  test_parser_waits_for_line_ending();
  test_parser_emits_valid_command_after_newline();
  test_parser_ignores_non_control_frames();
  test_parser_resets_after_overflow();

  if (tests_failed != 0)
  {
    return 1;
  }

  printf("PASS\n");
  return 0;
}
