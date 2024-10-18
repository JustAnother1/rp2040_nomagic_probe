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

#ifndef SOURCE_RP2040_FLASH_DRIVER_H_
#define SOURCE_RP2040_FLASH_DRIVER_H_

#include <stdint.h>
#include "probe_api/common.h"
#include "probe_api/result.h"

#define INTERN_ALREADY_WRITTEN_BYTES  0

void flash_driver_init(void);
Result flash_driver_add_erase_range(action_data_typ* const action, uint32_t start_address, uint32_t length);
Result flash_driver_write(action_data_typ* const action, uint32_t start_address, uint32_t length, uint8_t* data);
Result flash_driver_erase_finish(action_data_typ* const action);
Result flash_driver_write_finish(action_data_typ* const action);


#endif /* SOURCE_RP2040_FLASH_DRIVER_H_ */