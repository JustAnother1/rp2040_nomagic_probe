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
#include "mock/lib/printf_mock.h"

void setUp(void)
{
    flash_driver_init();
    init_printf_mock();
}

void tearDown(void)
{

}

static void hex_dump(uint8_t* data, uint32_t length, const char* description)
{
    uint32_t i;

    for(i = 0; i < length; i++)
    {
        if(0 == i%16)
        {
            if(0 == i)
            {
                printf("%s :\r\n", description);
            }
            else
            {
                printf("\r\n");
            }
        }
        printf(" %02x", data[i]);
    }
    printf("\r\n\r\n");
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
    // Objective: data gets stored in internal buffer, but not tried to be written to flash
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
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(3, state.phase);
    res = flash_driver_write(&state, 0x10000000, 100, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);
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

    // state setup
    res = flash_driver_write(&state, 0x10000000, 256, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(3, state.phase);

    // buffer clear
    res = flash_driver_write(&state, 0x10000000, 256, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);

    // write page
    set_return_for_flash_write_page(ERR_NOT_COMPLETED);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000000, 256, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);
    TEST_ASSERT_EQUAL_HEX32(0x10000000, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[0], get_data_ptr_from_flash_write_page());

    set_return_for_flash_write_page(RESULT_OK);
    set_expect_first_call_for_flash_write_page(false);
    res = flash_driver_write(&state, 0x10000000, 256, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);

    set_return_for_flash_write_page(25);
    set_expect_first_call_for_flash_write_page(false);
    res = flash_driver_write(&state, 0x10000000, 256, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(RESULT_OK, res);
    TEST_ASSERT_EQUAL_HEX32(0x10000000, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[0], get_data_ptr_from_flash_write_page());
}

// Result flash_driver_write(flash_driver_data_typ* const state, uint32_t start_address, uint32_t length, uint8_t* data);
void test_flash_driver_write_512(void)
{
    // Objective: data gets written to flash
    uint8_t data[512];
    flash_driver_data_typ state;
    state.first_call = true;
    state.phase = 0;

    // flash erase
    Result res = flash_driver_write(&state, 0x10000000, 512, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_FALSE(state.first_call);
    TEST_ASSERT_EQUAL_UINT32(1, state.phase);

    // flash init
    set_expect_first_call_for_flash_initialize(true);
    set_return_for_flash_initialize(RESULT_OK);
    res = flash_driver_write(&state, 0x10000000, 512, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(2, state.phase);

    // state setup
    res = flash_driver_write(&state, 0x10000000, 512, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(3, state.phase);

    // buffer clear
    res = flash_driver_write(&state, 0x10000000, 512, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);

    // write first page
    set_return_for_flash_write_page(ERR_NOT_COMPLETED);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000000, 512, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);
    TEST_ASSERT_EQUAL_HEX32(0x10000000, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[0], get_data_ptr_from_flash_write_page());

    set_return_for_flash_write_page(RESULT_OK);
    set_expect_first_call_for_flash_write_page(false);
    res = flash_driver_write(&state, 0x10000000, 512, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);
    TEST_ASSERT_EQUAL_HEX32(0x10000000, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[0], get_data_ptr_from_flash_write_page());

    // write second page
    // write
    set_return_for_flash_write_page(ERR_NOT_COMPLETED);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000000, 512, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);
    TEST_ASSERT_EQUAL_HEX32(0x10000100, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[256], get_data_ptr_from_flash_write_page());

    set_return_for_flash_write_page(RESULT_OK);
    set_expect_first_call_for_flash_write_page(false);
    res = flash_driver_write(&state, 0x10000000, 512, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);
    TEST_ASSERT_EQUAL_HEX32(0x10000100, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[256], get_data_ptr_from_flash_write_page());

    set_return_for_flash_write_page(ERR_NOT_COMPLETED);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000000, 512, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(RESULT_OK, res);
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

    // state setup
    res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(3, state.phase);

    // buffer clear
    res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);

    // write first page
    set_return_for_flash_write_page(ERR_NOT_COMPLETED);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);
    TEST_ASSERT_EQUAL_HEX32(0x10000000, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[0], get_data_ptr_from_flash_write_page());

    set_return_for_flash_write_page(RESULT_OK);
    set_expect_first_call_for_flash_write_page(false);
    res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);
    TEST_ASSERT_EQUAL_HEX32(0x10000000, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[0], get_data_ptr_from_flash_write_page());

    // write second page
    set_return_for_flash_write_page(ERR_NOT_COMPLETED);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);
    TEST_ASSERT_EQUAL_HEX32(0x10000100, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[256], get_data_ptr_from_flash_write_page());

    set_return_for_flash_write_page(RESULT_OK);
    set_expect_first_call_for_flash_write_page(false);
    res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);
    TEST_ASSERT_EQUAL_HEX32(0x10000100, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    TEST_ASSERT_EQUAL_PTR(&data[256], get_data_ptr_from_flash_write_page());

    // detected that done
    set_return_for_flash_write_page(ERR_NOT_COMPLETED);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    TEST_ASSERT_EQUAL_INT32(RESULT_OK, res);
}


// Result flash_driver_write(flash_driver_data_typ* const state, uint32_t start_address, uint32_t length, uint8_t* data);
void test_flash_driver_write_two_sections(void)
{
    // Objective: data gets written to flash
    uint16_t buf[300];
    uint8_t* data = (uint8_t*)&buf[0];
    uint16_t i;
    for(i = 0; i < 300; i++)
    {
        buf[i] = i;
    }
    hex_dump(data, 600, "should");
    flash_driver_data_typ state;
    state.first_call = true;
    state.phase = 0;

    // flash erase
    Result res = flash_driver_write(&state, 0x10000000, 600, data);
    if(0 < printf_mock_get_write_idx())
    {
        char* msg = printf_mock_get_as_String();
        printf("%s\r\n", msg);
    }
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_FALSE(state.first_call);
    TEST_ASSERT_EQUAL_UINT32(1, state.phase);

    // flash init
    set_expect_first_call_for_flash_initialize(true);
    set_return_for_flash_initialize(RESULT_OK);
    res = flash_driver_write(&state, 0x10000000, 600, data);
    if(0 < printf_mock_get_write_idx())
    {
        printf("%s\r\n", printf_mock_get_as_String());
    }
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(2, state.phase);

    // state setup
    res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    if(0 < printf_mock_get_write_idx())
    {
        printf("%s\r\n", printf_mock_get_as_String());
    }
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(3, state.phase);

    // buffer clear
    res = flash_driver_write(&state, 0x10000000, 600, (uint8_t*)&data);
    if(0 < printf_mock_get_write_idx())
    {
        printf("%s\r\n", printf_mock_get_as_String());
    }
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);

    // write first page
    set_return_for_flash_write_page(ERR_NOT_COMPLETED);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000000, 600, data);
    if(0 < printf_mock_get_write_idx())
    {
        printf("%s\r\n", printf_mock_get_as_String());
    }
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);
    TEST_ASSERT_EQUAL_HEX32(0x10000000, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    // TEST_ASSERT_EQUAL_PTR(&data[0], get_data_ptr_from_flash_write_page());
    // TEST_ASSERT_EQUAL_UINT8_ARRAY(data, get_data_ptr_from_flash_write_page(), 256);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, get_copied_data_from_flash_write_page(), 256);

    set_return_for_flash_write_page(RESULT_OK);
    set_expect_first_call_for_flash_write_page(false);
    res = flash_driver_write(&state, 0x10000000, 600, data);
    if(0 < printf_mock_get_write_idx())
    {
        printf("%s\r\n", printf_mock_get_as_String());
    }
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);
    TEST_ASSERT_EQUAL_HEX32(0x10000000, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    hex_dump(get_copied_data_from_flash_write_page(), 256, "first section - first page");
    // TEST_ASSERT_EQUAL_PTR(&data[0], get_data_ptr_from_flash_write_page());
    // TEST_ASSERT_EQUAL_UINT8_ARRAY(data, get_data_ptr_from_flash_write_page(), 256);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, get_copied_data_from_flash_write_page(), 256);

    // write second page
    set_return_for_flash_write_page(RESULT_OK);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000000, 600, data);
    if(0 < printf_mock_get_write_idx())
    {
        printf("%s\r\n", printf_mock_get_as_String());
    }
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);
    TEST_ASSERT_EQUAL_HEX32(0x10000100, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    hex_dump(get_copied_data_from_flash_write_page(), 256, "first section - second page");
    // TEST_ASSERT_EQUAL_PTR(&data[256], get_data_ptr_from_flash_write_page());
    // TEST_ASSERT_EQUAL_UINT8_ARRAY(data + 256, get_data_ptr_from_flash_write_page(), 256);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data + 256, get_copied_data_from_flash_write_page(), 256);

    set_return_for_flash_write_page(1);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000000, 600, data);
    if(0 < printf_mock_get_write_idx())
    {
        printf("%s\r\n", printf_mock_get_as_String());
    }
    TEST_ASSERT_EQUAL_INT32(RESULT_OK, res);

    // second write
    state.first_call = true;
    state.phase = 0;

    // flash erase
    res = flash_driver_write(&state, 0x10000258, 300, data);
    if(0 < printf_mock_get_write_idx())
    {
        printf("%s\r\n", printf_mock_get_as_String());
    }
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_FALSE(state.first_call);
    TEST_ASSERT_EQUAL_UINT32(1, state.phase);

    // flash init
    set_expect_first_call_for_flash_initialize(true);
    set_return_for_flash_initialize(RESULT_OK);
    res = flash_driver_write(&state, 0x10000258, 300, data);
    if(0 < printf_mock_get_write_idx())
    {
        printf("%s\r\n", printf_mock_get_as_String());
    }
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(2, state.phase);

    // state setup
    set_return_for_flash_write_page(2);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000258, 300, (uint8_t*)&data);
    if(0 < printf_mock_get_write_idx())
    {
        printf("%s\r\n", printf_mock_get_as_String());
    }
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(3, state.phase);

    // state setup
    set_return_for_flash_write_page(ERR_NOT_COMPLETED);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000258, 300, (uint8_t*)&data);
    if(0 < printf_mock_get_write_idx())
    {
        printf("%s\r\n", printf_mock_get_as_String());
    }
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(3, state.phase);

    set_return_for_flash_write_page(RESULT_OK);
    set_expect_first_call_for_flash_write_page(false);
    res = flash_driver_write(&state, 0x10000258, 300, (uint8_t*)&data);
    if(0 < printf_mock_get_write_idx())
    {
        printf("%s\r\n", printf_mock_get_as_String());
    }
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);
    TEST_ASSERT_EQUAL_HEX32(0x10000200, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    // TEST_ASSERT_EQUAL_PTR(&data[0], get_data_ptr_from_flash_write_page());
    hex_dump(get_copied_data_from_flash_write_page(), 256, "second section - split page");
    // TEST_ASSERT_EQUAL_UINT8_ARRAY(data + 512, get_data_ptr_from_flash_write_page(), 88);
    // TEST_ASSERT_EQUAL_UINT8_ARRAY(data, get_data_ptr_from_flash_write_page() + 88, 168);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data + 512, get_copied_data_from_flash_write_page(), 88);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, get_copied_data_from_flash_write_page() + 88, 168);

    // write first page
    set_return_for_flash_write_page(ERR_NOT_COMPLETED);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000258, 300, data);
    if(0 < printf_mock_get_write_idx())
    {
        printf("%s\r\n", printf_mock_get_as_String());
    }
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);

    set_return_for_flash_write_page(RESULT_OK);
    set_expect_first_call_for_flash_write_page(false);
    res = flash_driver_write(&state, 0x10000258, 300, data);
    if(0 < printf_mock_get_write_idx())
    {
        printf("%s\r\n", printf_mock_get_as_String());
    }
    TEST_ASSERT_EQUAL_INT32(ERR_NOT_COMPLETED, res);
    TEST_ASSERT_EQUAL_UINT32(4, state.phase);
    TEST_ASSERT_EQUAL_HEX32(0x10000200, get_start_address_from_flash_write_page());
    TEST_ASSERT_EQUAL_INT32(256, get_length_from_flash_write_page());
    // TEST_ASSERT_EQUAL_PTR(&data[0], get_data_ptr_from_flash_write_page());
    hex_dump(get_copied_data_from_flash_write_page(), 256, "second section - first page");
    // TEST_ASSERT_EQUAL_UINT8_ARRAY(data + 512, get_data_ptr_from_flash_write_page(), 88);
    // TEST_ASSERT_EQUAL_UINT8_ARRAY(data, get_data_ptr_from_flash_write_page() + 88, 168);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data + 512, get_copied_data_from_flash_write_page(), 88);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, get_copied_data_from_flash_write_page() + 88, 168);

    // no write second page
    set_return_for_flash_write_page(ERR_NOT_COMPLETED);
    set_expect_first_call_for_flash_write_page(true);
    res = flash_driver_write(&state, 0x10000258, 300, data);
    if(0 < printf_mock_get_write_idx())
    {
        printf("%s\r\n", printf_mock_get_as_String());
    }
    TEST_ASSERT_EQUAL_INT32(RESULT_OK, res);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_flash_driver_write_NULL_NULL);
    RUN_TEST(test_flash_driver_write_NULL);
    RUN_TEST(test_flash_driver_write_flash_init);
    RUN_TEST(test_flash_driver_write_too_short);
    RUN_TEST(test_flash_driver_write_256);
    RUN_TEST(test_flash_driver_write_512);
    RUN_TEST(test_flash_driver_write_600);
    RUN_TEST(test_flash_driver_write_two_sections);
    return UNITY_END();
}
