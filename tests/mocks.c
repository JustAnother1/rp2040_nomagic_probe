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
#include <stdint.h>
#include "probe_api/common.h"
#include "probe_api/result.h"

void send_part(char* part, uint32_t size, uint32_t offset, uint32_t length)
{
    (void)part;
    (void)size;
    (void)offset;
    (void)length;
}

bool common_cmd_target_info(uint32_t loop)
{
    (void)loop;
    return true;
}

void reply_packet_prepare(void)
{

}

void reply_packet_add(char* data)
{
    (void)data;
}

void reply_packet_send(void)
{

}

void mon_cmd_version(void)
{

}

void encode_text_to_hex_string(char * text, uint32_t buf_length, char * buf)
{
    (void)text;
    (void)buf_length;
    (void)buf;
}

void gdb_is_not_busy_anymore(void)
{

}

bool add_action(action_typ act)
{
    (void)act;
    return false;
}

Result do_connect(action_data_typ* const action)
{
    (void)action;
    return ERR_WRONG_VALUE;
}

Result do_disconnect(action_data_typ* const action)
{
    (void)action;
    return ERR_WRONG_VALUE;
}

Result do_read_ap_reg(action_data_typ* const action, uint32_t bank, uint32_t reg)
{
    (void)action;
    (void)bank;
    (void)reg;
    return ERR_WRONG_VALUE;
}

Result do_write_ap_reg(action_data_typ* const action, uint32_t bank, uint32_t reg, uint32_t data)
{
    (void)action;
    (void)bank;
    (void)reg;
    (void)data;
    return ERR_WRONG_VALUE;
}

Result do_read_ap(action_data_typ* const action, uint32_t address)
{
    (void)action;
    (void)address;
    return ERR_WRONG_VALUE;
}

Result do_write_ap(action_data_typ* const action, uint32_t address, uint32_t data)
{
    (void)action;
    (void)address;
    (void)data;
    return ERR_WRONG_VALUE;
}

Result do_get_Result_OK(action_data_typ* const action)
{
    (void)action;
    return ERR_WRONG_VALUE;
}

Result do_get_Result_data(action_data_typ* const action)
{
    (void)action;
    return ERR_WRONG_VALUE;
}

void mon_cmd_help(char* command)
{
    (void)command;
}

bool add_action_with_parameter(action_typ act, parameter_typ* parsed_parameter)
{
    (void)act;
    (void)parsed_parameter;
    return false;
}

void div_and_mod(uint32_t divident, uint32_t divisor, uint32_t* quotient, uint32_t* remainder)
{
    *remainder = divident%divisor;
    *quotient = divident/divisor;
}
