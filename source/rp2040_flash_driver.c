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

static Result flash_erase_32kb(uint32_t start_address);
static Result flash_erase_4kb(uint32_t start_address);
static Result flash_erase_64kb(uint32_t start_address);
static Result flash_write_page(uint32_t start_address, uint8_t* data , uint32_t length);
static Result flash_initialize(void);


void flash_driver_init(void)
{
    flash_initialized = false;
    flash_erase_ongoing = false;
    flash_writing_ongoing = false;
}

Result flash_driver_add_erase_range(uint32_t start_address, uint32_t length)
{
    if(false == flash_erase_ongoing)
    {
        // first erase command
        if(false == flash_initialized)
        {
            Result res = flash_initialize();
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

        // The flash on the pico (Winbond W25Q16JV) can be erased in blocks of 64KB, 32KB or 4KB
        // TODO support other chips
        // TODO make the used chip a configuration setting
        erase_start_address = start_address;
        if(length > (64 * 1024))
        {
            // do block 64kB erase
            Result res = flash_erase_64kb(erase_start_address);
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
            }
        }
        else
        {
            // we wait for another erase command or vFlashDone or vFlashWrite before starting erase
            erase_done_up_to = start_address;
        }
        erase_end_address = start_address + length;
        if(erase_done_up_to < erase_end_address)
        {
            // there is still work to do.
            flash_erase_ongoing = true;
        }
    }
    else
    {
        // another erase command
        if(start_address == erase_end_address)
        {
            // continuation of the last erase command
            if(((erase_end_address + length) - erase_done_up_to) > (64 * 1024))
            {
                // do block 64kB erase
                Result res = flash_erase_64kb(erase_done_up_to);
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
                }
            }
            else
            {
                // we wait for another erase command or vFlashDone or vFlashWrite before starting erase
            }
            erase_end_address = erase_end_address + length;
        }
        else
        {
            // start of new erase in different area
            // 1. finish last erase
            Result res = flash_driver_erase_finish();
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
                // 2. start this erase
                erase_start_address = start_address;
                erase_end_address = start_address + length;
                if(length > (64 * 1024))
                {
                    // do block 64kB erase
                    res = flash_erase_64kb(erase_start_address);
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
            }
        }
    }
    return RESULT_OK;
}

Result flash_driver_write(action_data_typ* const action, uint32_t start_address, uint32_t length, uint8_t* data)
{
    Result res;
    if(true == flash_erase_ongoing)
    {
        // finish erasing the flash
        res = flash_driver_erase_finish();
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
    // we can at most write 256 Bytes in one go !
    if(false == flash_writing_ongoing)
    {
        // first write command
        write_start_address = start_address;
        while(length > 255)
        {
            res = flash_write_page(write_start_address, data, 256);
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
            length = length - 256;
            data = data + 256;
            write_start_address = write_start_address + 256;
        }
        if(0 < length)
        {
            // part of a page still in action
            bytes_in_buffer = length;
            memcpy(write_buffer, data, length);
            flash_writing_ongoing = true;
        }
        write_end_address = start_address + length;
    }
    else
    {
        // another write command
        if(start_address == write_end_address)
        {
            // we continue the last write
            if(0 < bytes_in_buffer)
            {
                if((length + bytes_in_buffer) > 255)
                {
                    if(256 > bytes_in_buffer)
                    {
                        action->intern[INTERN_ALREADY_WRITTEN_BYTES] = 256 - bytes_in_buffer;
                        memcpy(&write_buffer[bytes_in_buffer], data, 256 - bytes_in_buffer);
                        bytes_in_buffer = 256; // copy only once
                    }
                    res = flash_write_page(write_start_address, write_buffer, 256);
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
                    return ERR_NOT_COMPLETED;
                }
                else
                {
                    // not enough new bytes for a page write -> waiting for more
                    memcpy(&write_buffer[bytes_in_buffer], data, length);
                    bytes_in_buffer = bytes_in_buffer + length;
                    write_end_address = write_end_address + length;
                    return RESULT_OK;
                }
            }

            if((length - action->intern[INTERN_ALREADY_WRITTEN_BYTES]) > 255)
            {
                res = flash_write_page(write_start_address, &data[action->intern[INTERN_ALREADY_WRITTEN_BYTES]], 256);
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
                action->intern[INTERN_ALREADY_WRITTEN_BYTES] = action->intern[INTERN_ALREADY_WRITTEN_BYTES] + 256;
                return ERR_NOT_COMPLETED;
            }
            else if(0 < (length - action->intern[INTERN_ALREADY_WRITTEN_BYTES]))
            {
                // part of a page still in action
                bytes_in_buffer = length;
                memcpy(write_buffer, data, length);
                flash_writing_ongoing = true;
                return RESULT_OK;
            }
            else
            {
                // no more bytes
                flash_writing_ongoing = false;
                write_end_address = write_end_address + length;
                return RESULT_OK;
            }
        }
        else
        {
            // writing to another area
            // -> just finish the old one
            res = flash_write_page(write_start_address, write_buffer, bytes_in_buffer);
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
            write_start_address = start_address;
            return ERR_NOT_COMPLETED;
        }
    }
    return RESULT_OK;
}

Result flash_driver_erase_finish(void)
{
    if(true == flash_erase_ongoing)
    {
        // finish erasing the flash
        uint32_t length = erase_end_address - erase_done_up_to;
        if(length > (64 * 1024))
        {
            // do block 64kB erase
            Result res = flash_erase_64kb(erase_done_up_to);
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
                erase_done_up_to = erase_done_up_to + (64*1024);
            }
        }
        else if(length > (32 * 1024))
        {
            // do block 32kB erase
            Result res = flash_erase_32kb(erase_done_up_to);
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
                // erase done
                erase_done_up_to = erase_done_up_to + (32*1024);
            }
        }
        else if(length > 0)
        {
            // do block 4kB erase
            Result res = flash_erase_4kb(erase_done_up_to);
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
                // erase done
                erase_done_up_to = erase_done_up_to + (4*1024);
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
            return ERR_NOT_COMPLETED;
        }
    }
    else
    {
        // erase already finished
        return RESULT_OK;
    }
}

Result flash_driver_write_finish(void)
{
    if(true == flash_writing_ongoing)
    {
        // finish writing to flash
        Result res = flash_write_page(write_start_address, write_buffer, bytes_in_buffer);
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




static Result flash_initialize(void)
{
    // TODO erase sector of size 64KB
    return RESULT_OK;
}

static Result flash_erase_64kb(uint32_t start_address)
{
    // TODO erase sector of size 64KB
    (void)start_address;
    return RESULT_OK;
}

static Result flash_erase_32kb(uint32_t start_address)
{
    // TODO erase sector of size 32KB
    (void)start_address;
    return RESULT_OK;
}

static Result flash_erase_4kb(uint32_t start_address)
{
    // TODO erase sector of size 4KB
    (void)start_address;
    return RESULT_OK;
}

static Result flash_write_page(uint32_t start_address, uint8_t* data , uint32_t length)
{
    // TODO
    (void)start_address;
    (void)data;
    (void)length;
    return RESULT_OK;
}
