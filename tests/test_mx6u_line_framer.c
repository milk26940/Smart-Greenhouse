#include <stdio.h>
#include <string.h>

#include "mx6u_line_framer.h"

static int tests_failed = 0;

static void expect_true(int condition, const char *message)
{
    if (!condition)
    {
        printf("FAIL: %s\n", message);
        tests_failed++;
    }
}

static void test_framer_waits_for_line_endings(void)
{
    mx6u_line_framer_t framer;
    char line[64];

    mx6u_line_framer_init(&framer);
    expect_true(mx6u_line_framer_push_byte(&framer, 'E', line, sizeof(line)) == 0,
                "single byte should not emit a frame");
    expect_true(mx6u_line_framer_push_byte(&framer, 'N', line, sizeof(line)) == 0,
                "partial frame should not emit");
}

static void test_framer_emits_line_on_newline(void)
{
    const char *frame = "ENV,N=1,T=25.3,H=60.1,SOIL=41.2,L=1234,CO2=567\r\n";
    mx6u_line_framer_t framer;
    char line[128];
    int emitted = 0;
    size_t i;

    mx6u_line_framer_init(&framer);
    for (i = 0; frame[i] != '\0'; ++i)
    {
        if (mx6u_line_framer_push_byte(&framer, (unsigned char)frame[i], line, sizeof(line)) == 1)
        {
            emitted = 1;
        }
    }

    expect_true(emitted == 1, "newline should emit one frame");
    expect_true(strcmp(line, "ENV,N=1,T=25.3,H=60.1,SOIL=41.2,L=1234,CO2=567") == 0,
                "line content should keep the full payload");
}

static void test_framer_ignores_empty_lines(void)
{
    mx6u_line_framer_t framer;
    char line[32];

    mx6u_line_framer_init(&framer);
    expect_true(mx6u_line_framer_push_byte(&framer, '\n', line, sizeof(line)) == 0,
                "empty newline should be ignored");
    expect_true(mx6u_line_framer_push_byte(&framer, '\r', line, sizeof(line)) == 0,
                "empty carriage return should be ignored");
}

static void test_framer_recovers_after_overflow(void)
{
    mx6u_line_framer_t framer;
    char line[32];
    int result = 0;
    int i;

    mx6u_line_framer_init(&framer);
    for (i = 0; i < 300; ++i)
    {
        result = mx6u_line_framer_push_byte(&framer, 'A', line, sizeof(line));
    }

    expect_true(result == -1, "overflow should be reported");
    expect_true(mx6u_line_framer_push_byte(&framer, '\n', line, sizeof(line)) == 0,
                "newline should reset overflowed state");
}

static void test_framer_flushes_pending_line_without_newline(void)
{
    const char *frame = "ENV,N=1,T=25.3,H=60.1,SOIL=35.0,L=1059,CO2=393";
    mx6u_line_framer_t framer;
    char line[128];
    size_t i;

    mx6u_line_framer_init(&framer);
    for (i = 0; frame[i] != '\0'; ++i)
    {
        expect_true(mx6u_line_framer_push_byte(&framer, (unsigned char)frame[i], line, sizeof(line)) == 0,
                    "bytes without newline should stay buffered");
    }

    expect_true(mx6u_line_framer_flush(&framer, line, sizeof(line)) == 1,
                "flush should emit pending line after idle timeout");
    expect_true(strcmp(line, frame) == 0,
                "flushed line should match buffered payload");
}

int main(void)
{
    test_framer_waits_for_line_endings();
    test_framer_emits_line_on_newline();
    test_framer_ignores_empty_lines();
    test_framer_recovers_after_overflow();
    test_framer_flushes_pending_line_without_newline();

    if (tests_failed != 0)
    {
        return 1;
    }

    printf("PASS\n");
    return 0;
}
