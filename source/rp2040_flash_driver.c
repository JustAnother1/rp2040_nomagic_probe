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
#include <string.h>
#include "flash_actions.h"
#include "probe_api/common.h"
#include "probe_api/debug_log.h"
#include "probe_api/result.h"
#include "rp2040_flash_driver.h"


static bool flash_initialized;
static bool flash_erase_ongoing;
static bool flash_writing_ongoing;
// the following are only valid if flash_erase_ongoing = true
static uint32_t erase_start_address;
static uint32_t erase_end_address;
static uint32_t erase_done_up_to;
// the following are only valid if flash_writing_ongoing = true
static uint32_t write_start_address;
static uint32_t write_end_address;
static uint8_t  write_buffer[256];
static uint32_t bytes_in_buffer;
static uint32_t already_written_bytes;

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

// length can be more than 256 bytes !
Result flash_driver_write(flash_driver_data_typ* const state, uint32_t start_address, uint32_t length, uint8_t* data)
{
    Result res;

    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }

    if(true == state->first_call)
    {
        cross_call_state.first_call = true;
        action_state.first_call = true;
        state->first_call = false;
        state->phase = 0;
        already_written_bytes = 0;
        debug_line("Flash driver write: @0x%08lx len %ld", start_address, length);
    }

    if(0 == state->phase)
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
        state->phase++;
        action_state.first_call = true;
        return ERR_NOT_COMPLETED;
    }

    // we can at most write 256 Bytes in one go !
    if(false == flash_writing_ongoing)
    {
        // first write command

        if(1 == state->phase)
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
            write_start_address = start_address;
            write_end_address = write_start_address + length;
            bytes_in_buffer = 0;
            state->phase++;
            action_state.first_call = true;
            return ERR_NOT_COMPLETED;
        }

        if(2 == state->phase)
        {
            uint32_t bytes_to_write = write_end_address - write_start_address;
            if(256 < bytes_to_write)
            {
                bytes_to_write = 256;
                // we can not write more than 256 Bytes in one go
            }
            if(bytes_to_write < 256)
            {
                // not enough data to write a complete block
                // -> copy those into the write_buffer
                memcpy(&write_buffer[bytes_in_buffer], &data[already_written_bytes], length);
                flash_writing_ongoing = true;
                bytes_in_buffer += length - already_written_bytes;
                return RESULT_OK;
            }
            res = flash_write_page(&action_state,
                                   write_start_address,
                                   &data[write_start_address - start_address - already_written_bytes],
                                   bytes_to_write);
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

            write_start_address += bytes_to_write;
            already_written_bytes += bytes_to_write;
            if(write_start_address + 256 < write_end_address)
            {
                // repeat this phase
                action_state.first_call = true;
                return ERR_NOT_COMPLETED;
            }
            else
            {
                if(write_start_address != write_end_address)
                {
                    // part of a page still in action
                    bytes_in_buffer = write_end_address - write_start_address;
                    memcpy(write_buffer,
                           &data[write_start_address - start_address - already_written_bytes],
                           bytes_in_buffer);
                    flash_writing_ongoing = true;
                }
                // done for now
                return RESULT_OK;
            }
        }

        return ERR_WRONG_STATE;
    }
    else
    {
        // another write command
        if(start_address == write_end_address)
        {
            if(1 == state->phase)
            {
                debug_line("INFO: another write command !");
                state->phase++;
                write_end_address = write_end_address + length;
            }
            // we continue the last write
            if(2 == state->phase)
            {
                if(0 < bytes_in_buffer)
                {
                    if((length + bytes_in_buffer) > 255)
                    {
                        if(256 > bytes_in_buffer)
                        {
                            // fill up write Buffer with the new data
                            already_written_bytes = 256 - bytes_in_buffer;
                            memcpy(&write_buffer[bytes_in_buffer], data, 256 - bytes_in_buffer);
                            bytes_in_buffer = 256; // copy only once
                        }
                        res = flash_write_page(&action_state, write_start_address, write_buffer, 256);
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
                        bytes_in_buffer = 0;
                        write_start_address = write_start_address + 256;
                        state->phase++;
                        action_state.first_call = true;
                        return ERR_NOT_COMPLETED;
                    }
                    else
                    {
                        // not enough new bytes for a page write -> waiting for more
                        memcpy(&write_buffer[bytes_in_buffer], data, length);
                        bytes_in_buffer = bytes_in_buffer + length;
                        return RESULT_OK;
                    }
                }
                state->phase++;
                action_state.first_call = true;
                return ERR_NOT_COMPLETED;
            }

            if(3 == state->phase)
            {
                if((length - already_written_bytes + bytes_in_buffer) > 255)
                {
                    // write a block
                    if(0 == bytes_in_buffer)
                    {
                        // direct copy
                        res = flash_write_page(&action_state,
                                               write_start_address,
                                               &write_buffer[already_written_bytes],
                                               256);
                    }
                    else
                    {
                        // fill up write buffer
                        memcpy(&write_buffer[bytes_in_buffer], &data[already_written_bytes], 256 - already_written_bytes);
                        // write from write_buffer
                        res = flash_write_page(&action_state,
                                               write_start_address,
                                               write_buffer,
                                               256);
                    }
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
                    action_state.first_call = true;
                    already_written_bytes += 256;
                    write_start_address += 256;
                    bytes_in_buffer = 0;
                    return ERR_NOT_COMPLETED;
                }
                else if(0 < (length - already_written_bytes))
                {
                    // part of a page still in action
                    // -> copy those into the write_buffer
                    memcpy(&write_buffer[bytes_in_buffer], &data[already_written_bytes], length);
                    flash_writing_ongoing = true;
                    bytes_in_buffer += length - already_written_bytes;
                    return RESULT_OK;
                }
                else
                {
                    // no more bytes
                    flash_writing_ongoing = false;
                    return RESULT_OK;
                }
            }

            return ERR_WRONG_STATE;
        }
        else
        {
            // writing to another area
            // -> just finish the old one
            if(1 == state->phase)
            {
                debug_line("INFO: writing to a new area !");
                // if(start_address == write_end_address)
                debug_line("start_address     : 0x%08lx", start_address);
                debug_line("write_end_address : 0x%08lx", write_end_address);
                state->phase++;
            }
            if(2 == state->phase)
            {
                if(bytes_in_buffer < 256)
                {
                    uint32_t i;
                    for(i = bytes_in_buffer; i < 256; i++)
                    {
                        write_buffer[i] = 0xff;
                    }
                    bytes_in_buffer = 256;
                }
                res = flash_write_page(&action_state, write_start_address, write_buffer, 256);
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
                bytes_in_buffer = 0;
                // next time this will be a "continue last write"
                write_end_address = start_address;
                write_start_address = start_address;
                action_state.first_call = true;
                return ERR_NOT_COMPLETED;
            }

            return ERR_WRONG_STATE;
        }
    }
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
        // finish writing to flash
        if(bytes_in_buffer < 256)
        {
            uint32_t i;
            for(i = bytes_in_buffer; i < 256; i++)
            {
                write_buffer[i] = 0xff;
            }
            bytes_in_buffer = 256;
        }
        Result res = flash_write_page(&action_state, write_start_address, write_buffer, bytes_in_buffer);
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
        bytes_in_buffer = 0;
        flash_writing_ongoing = false;
        // after a completed Write everything can happen
        // -> we might need to initialize the Flash again
        flash_initialized = false;
    }
    else
    {
        // write already finished
    }
    return RESULT_OK;
}
