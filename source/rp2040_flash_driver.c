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
#include <stdbool.h>
#include <string.h>
#include "flash_actions.h"
#include "probe_api/common.h"
#include "probe_api/debug_log.h"
#include "probe_api/result.h"
#include "rp2040_flash_driver.h"
#include "flash_write_buffer.h"


static bool flash_initialized;
static bool flash_erase_ongoing;
static bool flash_writing_ongoing; // true as long as there are still writes that not have been written to flash
// the following are only valid if flash_erase_ongoing = true
static uint32_t erase_start_address;
static uint32_t erase_end_address;
static uint32_t erase_done_up_to;
// the following are only valid if flash_writing_ongoing = true
static uint32_t write_start_address; // lowest address of this long write (writes can be more then 256 Bytes long !)
static uint32_t write_end_address; // highest address of this long write
static uint32_t bytes_in_buffer; // number of bytes in buffer that still need to be written. (less than 256 Bytes waiting for more bytes to come)
static uint32_t already_written_bytes; // number of bytes that have been written in this long write
static uint32_t write_address_offset; // number of bytes from a 0x00 address from the start address (Start Address 0x10000000 -> 0; 0x10000005 -> 5,...)

static flash_action_data_typ action_state;
static flash_driver_data_typ cross_call_state;

void flash_driver_init(void)
{
    flash_initialized = false;
    flash_erase_ongoing = false;
    flash_writing_ongoing = false;
    action_state.first_call = true;
    erase_start_address = 0;
    erase_end_address = 0;
    erase_done_up_to = 0;
    write_start_address = 0;
    write_end_address = 0;
    bytes_in_buffer = 0;
    already_written_bytes = 0;
    write_address_offset = 0;
}

Result flash_driver_add_erase_range(flash_driver_data_typ* const state, uint32_t start_address, uint32_t length)
{
    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }

    if(true == state->first_call)
    {
        action_state.first_call = true;
        state->first_call = false;
        state->phase = 0;
        cross_call_state.first_call = true;
    }

    if(false == flash_erase_ongoing)
    {
        if(0 == state->phase)
        {
            // first erase command
            if(false == flash_initialized)
            {
                Result res = flash_initialize(&action_state);
                if(ERR_NOT_COMPLETED == res)
                {
                    // Try again next time
                    return res;
                }
                if(RESULT_OK != res)
                {
                    flash_erase_ongoing = false;
                    debug_line("ERROR: flash initialization failed(%ld) !", res);
                    return res;
                }
                // OK
                flash_initialized = true;
            }
            action_state.first_call = true;
            state->phase++;
            return ERR_NOT_COMPLETED;
        }

        if(1 == state->phase)
        {
            // The flash on the pico (Winbond W25Q16JV) can be erased in blocks of 64KB, 32KB or 4KB
            // TODO support other chips
            // TODO make the used chip a configuration setting
            erase_start_address = start_address;
            if(length > (64 * 1024))
            {
                // do block 64kB erase
                Result res = flash_erase_64kb(&action_state, erase_start_address);
                if(ERR_NOT_COMPLETED == res)
                {
                    // Try again next time
                    return res;
                }
                if(RESULT_OK != res)
                {
                    flash_erase_ongoing = false;
                    debug_line("ERROR: erase 64kB failed !");
                    return res;
                }
                else
                {
                    // erase done
                    erase_done_up_to = start_address + (64*1024);
                    target_restart_action_timeout();
                    action_state.first_call = true;
                }
            }
            else
            {
                // we wait for another erase command or vFlashDone or vFlashWrite before starting possible next erase
                erase_done_up_to = start_address;
            }
            erase_end_address = start_address + length;
            if(erase_done_up_to < erase_end_address)
            {
                // there is still work to do.
                flash_erase_ongoing = true;
            }
            // done for now
            return RESULT_OK;
        }
    }
    else
    {
        if(start_address == erase_end_address)
        {
            // another erase command

            if(0 == state->phase)
            {
                // continuation of the last erase command
                if(((erase_end_address + length) - erase_done_up_to) > (64 * 1024))
                {
                    // do block 64kB erase
                    Result res = flash_erase_64kb(&action_state, erase_done_up_to);
                    if(ERR_NOT_COMPLETED == res)
                    {
                        // Try again next time
                        return res;
                    }
                    if(RESULT_OK != res)
                    {
                        flash_erase_ongoing = false;
                        debug_line("ERROR: erase 64kB failed !");
                        return res;
                    }
                    else
                    {
                        erase_done_up_to = start_address + (64*1024);
                        target_restart_action_timeout();
                    }
                }
                else
                {
                    // we wait for another erase command or vFlashDone or vFlashWrite before starting erase
                }
                erase_end_address = erase_end_address + length;
                state->phase++;
                action_state.first_call = true;
                return ERR_NOT_COMPLETED;
            }

            return ERR_WRONG_STATE;
        }
        else
        {
            // start of new erase in different area

            // 1. finish last erase
            if(0 == state->phase)
            {
                Result res = flash_driver_erase_finish(&cross_call_state);
                if(ERR_NOT_COMPLETED == res)
                {
                    // Try again next time
                    return res;
                }
                if(RESULT_OK != res)
                {
                    flash_erase_ongoing = false;
                    debug_line("ERROR: finishing erase failed !");
                    return res;
                }
                else
                {
                    state->phase++;
                    action_state.first_call = true;
                    return ERR_NOT_COMPLETED;
                }
            }

            // 2. start this erase
            if(1 == state->phase)
            {
                erase_start_address = start_address;
                erase_end_address = start_address + length;
                if(length > (64 * 1024))
                {
                    // do block 64kB erase
                    Result res = flash_erase_64kb(&action_state, erase_start_address);
                    if(ERR_NOT_COMPLETED == res)
                    {
                        // Try again next time
                        return res;
                    }
                    if(RESULT_OK != res)
                    {
                        flash_erase_ongoing = false;
                        debug_line("ERROR: erase 64kB failed !");
                        return res;
                    }
                    else
                    {
                        erase_done_up_to = start_address + (64*1024);
                        target_restart_action_timeout();
                    }
                }
                else
                {
                    // we wait for another erase command or vFlashDone or vFlashWrite before starting erase
                    erase_done_up_to = start_address;
                }
                if(erase_done_up_to < erase_end_address)
                {
                    // there is still work to do.
                    flash_erase_ongoing = true;
                }
                // done for now
                return RESULT_OK;
            }
            return ERR_WRONG_STATE;
        }
    }
    return ERR_WRONG_STATE;
}


Result flash_driver_write(flash_driver_data_typ* const state)
{
    Result res;

    if(NULL == state)
    {
        debug_line("ERROR: flash write: state is NULL !");
        return ERR_ACTION_NULL;
    }

    if(true == state->first_call)
    {
        cross_call_state.first_call = true;
        action_state.first_call = true;
        state->first_call = false;
        state->phase = 0;
    }

    if(0 == state->phase) // make sure that erase operations have finished
    {
        if(true == flash_erase_ongoing)
        {
            // finish erasing the flash
            res = flash_driver_erase_finish(&cross_call_state);
            if(ERR_NOT_COMPLETED == res)
            {
                // Try again next time
                return res;
            }
            flash_erase_ongoing = false;
            if(RESULT_OK != res)
            {
                debug_line("ERROR: finishing erase failed !");
                return res;
            }
        }
        // else flash erase already finished
        state->phase++;
        action_state.first_call = true;
        return ERR_NOT_COMPLETED;
    }

    if(1 == state->phase) // make sure that the flash interface has been initialized
    {
        if(false == flash_initialized)
        {
            res = flash_initialize(&action_state);
            if(ERR_NOT_COMPLETED == res)
            {
                // Try again next time
                return res;
            }
            if(RESULT_OK != res)
            {
                flash_erase_ongoing = false;
                debug_line("ERROR: flash initialization failed !");
                return res;
            }
            // OK
            flash_initialized = true;
        }
        state->phase++;
        action_state.first_call = true;
        return ERR_NOT_COMPLETED;
    }

    if(2 == state->phase)
    {
        if(true == flash_write_buffer_has_data_block())
        {
            uint32_t address = flash_write_buffer_get_write_address();
            uint8_t* data = flash_write_buffer_get_data_block();
            res = flash_write_page(&action_state, address, data, 256);
            if(ERR_NOT_COMPLETED == res)
            {
                // Try again next time
                return res;
            }
            if(RESULT_OK != res)
            {
                flash_writing_ongoing = false;
                debug_line("ERROR: writing page failed !");
                return res;
            }
            // page was successfully written
            flash_write_buffer_remove_block();
            action_state.first_call = true;
            return ERR_NOT_COMPLETED;
        }
        else
        {
            uint32_t length = flash_write_buffer_get_length_available_no_waiting();
            if(1 > length)
            {
                // nothing to write anymore
                // -> we are done here
                return RESULT_OK;
            }
            else
            {
                // some bytes remaining
                uint32_t address = flash_write_buffer_get_write_address();
                uint8_t* data = flash_write_buffer_get_data_block();
                res = flash_write_page(&action_state, address, data, length);
                if(ERR_NOT_COMPLETED == res)
                {
                    // Try again next time
                    return res;
                }
                if(RESULT_OK != res)
                {
                    flash_writing_ongoing = false;
                    debug_line("ERROR: writing page failed !");
                    return res;
                }
                // page was successfully written
                flash_write_buffer_remove_block();
                action_state.first_call = true;
                return ERR_NOT_COMPLETED;
            }
        }
    }

    debug_line("ERROR: flash write: wrong state !");
    return ERR_WRONG_STATE;
}

Result flash_driver_erase_finish(flash_driver_data_typ* const state)
{
    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }

    if(true == state->first_call)
    {
        action_state.first_call = true;
        state->first_call = false;
        state->phase = 0;
    }

    if(false == flash_erase_ongoing)
    {
        // erase already finished
        return RESULT_OK;
    }

    // finish erasing the flash
    uint32_t length = erase_end_address - erase_done_up_to;
    if(length > (64 * 1024))
    {
        // do block 64kB erase
        Result res = flash_erase_64kb(&action_state, erase_done_up_to);
        if(ERR_NOT_COMPLETED == res)
        {
            // Try again next time
            return res;
        }
        if(RESULT_OK != res)
        {
            flash_erase_ongoing = false;
            debug_line("ERROR: erase 64kB failed !");
            return res;
        }
        else
        {
            // block erase done
            erase_done_up_to = erase_done_up_to + (64*1024);
            target_restart_action_timeout();
            action_state.first_call = true;
            return ERR_NOT_COMPLETED;
        }
    }
    else if(length > (32 * 1024))
    {
        // do block 32kB erase
        Result res = flash_erase_32kb(&action_state, erase_done_up_to);
        if(ERR_NOT_COMPLETED == res)
        {
            // Try again next time
            return res;
        }
        if(RESULT_OK != res)
        {
            flash_erase_ongoing = false;
            debug_line("ERROR: erase 32kB failed !");
            return res;
        }
        else
        {
            // block erase done
            erase_done_up_to = erase_done_up_to + (32*1024);
            target_restart_action_timeout();
            action_state.first_call = true;
            return ERR_NOT_COMPLETED;
        }
    }
    else if(length > 0)
    {
        // do block 4kB erase
        Result res = flash_erase_4kb(&action_state, erase_done_up_to);
        if(ERR_NOT_COMPLETED == res)
        {
            // Try again next time
            return res;
        }
        if(RESULT_OK != res)
        {
            flash_erase_ongoing = false;
            debug_line("ERROR: erase 32kB failed !");
            return res;
        }
        else
        {
            // block erase done
            erase_done_up_to = erase_done_up_to + (4*1024);
            target_restart_action_timeout();
            action_state.first_call = true;
            return ERR_NOT_COMPLETED;
        }
    }

    if(erase_done_up_to >= erase_end_address)
    {
        // with even one byte past the last sector we need to erase the following sector.
        // we can not delete less than 4 kB -> we might erase (4KB -1Byte) too much.
        flash_erase_ongoing = false;
        return RESULT_OK;
    }
    else
    {
        action_state.first_call = true;
        return ERR_NOT_COMPLETED;
    }
}

Result flash_driver_write_finish(flash_driver_data_typ* const state)
{
    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }

    if(true == state->first_call)
    {
        action_state.first_call = true;
        state->first_call = false;
        state->phase = 0;
        debug_line("Finishing write,...");
    }

    if(true == flash_writing_ongoing)
    {
        Result res;
        // finish writing to flash
        uint32_t length = flash_write_buffer_get_length_available_waiting();
        if(1 > length)
        {
            // nothing to write anymore
            // -> we are done here
            flash_writing_ongoing = false;
            // after a completed Write everything can happen
            // -> we might need to initialize the Flash again
            flash_initialized = false;
            return RESULT_OK;
        }
        else
        {
            // some bytes remaining
            uint32_t address = flash_write_buffer_get_write_address();
            uint8_t* data = flash_write_buffer_get_data_block();
            res = flash_write_page(&action_state, address, data, length);
            if(ERR_NOT_COMPLETED == res)
            {
                // Try again next time
                return res;
            }
            if(RESULT_OK != res)
            {
                flash_writing_ongoing = false;
                debug_line("ERROR: writing page failed !");
                return res;
            }
            // page was successfully written
            flash_write_buffer_remove_block();
            flash_writing_ongoing = false;
            // after a completed Write everything can happen
            // -> we might need to initialize the Flash again
            flash_initialized = false;
        }
    }
    else
    {
        // write already finished
    }
    return RESULT_OK;
}

Result flash_driver_enter_xip_mode(flash_driver_data_typ* const state)
{
    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }
    if(true == state->first_call)
    {
        action_state.first_call = true;
        state->first_call = false;
        state->phase = 0;
        debug_line("switching to XIP,...");
    }

    Result res = flash_enter_XIP(&action_state);
    if(ERR_NOT_COMPLETED == res)
    {
        // Try again next time
        return res;
    }
    if(RESULT_OK != res)
    {
        debug_line("ERROR: switching to XiP mode failed (%ld in %ld)!", res, action_state.phase);
        return res;
    }

    return RESULT_OK;
}
