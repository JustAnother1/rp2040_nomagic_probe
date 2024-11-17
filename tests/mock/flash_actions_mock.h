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

#ifndef MOCK_FLASH_ACTIONS_MOCK_H_
#define MOCK_FLASH_ACTIONS_MOCK_H_

#include <stdbool.h>
#include "probe_api/result.h"

void set_return_for_flash_erase_32kb(Result val);
void set_expect_first_call_for_flash_erase_32kb(bool val);
uint32_t get_start_address_from_flash_erase_32kb(void);

void set_return_for_flash_erase_4kb(Result val);
void set_expect_first_call_for_flash_erase_4kb(bool val);
uint32_t get_start_address_from_flash_erase_4kb(void);

void set_return_for_flash_erase_64kb(Result val);
void set_expect_first_call_for_flash_erase_64kb(bool val);
uint32_t get_start_address_from_flash_erase_64kb(void);

void set_return_for_flash_initialize(Result val);
void set_expect_first_call_for_flash_initialize(bool val);

void set_return_for_flash_write_page(Result val);
void set_expect_first_call_for_flash_write_page(bool val);
uint32_t get_start_address_from_flash_write_page(void);
uint32_t get_length_from_flash_write_page(void);
uint8_t* get_data_ptr_from_flash_write_page(void);
uint8_t* get_copied_data_from_flash_write_page(void);


#endif /* MOCK_FLASH_ACTIONS_MOCK_H_ */
