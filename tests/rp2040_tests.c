#include <stdio.h>
#include "rp2040_tests.h"
/*
#include "mocks.h"
#include "../src/probe_api/cli.h"
#include "../src/lib/printf.h"
#include "mock/serial_debug.h"

extern uint8_t send_buf[TST_SEND_BUFFER_SIZE];
*/

void* rp2040_setup(const MunitParameter params[], void* user_data) {
    (void)params;
    (void)user_data;
    /*
    init_printf(NULL, serial_debug_putc);
    reset_send_receive_buffers();
    set_echo_enabled(false);
    cli_init(); */
    return NULL;
}

MunitResult test_swd_v2(const MunitParameter params[], void* user_data) {
    // Objective: RP2040 is SWD v2 not v1
    (void) params;
    (void) user_data;
    munit_assert_true(target_is_SWDv2());
    return MUNIT_OK;
}
/*
MunitResult test_cli_tick_idle(const MunitParameter params[], void* user_data) {
    // Objective: tick does nothing if no new bytes have arrived
    (void) params;
    (void) user_data;
    reset_send_receive_buffers();
    cli_tick();
    munit_assert_uint32(0, ==, get_num_bytes_in_send_buffer());
    munit_assert_uint32(0, ==, get_num_bytes_in_recv_buffer());
    return MUNIT_OK;
}
*/
