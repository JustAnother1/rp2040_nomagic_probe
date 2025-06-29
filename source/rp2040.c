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

#include "probe_api/common.h"
#include "probe_api/cortex-m.h"
#include "probe_api/cortex-m_actions.h"
#include "probe_api/debug_log.h"
#include "probe_api/flash_write_buffer.h"
#include "probe_api/gdb_error_codes.h"
#include "probe_api/gdb_monitor_commands.h"
#include "probe_api/gdb_packets.h"
#include "probe_api/hex.h"
#include "probe_api/result.h"
#include "probe_api/steps.h"
#include "probe_api/swd.h"
#include "probe_api/util.h"
#include "rp2040_flash_driver.h"
#include "target.h"
#ifdef FEAT_EXECUTE_CODE_ON_TARGET
#include "target/execute.h"
#endif

// RP2040:
// Core 0: 0x01002927
// Core 1: 0x11002927
// Rescue DP: 0xf1002927
// Core0 and Core1 instance IDs can be changed
// so to be sure check also these end points:
// 0x21002927, 0x31002927, 0x41002927, 0x51002927, 0x61002927, 0x71002927
// 0x81002927, 0x91002927, 0xa1002927, 0xb1002927, 0xc1002927, 0xd1002927,
// 0xe1002927

// decoded:
// IDCODE:
// bit 0     = 1;
// bit 1-11  = Designer (JEP106)
// bit 27-12 = PartNo
// bit 28-31 = Version (implementation defined)

// Part Number : 0x1002
// Designer = Raspberry Pi Trading Ltd.
// JEP 106 = 9x 0x7f then 0x13


#define SWD_ID_CORE_0    0x01002927
#define SWD_ID_CORE_1    0x11002927
#define SWD_ID_RESCUE_DP 0xf1002927

#define SWD_AP_SEL       0

#define MEMORY_MAP_CONTENT  \
"<memory-map>\r\n" \
    "<memory type=\"rom\" start=\"0x00000000\" length=\"0x00004000\"/>\r\n" \
    "<memory type=\"flash\" start=\"0x10000000\" length=\"0x1000000\">\r\n" \
        "<property name=\"blocksize\">0x1000</property>\r\n" \
    "</memory>\r\n" \
    "<memory type=\"ram\" start=\"0x20000000\" length=\"0x20042000\"/>\r\n" \
"</memory-map>\r\n"


static flash_driver_data_typ flash_driver_state;


void target_init(void)
{
    flash_write_buffer_init(256); // flash page size = 256 Bytes
    flash_driver_init();
#ifdef FEAT_EXECUTE_CODE_ON_TARGET
    target_execute_init();
#endif
    common_target_init();
}

void target_re_init(void)
{
    flash_write_buffer_clear();
    flash_driver_init();
}

void target_tick(void)
{
    common_target_tick();
}

bool target_is_SWDv2(void)
{
    return true;
}

uint32_t target_get_SWD_core_id(uint32_t core_num) // only required for SWDv2 (TARGETSEL)
{
    switch(core_num)
    {
    case 0: return SWD_ID_CORE_0;
    case 1: return SWD_ID_CORE_1;
    }
    return 0;
}

uint32_t target_get_SWD_APSel(uint32_t core_num)
{
    (void) core_num;
    return SWD_AP_SEL;
}

bool cmd_target_info(uint32_t loop)
{
    if(0 == loop)
    {
        cli_line("Target Status");
        cli_line("=============");
        cli_line("target: RP2040");
    }
    else
    {
        return common_cmd_target_info(loop -1);
    }
    return false; // true == Done; false = call me again
}

void target_send_file(char* filename, uint32_t offset, uint32_t len)
{
    if(0 == strncmp(filename, "target.xml", 10))
    {
        send_part(TARGET_XML_CONTENT, sizeof(TARGET_XML_CONTENT), offset, len);
        return;
    }
    else if(0 == strncmp(filename, "threads", 7))
    {
        send_part(THREADS_CONTENT, sizeof(THREADS_CONTENT), offset, len);  // TODO fix for dual core
        return;
    }
    else if(0 == strncmp(filename, "memory-map", 10))
    {
        send_part(MEMORY_MAP_CONTENT, sizeof(MEMORY_MAP_CONTENT), offset, len);
        return;
    }

    debug_error("xfer:file invalid");
    // if we reach this, then the request was invalid
    reply_packet_prepare();
    reply_packet_add("E00");
    reply_packet_send();
}

// GDB_CMD_VFLASH_DONE
Result handle_target_reply_vFlashDone(action_data_typ* const action)
{
    Result res;

    if(NULL == action)
    {
        return ERR_ACTION_NULL;
    }

    if(true == action->first_call)
    {
        debug_line("Flash Done!");
        action->first_call = false;
        action->cur_phase = 0;
        flash_driver_state.first_call = true;
    }

    if(0 == action->cur_phase)
    {
        // finish erasing the flash
        res = flash_driver_erase_finish(&flash_driver_state);
        if(ERR_NOT_COMPLETED == res)
        {
            // Try again next time
            return res;
        }
        if(RESULT_OK != res)
        {
            debug_error("ERROR: finishing erase failed !");
            reply_packet_prepare();
            reply_packet_add(ERROR_TARGET_FAILED);
            reply_packet_send();
            return res;
        }
        action->cur_phase++;
        flash_driver_state.first_call = true;
        return ERR_NOT_COMPLETED;
    }

    if(1 == action->cur_phase)
    {
        // finish writing to flash
        res = flash_driver_write_finish(&flash_driver_state);
        if(ERR_NOT_COMPLETED == res)
        {
            // Try again next time
            return res;
        }
        if(RESULT_OK != res)
        {
            debug_error("ERROR: finishing write failed !");
            reply_packet_prepare();
            reply_packet_add(ERROR_TARGET_FAILED);
            reply_packet_send();
            return res;
        }

        action->cur_phase++;
        flash_driver_state.first_call = true;
        return ERR_NOT_COMPLETED;
    }

    if(2 == action->cur_phase)
    {
        // switch to XIP Mode
        res = flash_driver_enter_xip_mode(&flash_driver_state);
        if(ERR_NOT_COMPLETED == res)
        {
            // Try again next time
            return res;
        }
        if(RESULT_OK != res)
        {
            debug_error("ERROR: enter XiP mode failed !");
            reply_packet_prepare();
            reply_packet_add(ERROR_TARGET_FAILED);
            reply_packet_send();
            return res;
        }
        // erase + write has finished now -> send OK
        reply_packet_prepare();
        reply_packet_add("OK");
        reply_packet_send();
        return RESULT_OK;
    }

    return ERR_WRONG_STATE;
}

// GDB_CMD_VFLASH_ERASE
Result handle_target_reply_vFlashErase(action_data_typ* const action)
{
    Result res;
    uint32_t start_address = action->gdb_parameter.address_length.address;
    uint32_t length = action->gdb_parameter.address_length.length;

    if(NULL == action)
    {
        return ERR_ACTION_NULL;
    }

    if(ADDRESS_LENGTH != action->gdb_parameter.type)
    {
        // wrong parameter type
        debug_error("ERROR: wrong parameter type !");
        reply_packet_prepare();
        reply_packet_add(ERROR_CODE_INVALID_PARAMETER_FORMAT_TYPE);
        reply_packet_send();
        return ERR_WRONG_VALUE;
    }

    if(true == action->first_call)
    {
        debug_line("Flash erase: address : 0x%08lx, length: 0x%08lx", start_address, length);
        action->first_call = false;
        flash_driver_state.first_call = true;
    }

    res = flash_driver_add_erase_range(&flash_driver_state, start_address, length);
    if(ERR_NOT_COMPLETED == res)
    {
        // Try again next time
        return res;
    }
    if(RESULT_OK != res)
    {
        debug_error("ERROR: adding erase range failed !");
        reply_packet_prepare();
        reply_packet_add(ERROR_TARGET_FAILED);
        reply_packet_send();
        return res;
    }

    // driver finished with RESULT_OK -> send OK
    reply_packet_prepare();
    reply_packet_add("OK");
    reply_packet_send();
    return RESULT_OK;
}

// GDB_CMD_VFLASH_WRITE
Result handle_target_reply_vFlashWrite(action_data_typ* const action)
{
    Result res;
    uint32_t start_address = action->gdb_parameter.address_binary.address;
    uint32_t length = action->gdb_parameter.address_binary.data_length; // length can be up to MAX_BINARY_SIZE_BYTES
    uint8_t* data = (uint8_t*)action->gdb_parameter.address_binary.data;

    if(NULL == action)
    {
        return ERR_ACTION_NULL;
    }

    if(ADDRESS_MEMORY != action->gdb_parameter.type)
    {
        // wrong parameter type
        debug_error("ERROR: wrong parameter type !");
        reply_packet_prepare();
        reply_packet_add(ERROR_CODE_INVALID_PARAMETER_FORMAT_TYPE);
        reply_packet_send();
        return ERR_WRONG_VALUE;
    }

    if(true == action->first_call)
    {
        debug_line("Flash write: address : 0x%08lx", start_address);
        debug_line("Flash write: length : %ld", length);
        action->intern[INTERN_ALREADY_WRITTEN_BYTES] = 0;
        action->first_call = false;
        flash_driver_state.first_call = true;
    }

    res = flash_write_buffer_add_data(start_address, length, data);
    if(RESULT_OK != res)
    {
        debug_error("ERROR: flash write buffer issue ! (%ld)", res);
        reply_packet_prepare();
        reply_packet_add(ERROR_TARGET_FAILED);
        reply_packet_send();
        return res;
    }
    res = flash_driver_write(&flash_driver_state);
    if(ERR_NOT_COMPLETED == res)
    {
        // Try again next time
        return res;
    }
    if(RESULT_OK != res)
    {
        debug_error("ERROR: flash write failed !");
        reply_packet_prepare();
        reply_packet_add(ERROR_TARGET_FAILED);
        reply_packet_send();
        return res;
    }

    if(true == flash_write_buffer_has_data_block())
    {
        // the buffer still has enough bytes for an additional write
        // -> try again
        return ERR_NOT_COMPLETED;
    }

    // driver finished with RESULT_OK -> send OK
    reply_packet_prepare();
    reply_packet_add("OK");
    reply_packet_send();
    return RESULT_OK;
}

// GDB_CMD_READ_MEMORY
Result handle_target_reply_read_memory(action_data_typ* const action)
{
    // ‘m addr,length’
    //     Read length addressable memory units starting at address addr
    // (see addressable memory unit). Note that addr may not be aligned to any
    // particular boundary.
    //     The stub need not use any particular size or alignment when gathering
    // data from memory for the response; even if addr is word-aligned and
    // length is a multiple of the word size, the stub is free to use byte
    // accesses, or not. For this reason, this packet may not be suitable for
    // accessing memory-mapped I/O devices.
    //     Reply:
    //     ‘XX…’
    //         Memory contents; each byte is transmitted as a two-digit
    // hexadecimal number. The reply may contain fewer addressable memory units
    // than requested if the server was able to read only part of the region of
    // memory.
    //     ‘E NN’
    //         NN is errno

#define INTERN_MEMORY_OFFSET     1

    Result res;

    if(NULL == action)
    {
        return ERR_ACTION_NULL;
    }

    if(true == action->first_call)
    {
        reply_packet_prepare();
        if(ADDRESS_LENGTH != action->gdb_parameter.type)
        {
            // wrong parameter type
            debug_error("ERROR: wrong parameter type !");
            reply_packet_add(ERROR_CODE_INVALID_PARAMETER_FORMAT_TYPE);
            reply_packet_send();
            return ERR_WRONG_VALUE;
        }
        else
        {
            action->cur_phase = 0;
            action->intern[INTERN_MEMORY_OFFSET] = 0;
        }
        action->first_call = false;
    }

    if(0 == action->cur_phase)
    {
        res = step_read_ap((uint32_t *)(action->gdb_parameter.address_length.address + action->intern[INTERN_MEMORY_OFFSET]));
        if(RESULT_OK == res)
        {
            action->cur_phase++;
        }
        else
        {
            return res;
        }
    }

    if(1 == action->cur_phase)
    {
        res = step_get_Result_data(&action->read_0);
        if(RESULT_OK == res)
        {
            action->cur_phase++;
        }
        else
        {
            return res;
        }
    }

    if(2 == action->cur_phase)
    {
        char buf[9];
        int_to_hex(buf, action->read_0, 8);
        buf[8] = 0;
        reply_packet_add(buf);
        // debug_line("read 0x%08lx / %s from 0x%08lx", action->read_0, buf, (action->gdb_parameter.address_length.address + action->intern[INTERN_MEMORY_OFFSET]));
        action->intern[INTERN_MEMORY_OFFSET] = action->intern[INTERN_MEMORY_OFFSET] + 4;
        action->cur_phase = 0;
        if(action->gdb_parameter.address_length.length <= action->intern[INTERN_MEMORY_OFFSET])
        {
            // finished
            reply_packet_send();
            return RESULT_OK;
        }
        else
        {
            // continue with next register
            return ERR_NOT_COMPLETED;
        }
    }

    return ERR_WRONG_STATE;
}

bool target_command_halt_cpu(void)
{
    return target_command_halt_cortex_m_cpu();
}

Result target_write(uint32_t start_address, uint8_t* data, uint32_t length)
{
    // TODO
    (void) start_address;
    (void) data;
    (void) length;
    debug_error("ERROR: target_write() not implemented !!!");
    return RESULT_OK;
}
