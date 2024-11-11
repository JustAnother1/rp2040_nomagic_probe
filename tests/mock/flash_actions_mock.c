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
#include <stddef.h>
#include "flash_actions_mock.h"
#include "flash_actions.h"

Result res_flash_erase_32kb = 23;
bool flash_erase_32kb_expect_first_call = false;
uint32_t flash_erase_32kb_received_start_address = 1;

Result res_flash_erase_4kb = 23;
bool flash_erase_4kb_expect_first_call = false;
uint32_t flash_erase_4kb_received_start_address = 1;

Result res_flash_erase_64kb = 23;
bool flash_erase_64kb_expect_first_call = false;
uint32_t flash_erase_64kb_received_start_address = 1;

Result res_flash_initialize = 23;
bool flash_initialize_expect_first_call = false;

Result res_flash_write_page = 23;
bool flash_write_page_expect_first_call = false;
uint32_t flash_write_page_received_start_address = 1;
uint32_t flash_write_page_received_length = 2;
uint8_t* flash_write_page_received_data_ptr = NULL;

Result flash_erase_32kb(flash_action_data_typ* const state, uint32_t start_address)
{
    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }
    if(true == flash_erase_32kb_expect_first_call)
    {
        if(true == state->first_call)
        {
            // OK
            state->first_call = false;
        }
        else
        {
            // expected first call but was not !
            return 24;
        }
    }
    else
    {
        if(true == state->first_call)
        {
            // did not expected first call but was !
            return 24;
        }
        else
        {
            // OK
        }
    }
    flash_erase_32kb_received_start_address = start_address;
    return res_flash_erase_32kb;
}

void set_return_for_flash_erase_32kb(Result val)
{
    res_flash_erase_32kb = val;
}

void set_expect_first_call_for_flash_erase_32kb(bool val)
{
    flash_erase_32kb_expect_first_call = val;
}

uint32_t get_start_address_from_flash_erase_32kb(void)
{
    return flash_erase_32kb_received_start_address;
}

Result flash_erase_4kb(flash_action_data_typ* const state, uint32_t start_address)
{
    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }
    if(true == flash_erase_4kb_expect_first_call)
    {
        if(true == state->first_call)
        {
            // OK
            state->first_call = false;
        }
        else
        {
            // expected first call but was not !
            return 24;
        }
    }
    else
    {
        if(true == state->first_call)
        {
            // did not expected first call but was !
            return 24;
        }
        else
        {
            // OK
        }
    }
    flash_erase_4kb_received_start_address = start_address;
    return res_flash_erase_4kb;
}

void set_return_for_flash_erase_4kb(Result val)
{
    res_flash_erase_32kb = val;
}

void set_expect_first_call_for_flash_erase_4kb(bool val)
{
    flash_erase_32kb_expect_first_call = val;
}

uint32_t get_start_address_from_flash_erase_4kb(void)
{
    return flash_erase_32kb_received_start_address;
}


Result flash_erase_64kb(flash_action_data_typ* const state, uint32_t start_address)
{
    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }
    if(true == flash_erase_64kb_expect_first_call)
    {
        if(true == state->first_call)
        {
            // OK
            state->first_call = false;
        }
        else
        {
            // expected first call but was not !
            return 24;
        }
    }
    else
    {
        if(true == state->first_call)
        {
            // did not expected first call but was !
            return 24;
        }
        else
        {
            // OK
        }
    }
    flash_erase_64kb_received_start_address = start_address;
    return res_flash_erase_64kb;
}

void set_return_for_flash_erase_64kb(Result val)
{
    res_flash_erase_32kb = val;
}

void set_expect_first_call_for_flash_erase_64kb(bool val)
{
    flash_erase_32kb_expect_first_call = val;
}

uint32_t get_start_address_from_flash_erase_64kb(void)
{
    return flash_erase_32kb_received_start_address;
}


Result flash_write_page(flash_action_data_typ* const state, uint32_t start_address, uint8_t* data , uint32_t length)
{
    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }
    if(true == flash_write_page_expect_first_call)
    {
        if(true == state->first_call)
        {
            // OK
            state->first_call = false;
        }
        else
        {
            // expected first call but was not !
            return 24;
        }
    }
    else
    {
        if(true == state->first_call)
        {
            // did not expected first call but was !
            return 24;
        }
        else
        {
            // OK
        }
    }
    flash_write_page_received_start_address = start_address;
    flash_write_page_received_length = length;
    flash_write_page_received_data_ptr = data;
    return res_flash_write_page;
}

void set_return_for_flash_write_page(Result val)
{
    res_flash_write_page = val;
}

void set_expect_first_call_for_flash_write_page(bool val)
{
    flash_write_page_expect_first_call = val;
}

uint32_t get_start_address_from_flash_write_page(void)
{
    return flash_write_page_received_start_address;
}

uint32_t get_length_from_flash_write_page(void)
{
    return flash_write_page_received_length;
}

uint8_t* get_data_ptr_from_flash_write_page(void)
{
    return flash_write_page_received_data_ptr;
}

Result flash_initialize(flash_action_data_typ* const state)
{
    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }
    if(true == flash_initialize_expect_first_call)
    {
        if(true == state->first_call)
        {
            // OK
            state->first_call = false;
        }
        else
        {
            // expected first call but was not !
            return 24;
        }
    }
    else
    {
        if(true == state->first_call)
        {
            // did not expected first call but was !
            return 24;
        }
        else
        {
            // OK
        }
    }
    return res_flash_initialize;
}

void set_return_for_flash_initialize(Result val)
{
    res_flash_initialize = val;
}

void set_expect_first_call_for_flash_initialize(bool val)
{
    flash_initialize_expect_first_call = val;
}
