#include <stdio.h>

#include "co2_uart_protocol.h"

static int tests_failed = 0;

static void expect_true(int condition, const char *message)
{
  if (!condition)
  {
    printf("FAIL: %s\n", message);
    tests_failed++;
  }
}

static void test_parser_accepts_valid_frame(void)
{
  uint8_t frame[] = {0x2C, 0x01, 0x90, 0x03, 0xFF, 0xBF};
  Co2UartParser parser;
  uint16_t co2_ppm = 0U;
  uint8_t result = 0U;
  unsigned int i;

  Co2UartParser_Init(&parser);
  for (i = 0; i < sizeof(frame); ++i)
  {
    result = Co2UartParser_PushByte(&parser, frame[i], &co2_ppm);
  }

  expect_true(result == 1U, "valid CO2 frame should be accepted");
  expect_true(co2_ppm == 400U, "valid CO2 frame should decode ppm");
}

static void test_parser_rejects_invalid_checksum(void)
{
  uint8_t frame[] = {0x2C, 0x01, 0x90, 0x03, 0xFF, 0x00};
  Co2UartParser parser;
  uint16_t co2_ppm = 123U;
  uint8_t result = 0U;
  unsigned int i;

  Co2UartParser_Init(&parser);
  for (i = 0; i < sizeof(frame); ++i)
  {
    result = Co2UartParser_PushByte(&parser, frame[i], &co2_ppm);
  }

  expect_true(result == 0U, "bad checksum should be rejected");
  expect_true(co2_ppm == 123U, "bad checksum should not update ppm");
}

static void test_parser_skips_noise_before_frame(void)
{
  uint8_t stream[] = {0x00, 0x11, 0x2C, 0x01, 0x90, 0x03, 0xFF, 0xBF};
  Co2UartParser parser;
  uint16_t co2_ppm = 0U;
  uint8_t result = 0U;
  unsigned int i;

  Co2UartParser_Init(&parser);
  for (i = 0; i < sizeof(stream); ++i)
  {
    result = Co2UartParser_PushByte(&parser, stream[i], &co2_ppm);
  }

  expect_true(result == 1U, "parser should resync on frame header");
  expect_true(co2_ppm == 400U, "parser should decode frame after noise");
}

int main(void)
{
  test_parser_accepts_valid_frame();
  test_parser_rejects_invalid_checksum();
  test_parser_skips_noise_before_frame();

  if (tests_failed != 0)
  {
    return 1;
  }

  printf("PASS\n");
  return 0;
}
