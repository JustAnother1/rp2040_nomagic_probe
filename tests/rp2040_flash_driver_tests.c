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
#include "probe_api/result.h"
#include "rp2040_flash_driver.h"
#include "mock/flash_actions_mock.h"

void setUp(void)
{
    flash_driver_init();
}

void tearDown(void)
{

}

// Result flash_driver_write(flash_driver_data_typ* const state, uint32_t start_address, uint32_t length, uint8_t* data);
void test_flash_driver_write_NULL_NULL(void)
{
    // Objective: input parameter check
    Result res = flash_driver_write(NULL, 0, 0, NULL);

    TEST_ASSERT_EQUAL_INT32(ERR_ACTION_NULL, res);
}

// Result flash_driver_write(flash_driver_data_typ* const state, uint32_t start_address, uint32_t length, uint8_t* data);
void test_flash_driver_write_NULL(void)
{
    // Objective: input parameter check

    flash_driver_data_typ state;
    state.first_call = true;
    state.phase = 0;

    Result res = flash_driver_write(&state, 0, 0, NULL);
    TEST_ASSERT_EQUAL_INT32(ERR_INVALID_PARAMETER, res);

    res = flash_driver_write(&state, 0x10000000, 0, NULL);
    TEST_ASSERT_EQUAL_INT32(ERR_INVALID_PARAMETER, res);

    res = flash_driver_write(&state, 0x10000000, 256, NULL);
    TEST_ASSERT_EQUAL_INT32(ERR_INVALID_PARAMETER, res);

    res = flash_driver_write(&state, 0x10000000, 0, (uint8_t*)&state);
    TEST_ASSERT_EQUAL_INT32(ERR_INVALID_PARAMETER, res);
}

// Result flash_driver_write(flash_driver_data_typ* const state, uint32_t start_address, uint32_t length, uint8_t* data);
void test_flash_driver_write_flash_init(void)
{
    // Objective: flash initialization gets called and has error
    uint8_t data[100];
    flash_driver_data_typ state;
    state.first_call = true;
    state.phase = 0;

    Result res = flash_driver_write(&state, 0x10000000, 100, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_FALSE(state.first_call);
    TEST_ASSERT_EQUAL_UINT32(1, state.phase);
    set_expect_first_call_for_flash_initialize(true);
    set_return_for_flash_initialize(42);
    res = flash_driver_write(&state, 0x10000000, 100, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(42, res);
}

// Result flash_driver_write(flash_driver_data_typ* const state, uint32_t start_address, uint32_t length, uint8_t* data);
void test_flash_driver_write_too_short(void)
{
    // Objective: data gets stored, but not send
    uint8_t data[100];
    flash_driver_data_typ state;
    state.first_call = true;
    state.phase = 0;

    Result res = flash_driver_write(&state, 0x10000000, 100, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_FALSE(state.first_call);
    TEST_ASSERT_EQUAL_UINT32(1, state.phase);
    set_expect_first_call_for_flash_initialize(true);
    set_return_for_flash_initialize(RESULT_OK);
    res = flash_driver_write(&state, 0x10000000, 100, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(2, state.phase);
    res = flash_driver_write(&state, 0x10000000, 100, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(RESULT_OK, res);
}

// Result flash_driver_write(flash_driver_data_typ* const state, uint32_t start_address, uint32_t length, uint8_t* data);
void test_flash_driver_write_256(void)
{
    // Objective: data gets written to flash
    uint8_t data[256];
    flash_driver_data_typ state;
    state.first_call = true;
    state.phase = 0;

    // flash erase
    Result res = flash_driver_write(&state, 0x10000000, 256, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_FALSE(state.first_call);
    TEST_ASSERT_EQUAL_UINT32(1, state.phase);

    // flash init
    set_expect_first_call_for_flash_initialize(true);
    set_return_for_flash_initialize(RESULT_OK);
    res = flash_driver_write(&state, 0x10000000, 256, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(2, state.phase);

    // write page
    set_return_for_flash_write_page(ERR_NOT_COMPLETED);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000000, 256, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_HEX32(0x10000000, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[0], get_data_ptr_from_flash_write_page());

    set_return_for_flash_write_page(RESULT_OK);
    set_expect_first_call_for_flash_write_page(false);
    res = flash_driver_write(&state, 0x10000000, 256, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(RESULT_OK, res);
    TEST_ASSERT_EQUAL_HEX32(0x10000000, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[0], get_data_ptr_from_flash_write_page());
}

// Result flash_driver_write(flash_driver_data_typ* const state, uint32_t start_address, uint32_t length, uint8_t* data);
void test_flash_driver_write_600(void)
{
    // Objective: data gets written to flash
    uint8_t data[600];
    flash_driver_data_typ state;
    state.first_call = true;
    state.phase = 0;

    // flash erase
    Result res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_FALSE(state.first_call);
    TEST_ASSERT_EQUAL_UINT32(1, state.phase);

    // flash init
    set_expect_first_call_for_flash_initialize(true);
    set_return_for_flash_initialize(RESULT_OK);
    res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(2, state.phase);

    // write first page
    set_return_for_flash_write_page(ERR_NOT_COMPLETED);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_HEX32(0x10000000, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[0], get_data_ptr_from_flash_write_page());

    set_return_for_flash_write_page(RESULT_OK);
    set_expect_first_call_for_flash_write_page(false);
    res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_HEX32(0x10000000, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[0], get_data_ptr_from_flash_write_page());

    // write second page
    // check if bytes in buffer
    set_return_for_flash_write_page(1);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);

    // write
    set_return_for_flash_write_page(RESULT_OK);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_HEX32(0x10000100, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[256], get_data_ptr_from_flash_write_page());

    set_return_for_flash_write_page(RESULT_OK);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(RESULT_OK, res);
    TEST_ASSERT_EQUAL_HEX32(0x10000100, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[256], get_data_ptr_from_flash_write_page());

}


int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_flash_driver_write_NULL_NULL);
    RUN_TEST(test_flash_driver_write_NULL);
    RUN_TEST(test_flash_driver_write_flash_init);
    RUN_TEST(test_flash_driver_write_too_short);
    RUN_TEST(test_flash_driver_write_256);
    RUN_TEST(test_flash_driver_write_600);
    return UNITY_END();
}
