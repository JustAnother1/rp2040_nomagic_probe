#include <stdio.h>
#include "rp2040_tests.h"
#include "mocks.h"
/*
#include "mocks.h"
#include "../src/probe_api/cli.h"
#include "../src/lib/printf.h"
#include "mock/serial_debug.h"

extern uint8_t send_buf[TST_SEND_BUFFER_SIZE];
*/

void* rp2040_setup(const MunitParameter params[], void* user_data)
{
    (void)params;
    (void)user_data;
    mocks_init();
    /*
    init_printf(NULL, serial_debug_putc);
    reset_send_receive_buffers();
    set_echo_enabled(false);
    cli_init(); */
    return NULL;
}

MunitResult test_swd_v2(const MunitParameter params[], void* user_data)
{
    // Objective: RP2040 is SWD v2 not v1
    (void) params;
    (void) user_data;
    munit_assert_true(target_is_SWDv2());
    return MUNIT_OK;
}

MunitResult test_get_core_id(const MunitParameter params[], void* user_data)
{
    // Objective: request the core IDs needed for SWDv2 communication
    (void) params;
    (void) user_data;
    munit_assert_uint32(0x01002927, ==, target_get_SWD_core_id(0));
    munit_assert_uint32(0x11002927, ==, target_get_SWD_core_id(1));
    munit_assert_uint32(0, ==, target_get_SWD_core_id(2));
    return MUNIT_OK;
}

MunitResult test_get_apsel(const MunitParameter params[], void* user_data)
{
    // Objective: read correct APSEl value needed for SWD communication
    (void) params;
    (void) user_data;
    munit_assert_uint32(0, ==, target_get_SWD_APSel(0));
    munit_assert_uint32(0, ==, target_get_SWD_APSel(1));
    munit_assert_uint32(0, ==, target_get_SWD_APSel(2));
    return MUNIT_OK;
}

MunitResult test_target_info(const MunitParameter params[], void* user_data)
{
    // Objective: make sure that cmd_target_info() finishes.
    (void) params;
    (void) user_data;
    uint32_t loop = 0;
    for(loop = 0; loop < 100; loop++)
    {
        if(true == cmd_target_info(loop))
        {
            break;
        }
    }
    munit_assert_uint32(99, >, loop);
    return MUNIT_OK;
}

MunitResult test_target_send_file_target_xml(const MunitParameter params[], void* user_data)
{
    // Objective: target.xml
    (void) params;
    (void) user_data;
    target_send_file("target.xml", 0, 0);
    munit_assert_true(0 == get_num_send_packets());
    return MUNIT_OK;
}

MunitResult test_target_send_file_threads(const MunitParameter params[], void* user_data)
{
    // Objective: target.xml
    (void) params;
    (void) user_data;
    target_send_file("threads", 0, 0);
    munit_assert_true(0 == get_num_send_packets());
    return MUNIT_OK;
}

MunitResult test_target_send_file_memory_map(const MunitParameter params[], void* user_data)
{
    // Objective: target.xml
    (void) params;
    (void) user_data;
    target_send_file("memory-map", 0, 0);
    munit_assert_true(0 == get_num_send_packets());
    return MUNIT_OK;
}

MunitResult test_target_send_file_invalid(const MunitParameter params[], void* user_data)
{
    // Objective: target.xml
    (void) params;
    (void) user_data;
    target_send_file("invalid file", 0, 0);
    munit_assert_true(1 == get_num_send_packets());
    return MUNIT_OK;
}
