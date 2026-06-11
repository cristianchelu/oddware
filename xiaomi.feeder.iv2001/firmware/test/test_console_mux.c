/* Tests: spec/30-processes/uart-console.md § Remote telnet console */

#include "unity.h"

#include "console_mux.h"

void test_console_mux_starts_local_idle(void)
{
    console_mux_test_reset();

    TEST_ASSERT_FALSE(console_mux_remote_active());
    TEST_ASSERT_TRUE(console_mux_try_remote());
    TEST_ASSERT_TRUE(console_mux_remote_active());
}

void test_console_mux_rejects_second_remote_claim(void)
{
    console_mux_test_reset();

    TEST_ASSERT_TRUE(console_mux_try_remote());
    TEST_ASSERT_FALSE(console_mux_try_remote());
    TEST_ASSERT_TRUE(console_mux_remote_active());
}

void test_console_mux_release_returns_to_local(void)
{
    console_mux_test_reset();

    TEST_ASSERT_TRUE(console_mux_try_remote());
    console_mux_release_remote();

    TEST_ASSERT_FALSE(console_mux_remote_active());
    TEST_ASSERT_TRUE(console_mux_try_remote());
}

void test_console_mux_force_local_request_and_take(void)
{
    console_mux_test_reset();

    TEST_ASSERT_TRUE(console_mux_try_remote());
    console_mux_request_force_local();
    TEST_ASSERT_TRUE(console_mux_take_force_local());
    TEST_ASSERT_FALSE(console_mux_take_force_local());
}

void test_console_mux_force_local_ignored_when_local(void)
{
    console_mux_test_reset();

    console_mux_request_force_local();
    TEST_ASSERT_FALSE(console_mux_force_local_pending());
    TEST_ASSERT_FALSE(console_mux_take_force_local());
}

void test_console_mux_force_local_pending_until_taken(void)
{
    console_mux_test_reset();

    TEST_ASSERT_TRUE(console_mux_try_remote());
    console_mux_request_force_local();

    TEST_ASSERT_TRUE(console_mux_force_local_pending());
    TEST_ASSERT_TRUE(console_mux_force_local_pending());
    TEST_ASSERT_TRUE(console_mux_take_force_local());
    TEST_ASSERT_FALSE(console_mux_force_local_pending());
}
