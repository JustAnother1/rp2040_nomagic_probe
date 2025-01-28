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

#ifndef TARGET_SOURCE_INC_H_
#define TARGET_SOURCE_INC_H_

#include <stdint.h>

#define STACK_POINTER 0x20042000

#define FUNC void __attribute__((naked, used)) target_function() \
{                                                                \
    __asm__("ldr r4, =#STACK_POINTER\n"                          \
            "mov sp, r4\n"                                       \
            "bl exec_func\n"                                     \
            "bkpt #1\n");                                        \
}                                                                \
                                                                 \
static void __attribute__((used)) exec_func (void)

#endif /* TARGET_SOURCE_INC_H_ */
