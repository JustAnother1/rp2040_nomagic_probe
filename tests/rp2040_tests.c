/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#include <stdbool.h>
#include "unity.h"

void setUp(void)
{

}

void tearDown(void)
{

}

void test_swd_v2(void)
{
    // Objective: RP2040 is SWD v2 not v1
    TEST_ASSERT_TRUE(target_is_SWDv2());
}


void test_get_core_id(void)
{
    // Objective: request the core IDs needed for SWDv2 communication
    TEST_ASSERT_EQUAL_HEX32(0x01002927, target_get_SWD_core_id(0));
    TEST_ASSERT_EQUAL_HEX32(0x11002927, target_get_SWD_core_id(1));
    TEST_ASSERT_EQUAL_UINT32(0, target_get_SWD_core_id(2));
}

void test_get_apsel(void)
{
    // Objective: read correct APSEl value needed for SWD communication
    TEST_ASSERT_EQUAL_UINT32(0, target_get_SWD_APSel(0));
    TEST_ASSERT_EQUAL_UINT32(0, target_get_SWD_APSel(1));
    TEST_ASSERT_EQUAL_UINT32(0, target_get_SWD_APSel(2));
}

void test_target_info(void)
{
    // Objective: make sure that cmd_target_info() finishes.
    uint32_t loop = 0;
    for(loop = 0; loop < 100; loop++)
    {
        if(true == cmd_target_info(loop))
        {
            break;
        }
    }
    TEST_ASSERT_TRUE(99 > loop);
}

void test_target_send_file_target_xml(void)
{
    // Objective: target.xml
    target_send_file("target.xml", 0, 0);
    TEST_ASSERT_EQUAL_INT(0, mock_gdbserver_get_num_send_replies());
}

void test_target_send_file_threads(void)
{
    // Objective: target.xml
    target_send_file("threads", 0, 0);
    TEST_ASSERT_EQUAL_INT(0, mock_gdbserver_get_num_send_replies());
}

void test_target_send_file_memory_map(void)
{
    // Objective: target.xml
    target_send_file("memory-map", 0, 0);
    TEST_ASSERT_EQUAL_INT(0, mock_gdbserver_get_num_send_replies());
}

void test_target_send_file_invalid(void)
{
    // Objective: target.xml
    target_send_file("invalid file", 0, 0);
    TEST_ASSERT_EQUAL_INT(1, mock_gdbserver_get_num_send_replies());
}


int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_swd_v2);
    RUN_TEST(test_get_core_id);
    RUN_TEST(test_get_apsel);
    RUN_TEST(test_target_info);
    RUN_TEST(test_target_send_file_target_xml);
    RUN_TEST(test_target_send_file_threads);
    RUN_TEST(test_target_send_file_memory_map);
    RUN_TEST(test_target_send_file_invalid);
    return UNITY_END();
}
