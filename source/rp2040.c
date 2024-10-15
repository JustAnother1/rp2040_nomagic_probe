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

#include "probe_api/common.h"
#include "probe_api/cortex-m.h"
#include "probe_api/debug_log.h"
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
    "<memory type=\"flash\" start=\"0x10000000\" length=\"0x4000000\">\r\n" \
        "<property name=\"blocksize\">0x1000</property>\r\n" \
    "</memory>\r\n" \
    "<memory type=\"ram\" start=\"0x20000000\" length=\"0x20042000\"/>\r\n" \
"</memory-map>\r\n"


void target_init(void)
{
    flash_driver_init();
    common_target_init();
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
        debug_line("Target Status");
        debug_line("=============");
        debug_line("target: RP2040");
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
        send_part(THREADS_CONTENT, sizeof(THREADS_CONTENT), offset, len);
        return;
    }
    else if(0 == strncmp(filename, "memory-map", 10))
    {
        send_part(MEMORY_MAP_CONTENT, sizeof(MEMORY_MAP_CONTENT), offset, len);
        return;
    }

    debug_line("xfer:file invalid");
    // if we reach this, then the request was invalid
    reply_packet_prepare();
    reply_packet_add("E00");
    reply_packet_send();
}

// GDB_CMD_VFLASH_DONE
Result handle_target_reply_vFlashDone(action_data_typ* const action, bool first_call)
{
    Result res;

    if(true == first_call)
    {
        debug_line("Flash Done!");
    }

    // finish erasing the flash
    res = flash_driver_erase_finish();
    if(ERR_NOT_COMPLETED == res)
    {
        // Try again next time
        return res;
    }
    if(RESULT_OK != res)
    {
        debug_line("ERROR: finishing erase failed !");
        action->result = ERR_TARGET_ERROR;
        action->is_done = true;
        reply_packet_prepare();
        reply_packet_add(ERROR_TARGET_FAILED);
        reply_packet_send();
        return res;
    }

    // finish writing to flash
    res = flash_driver_write_finish();
    if(ERR_NOT_COMPLETED == res)
    {
        // Try again next time
        return res;
    }
    if(RESULT_OK != res)
    {
        debug_line("ERROR: finishing write failed !");
        action->result = ERR_TARGET_ERROR;
        action->is_done = true;
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

// GDB_CMD_VFLASH_ERASE
Result handle_target_reply_vFlashErase(action_data_typ* const action, bool first_call)
{
    Result res;
    uint32_t start_address = action->gdb_parameter.address_length.address;
    uint32_t length = action->gdb_parameter.address_length.length;

    if(ADDRESS_LENGTH != action->gdb_parameter.type)
    {
        // wrong parameter type
        debug_line("ERROR: wrong parameter type !");
        action->result = ERR_WRONG_VALUE;
        action->is_done = true;
        reply_packet_prepare();
        reply_packet_add(ERROR_CODE_INVALID_PARAMETER_FORMAT_TYPE);
        reply_packet_send();
        return ERR_WRONG_VALUE;
    }

    if(true == first_call)
    {
        debug_line("Flash erase: address : 0x%08lx, length: 0x%08lx", start_address, length);
    }

    res = flash_driver_add_erase_range(start_address, length);
    if(ERR_NOT_COMPLETED == res)
    {
        // Try again next time
        return res;
    }
    if(RESULT_OK != res)
    {
        debug_line("ERROR: adding erase range failed !");
        action->result = ERR_TARGET_ERROR;
        action->is_done = true;
        reply_packet_prepare();
        reply_packet_add(ERROR_TARGET_FAILED);
        reply_packet_send();
        return res;
    }

    reply_packet_prepare();
    reply_packet_add("OK");
    reply_packet_send();
    return RESULT_OK;
}

// GDB_CMD_VFLASH_WRITE
Result handle_target_reply_vFlashWrite(action_data_typ* const action, bool first_call)
{
    Result res;
    uint32_t start_address = action->gdb_parameter.address_binary.address;
    uint32_t length = action->gdb_parameter.address_binary.data_length; // length can be up to MAX_BINARY_SIZE_BYTES
    uint8_t* data = (uint8_t*)action->gdb_parameter.address_binary.data;

    if(ADDRESS_MEMORY != action->gdb_parameter.type)
    {
        // wrong parameter type
        debug_line("ERROR: wrong parameter type !");
        action->result = ERR_WRONG_VALUE;
        action->is_done = true;
        reply_packet_prepare();
        reply_packet_add(ERROR_CODE_INVALID_PARAMETER_FORMAT_TYPE);
        reply_packet_send();
        return ERR_WRONG_VALUE;
    }

    if(true == first_call)
    {
        debug_line("Flash write: address : 0x%08lx", start_address);
        debug_line("Flash write: length : %ld", length);
        action->intern[INTERN_ALREADY_WRITTEN_BYTES] = 0;
    }

    res = flash_driver_write(action, start_address, length, data);
    if(ERR_NOT_COMPLETED == res)
    {
        // Try again next time
        return res;
    }
    if(RESULT_OK != res)
    {
        debug_line("ERROR: flash write failed !");
        action->result = ERR_TARGET_ERROR;
        action->is_done = true;
        reply_packet_prepare();
        reply_packet_add(ERROR_TARGET_FAILED);
        reply_packet_send();
        return res;
    }

    reply_packet_prepare();
    reply_packet_add("OK");
    reply_packet_send();
    return RESULT_OK;
}
