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
#include "flash_actions.h"
#include "target/execute.h"
#include "probe_api/debug_log.h"
#include "probe_api/result.h"

Result flash_initialize(flash_action_data_typ* const state)
{
    Result res;

    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }

    if(true == state->first_call)
    {
        debug_line("starting flash_initialize()");
        state->phase = 0;
        state->first_call = false;
    }

    if(0 == state->phase)
    {
        res = target_execute(BLINK); // do something here
        if(RESULT_OK == res)
        {
            // action->cur_phase++;
            return RESULT_OK;
        }
        else
        {
            return res;
        }
    }

    return ERR_WRONG_STATE;
}

Result flash_erase_32kb(flash_action_data_typ* const state, uint32_t start_address)
{
    Result res;
    (void)start_address;

    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }

    if(true == state->first_call)
    {
        debug_line("starting flash_erase_32kb()");
        state->phase = 0;
        state->first_call = false;
    }

    if(0 == state->phase)
    {
        res = RESULT_OK; // do something here
        if(RESULT_OK == res)
        {
            // action->cur_phase++;
            return RESULT_OK;
        }
        else
        {
            return res;
        }
    }

    return ERR_WRONG_STATE;
}

Result flash_erase_4kb(flash_action_data_typ* const state, uint32_t start_address)
{
    Result res;
    (void)start_address;

    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }

    if(true == state->first_call)
    {
        debug_line("starting flash_erase_4kb()");
        state->phase = 0;
        state->first_call = false;
    }

    if(0 == state->phase)
    {
        res = RESULT_OK; // do something here
        if(RESULT_OK == res)
        {
            // action->cur_phase++;
            return RESULT_OK;
        }
        else
        {
            return res;
        }
    }

    return ERR_WRONG_STATE;
}

Result flash_erase_64kb(flash_action_data_typ* const state, uint32_t start_address)
{
    Result res;
    (void)start_address;

    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }

    if(true == state->first_call)
    {
        debug_line("starting flash_erase_64kb()");
        state->phase = 0;
        state->first_call = false;
    }

    if(0 == state->phase)
    {
        res = RESULT_OK; // do something here
        if(RESULT_OK == res)
        {
            // action->cur_phase++;
            return RESULT_OK;
        }
        else
        {
            return res;
        }
    }

    return ERR_WRONG_STATE;
}

Result flash_write_page(flash_action_data_typ* const state, uint32_t start_address, uint8_t* data , uint32_t length)
{
    Result res;
    (void)start_address;
    (void) data;
    (void) length;

    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }

    if(true == state->first_call)
    {
        debug_line("starting flash_write_page()");
        state->phase = 0;
        state->first_call = false;
    }

    if(0 == state->phase)
    {
        res = RESULT_OK; // do something here
        if(RESULT_OK == res)
        {
            // action->cur_phase++;
            return RESULT_OK;
        }
        else
        {
            return res;
        }
    }

    return ERR_WRONG_STATE;
}

Result flash_enter_XIP(flash_action_data_typ* const state)
{
    Result res;

    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }

    if(true == state->first_call)
    {
        debug_line("starting flash_enter_xip()");
        state->phase = 0;
        state->first_call = false;
    }

    if(0 == state->phase)
    {
        res = RESULT_OK; // do something here
        if(RESULT_OK == res)
        {
            // action->cur_phase++;
            return RESULT_OK;
        }
        else
        {
            return res;
        }
    }

    return ERR_WRONG_STATE;
}
