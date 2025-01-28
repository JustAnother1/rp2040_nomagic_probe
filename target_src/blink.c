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

#include "inc.h"
#include "hw/IO_BANK0.h"
#include "hw/PADS_BANK0.h"
#include "hw/PSM.h"
#include "hw/RESETS.h"
#include "hw/SIO.h"

#define NUM_LOOPS  100
#define NUM_CNT    11


FUNC
{
    uint32_t i;

    // init the LED pin
    RESETS->RESET = RESETS->RESET & ~0x00000020lu; // take IO_BANK0 out of Reset
    PSM->FRCE_ON = PSM->FRCE_ON | 0x00000400; // make sure that SIO is powered on
    SIO->GPIO_OE_CLR = 1ul << 25;
    SIO->GPIO_OUT_CLR = 1ul << 25;
    PADS_BANK0->GPIO25 = 0x00000030;
    IO_BANK0->GPIO25_CTRL = 5;  // 5 == SIO
    SIO->GPIO_OE_SET = 1ul << 25;

    for(i = 0; i < 10; i++)
    {
        // led on
        SIO->GPIO_OUT_SET = 1 << 25;

        // delay
        uint32_t loops = NUM_LOOPS;
        while(loops != 0)
        {
            volatile uint32_t cnt = NUM_CNT;
            while (cnt > 0)
            {
                cnt--;
            }
            loops--;
        }

        // led off
        SIO->GPIO_OUT_CLR = 1 << 25;

        // delay
        loops = NUM_LOOPS;
        while(loops != 0)
        {
            volatile uint32_t cnt = NUM_CNT;
            while (cnt > 0)
            {
                cnt--;
            }
            loops--;
        }
    }
}
