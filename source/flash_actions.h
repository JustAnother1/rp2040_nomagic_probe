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

#ifndef SOURCE_FLASH_ACTIONS_H_
#define SOURCE_FLASH_ACTIONS_H_

#include <stdint.h>
#include "probe_api/common.h"
#include "probe_api/result.h"

Result flash_erase_32kb(action_data_typ* const action, uint32_t start_address);
Result flash_erase_4kb(action_data_typ* const action, uint32_t start_address);
Result flash_erase_64kb(action_data_typ* const action, uint32_t start_address);
Result flash_write_page(action_data_typ* const action, uint32_t start_address, uint8_t* data , uint32_t length);
Result flash_initialize(action_data_typ* const action);

#endif /* SOURCE_FLASH_ACTIONS_H_ */
