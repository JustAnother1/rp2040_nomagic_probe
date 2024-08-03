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

#include "common.h"
#include "cortex-m.h"
#include "debug_log.h"
#include "gdb_monitor_commands.h"
#include "gdb_packets.h"
#include "hex.h"
#include "result.h"
#include "steps.h"
#include "swd.h"
#include "target.h"
#include "util.h"

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

#define INTERN_RETRY_COUNTER    1
#define INTERN_REGISTER_IDX     2
#define MAX_LINE_LENGTH         50


static void mon_cmd_halt(void);
static void mon_cmd_reset(char* command);
static void mon_cmd_reg(char* command);

static parameter_typ parsed_parameter;
static char msg_buf[MAX_LINE_LENGTH];

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

Result handle_target_reply_vFlashDone(action_data_typ* const action, bool first_call)
{
    (void) action;
    (void) first_call;
    debug_line("Flash Done!");
    // TODO
    reply_packet_prepare();
    reply_packet_add("OK");
    reply_packet_send();
    return RESULT_OK;
}

Result handle_target_reply_vFlashErase(action_data_typ* const action, bool first_call)
{
    (void) action;
    (void) first_call;
    debug_line("Flash erase: address : 0x%08lx, length: 0x%08lx",
               action->gdb_parameter->address, action->gdb_parameter->length);
    // TODO
    reply_packet_prepare();
    reply_packet_add("OK");
    reply_packet_send();
    return RESULT_OK;
}

Result handle_target_reply_vFlashWrite(action_data_typ* const action, bool first_call)
{
    (void) action;
    (void) first_call;
    debug_line("Flash write: address : 0x%08lx", action->gdb_parameter->address);
    // TODO
    reply_packet_prepare();
    reply_packet_add("OK");
    reply_packet_send();
    return RESULT_OK;
}

// monitor commands inspired by / copied from :
// https://openocd.org/doc/html/General-Commands.html

void target_monitor_command(uint32_t which, char* command)
{
    switch(which)
    {
    case MON_CMD_IDX_HELP:
        mon_cmd_help(command);
        break;

    case MON_CMD_IDX_VERSION:
        mon_cmd_version();
        break;

    case MON_CMD_IDX_RESET:
        mon_cmd_reset(command);
        break;

    case MON_CMD_IDX_HALT:
        mon_cmd_halt();
        break;

    case MON_CMD_IDX_REG:
        mon_cmd_reg(command);
        break;

    // new monitor commands go here

    default:
    {
        char buf[100];
        reply_packet_prepare();
        reply_packet_add("O"); // packet is $ big oh, hex string# checksum
        encode_text_to_hex_string("ERROR: invalid command !\r\n", sizeof(buf), buf);
        reply_packet_add(buf);
        reply_packet_send();

        reply_packet_prepare();
        reply_packet_add("E00");
        reply_packet_send();

        gdb_is_not_busy_anymore();
    }
        break;
    }
}

/*

(gdb) monitor reg
===== arm v7m registers
(0) r0 (/32): 0x00000080
(1) r1 (/32): 0x2002f91c (dirty)
(2) r2 (/32): 0x00000000 (dirty)
(3) r3 (/32): 0x00000000 (dirty)
(4) r4 (/32): 0x2002386c
(5) r5 (/32): 0x00000002
(6) r6 (/32): 0x00000001
(7) r7 (/32): 0x000195d4 (dirty)
(8) r8 (/32): 0x20021274
(9) r9 (/32): 0x000069ae
(10) r10 (/32): 0x100005c0
(11) r11 (/32): 0x200343c0
(12) r12 (/32): 0x00002ee0
(13) sp (/32): 0x20041f64 (dirty)
(14) lr (/32): 0x20006fab (dirty)
(15) pc (/32): 0x2000a87c (dirty)
(16) xPSR (/32): 0x61000000 (dirty)
(17) msp (/32): 0x20041f64 (dirty)
(18) psp (/32): 0xfffffffc
(20) primask (/1): 0x00 (dirty)
(21) basepri (/8): 0x00
(22) faultmask (/1): 0x00
(23) control (/3): 0x00
===== Cortex-M DWT registers
(gdb)

(gdb) monitor reg r0
r0 (/32): 0x00000080
(gdb)

(gdb) monitor reg r1
r1 (/32): 0x2002f91c
(gdb)


 */



static void mon_cmd_halt(void)
{
    // Command: halt [ms]
    // Command: wait_halt [ms]
    // The halt command first sends a halt request to the target, which wait_halt
    // doesn’t. Otherwise these behave the same: wait up to ms milliseconds, or
    // 5 seconds if there is no parameter, for the target to halt (and enter
    // debug mode). Using 0 as the ms parameter prevents OpenOCD from waiting.

    add_action(GDB_CMD_MON_HALT);
}

static void mon_cmd_reset(char* command)
{
    // Perform as hard a reset as possible, using SRST if possible. All defined
    // targets will be reset, and target events will fire during the reset sequence.
    if(0 == strncmp("reset init", command, sizeof("reset init")))
    {
        // Immediately halt the target, and execute the reset-init script
        add_action(GDB_CMD_MON_RESET_INIT);
    }
    else if(0 == strncmp("reset halt", command, sizeof("reset halt")))
    {
        // Immediately halt the target
    }
    else
    {
        // reset or reset run
        // Let the target run
    }
}

static void mon_cmd_reg(char* command)
{
    // command can be "reg" or reg + register name like this "reg r0"
    bool valid_register = true;
    if(3 == strlen(command))
    {
        // command is "reg"
        char buf[100];
        parsed_parameter.has_index = false;

        reply_packet_prepare();
        reply_packet_add("O"); // packet is $ big oh, hex string# checksum
        encode_text_to_hex_string("===== arm v7m registers\r\n", sizeof(buf), buf);
        reply_packet_add(buf);
        reply_packet_send();
    }
    else
    {
        // command is "reg" + register name
        char* register_name = command + 4;
        parsed_parameter.has_index = true;
        if('r' == *register_name)
        {
            switch(register_name[1])
            {
            case '0': parsed_parameter.index = 0; break;
            case '1': switch(register_name[2])
                      {
                      case 0 :  parsed_parameter.index = 1; break;
                      case '0': parsed_parameter.index = 10; break;
                      case '1': parsed_parameter.index = 11; break;
                      case '2': parsed_parameter.index = 12; break;
                      default: valid_register = false; break;
                      }
                      break;
            case '2': parsed_parameter.index = 2; break;
            case '3': parsed_parameter.index = 3; break;
            case '4': parsed_parameter.index = 4; break;
            case '5': parsed_parameter.index = 5; break;
            case '6': parsed_parameter.index = 6; break;
            case '7': parsed_parameter.index = 7; break;
            case '8': parsed_parameter.index = 8; break;
            case '9': parsed_parameter.index = 9; break;

            default: valid_register = false; break;
            }
        }
        else
        {
            if(0 == strncmp("sp", command, sizeof("sp")))
            {
                parsed_parameter.index = 13;
            }
            else if(0 == strncmp("lr", command, sizeof("lr")))
            {
                parsed_parameter.index = 14;
            }
            else if(0 == strncmp("pc", command, sizeof("pc")))
            {
                parsed_parameter.index = 15;
            }
            else if(0 == strncmp("xPSR", command, sizeof("xPSR")))
            {
                parsed_parameter.index = 16;
            }
            else if(0 == strncmp("msp", command, sizeof("msp")))
            {
                parsed_parameter.index = 17;
            }
            else if(0 == strncmp("psp", command, sizeof("psp")))
            {
                parsed_parameter.index = 18;
            }
            else if(0 == strncmp("primask", command, sizeof("primask")))
            {
                parsed_parameter.index = 20;
            }
            else if(0 == strncmp("basepri", command, sizeof("basepri")))
            {
                parsed_parameter.index = 21;
            }
            else if(0 == strncmp("faultmask", command, sizeof("faultmask")))
            {
                parsed_parameter.index = 22;
            }
            else if(0 == strncmp("control", command, sizeof("control")))
            {
                parsed_parameter.index = 23;
            }
            else
            {
                valid_register = false;
            }
        }
    }

    if(false == valid_register)
    {
        char buf[100];
        reply_packet_prepare();
        reply_packet_add("O"); // packet is $ big oh, hex string# checksum
        encode_text_to_hex_string("ERROR: invalid register name given !\r\n", sizeof(buf), buf);
        reply_packet_add(buf);
        reply_packet_send();

        reply_packet_prepare();
        reply_packet_add(command);
        reply_packet_send();

        reply_packet_prepare();
        reply_packet_add("E01");
        reply_packet_send();

        gdb_is_not_busy_anymore();
    }
    else
    {
        add_action_with_parameter(GDB_MONITOR_REG, &parsed_parameter);
    }
}

Result handle_monitor_reg(action_data_typ* const action, bool first_call)
{

    if(true == first_call)
    {
        reply_packet_prepare();
        *(action->cur_phase) = 0;
        if(true == action->gdb_parameter->has_index)
        {
            // not all registers but just one
            action->intern[INTERN_REGISTER_IDX] = action->gdb_parameter->index;
        }
        else
        {
            // all registers
            action->intern[INTERN_REGISTER_IDX] = 0;
        }
        action->intern[INTERN_RETRY_COUNTER] = 0;
    }

    // 1. write to DCRSR the REGSEL value and REGWnR = 0
    if(0 == *(action->cur_phase))
    {
        return do_write_ap(action, DCRSR, action->intern[INTERN_REGISTER_IDX]);
    }

    // 2. read DHCSR until S_REGRDY is 1
    if(1 == *(action->cur_phase))
    {
        return do_read_ap(action, DHCSR);
    }

    if(2 == *(action->cur_phase))
    {
        return do_get_Result_data(action);
    }

    if(3 == *(action->cur_phase))
    {
        if(0 == (action->read_0 & (1<<DHCSR_S_REGRDY_OFFSET)))
        {
            // Register not ready
            action->intern[INTERN_RETRY_COUNTER]++;
            if(10 < action->intern[INTERN_RETRY_COUNTER])
            {
                // no data available -> read again
                *(action->cur_phase) = 1;
                return ERR_NOT_COMPLETED;
            }
            else
            {
                // too many retries
                debug_line("ERROR: too many retries !");
                action->result = ERR_TIMEOUT;
                action->is_done = true;
                reply_packet_send();
                return action->result;
            }
        }
        else
        {
            // data available -> read data
            *(action->cur_phase) = *(action->cur_phase) + 1;
            action->intern[INTERN_RETRY_COUNTER] = 0;
        }
    }

    // 3. read data from DCRDR
    if(4 == *(action->cur_phase))
    {
        return do_read_ap(action, DCRDR);
    }

    if(5 == *(action->cur_phase))
    {
        return do_get_Result_data(action);
    }

    if(6 == *(action->cur_phase))
    {
        // this register done, next?
        reply_packet_add_hex(action->read_0, 8);
        debug_line("read 0x%08lx", action->read_0);
        action->intern[INTERN_REGISTER_IDX] ++;
        *(action->cur_phase) = 0;

        // clear buffer
        memset(&msg_buf, 0, sizeof(msg_buf));

        if(true == action->gdb_parameter->has_index)
        {
            // "r1 (/32): 0x2002f91c"
            // msg_buf
            // TODO
        }
        else
        {
            // (1) r1 (/32): 0x2002f91c (dirty)
            // TODO
        }

        if(   (true == action->gdb_parameter->has_index)
           || (17 == action->intern[INTERN_REGISTER_IDX]) )  // TODO 17 -> 23
        {
            // finished
            // end of output
            reply_packet_prepare();
            reply_packet_add("OK");
            reply_packet_send();

            gdb_is_not_busy_anymore();
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

Result handle_monitor_halt(action_data_typ* const action, bool first_call)
{
    // DHCSR.C_HALT == 1
    // then DHCSR.C_MASKINTS = 1 if ignore interrupt is enabled
    (void)action; // TODO
    if(true == first_call)
    {
        return ERR_NOT_COMPLETED;
    }
    return ERR_NOT_COMPLETED;
}

Result handle_monitor_reset_init(action_data_typ* const action, bool first_call)
{
    (void)action; // TODO
    if(true == first_call)
    {
        return ERR_NOT_COMPLETED;
    }
    return ERR_NOT_COMPLETED;
}





/*
#include <stdint.h>
#include <string.h>
#include "probe_api/gdb_packets.h"
#include "probe_api/hex.h"
#include "probe_api/result.h"
#include "probe_api/swd.h"
#include "probe_api/debug_log.h"
#include "probe_api/cortex-m.h"

static uint32_t val_DHCSR;

Result cortex_m_halt_cpu(bool first_call)
{
    // halt target DHCSR.C_HALT = 1 + check that DHCSR.S_HALT is 1
    static Result phase = 0;
    static Result transaction_id = 0;
    static uint32_t retries = 0;
    Result res;
    if(true == first_call)
    {
        phase = 1;
        retries = 0;
    }

    // write the halt command
    if(1 == phase)
    {
        uint32_t new_val = val_DHCSR | (1 <<1);
        if(new_val != val_DHCSR)
        {
            res = swd_write_ap(DHCSR, DBGKEY | (0xffff & val_DHCSR));
            if(RESULT_OK == res)
            {
                phase = 2;
            }
            else
            {
                return res;
            }
        }
        else
        {
            // already commanded to halt
            phase = 2;
        }
    }

    // read the status register
    if(2 == phase)
    {
        res = swd_read_ap(DHCSR);
        if(RESULT_OK < res)
        {
            transaction_id = res;
            phase = 3;
        }
        else
        {
            return res;
        }
    }

    // check if Halt status bit is set.
    if(3 == phase)
    {
        uint32_t data;
        res = swd_get_result(transaction_id, &data);
        if(RESULT_OK == res)
        {
            if(0 == (data & (1<<17)))
            {
                if(100 > retries)
                {
                    debug_line("TIMEOUT: when setting halt bit!");
                    return ERR_TIMEOUT;
                }
                else
                {
                    // not yet halted -> read again
                    phase = 2;
                    retries++;
                    return ERR_NOT_COMPLETED;
                }
            }
            else
            {
                // halted !
                phase = 4;
            }
        }
        else
        {
            return res;
        }
    }

    if(4 == phase)
    {
        return RESULT_OK;
    }

    debug_line("halt cpu: invalid phase (%ld)!", phase);
    return ERR_WRONG_STATE;
}
*/

