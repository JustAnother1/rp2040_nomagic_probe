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

#include <stdint.h>
#include "probe_api/common.h"
#include "probe_api/result.h"

void flash_driver_init(void)
{

}

Result flash_driver_add_erase_range(uint32_t start_address, uint32_t length)
{
    (void)start_address;
    (void)length;
    return RESULT_OK;
}

Result flash_driver_write(action_data_typ* const action, uint32_t start_address, uint32_t length, uint8_t* data)
{
    (void)action;
    (void)start_address;
    (void)length;
    (void)data;
    return RESULT_OK;
}

Result flash_driver_erase_finish(void)
{
    return RESULT_OK;
}

Result flash_driver_write_finish(void)
{
    return RESULT_OK;
}
