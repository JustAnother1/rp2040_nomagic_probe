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
#include "flash_actions.h"
#include "probe_api/activity.h"
#include "probe_api/debug_log.h"
#include "probe_api/steps.h"
#include "hal/hw/RESETS.h"
#include "hal/hw/PSM.h"
#include "hal/hw/PADS_QSPI.h"
#include "hal/hw/IO_QSPI.h"
#include "hal/hw/XIP_CTRL.h"
#include "hal/hw/XIP_SSI.h"
#include "hal/qspi_flash.h"

// TODO user configurable?
#define QSPI_BAUDRATE_DIVIDOR     2

#define FIFO_SIZE 10  // is probably 16 but just to be sure

// Register address offsets for atomic RMW aliases
#define REG_ALIAS_RW_BITS  (0x0u << 12u)
#define REG_ALIAS_XOR_BITS (0x1u << 12u)
#define REG_ALIAS_SET_BITS (0x2u << 12u)
#define REG_ALIAS_CLR_BITS (0x3u << 12u)

static Result flash_erase_param(flash_action_data_typ* const state, uint32_t start_address, uint32_t erase_cmd);

static uint32_t val; // a value read from a register or prepared to be written into a register
static uint32_t status; // read status value from Flash
static uint32_t tx_level;
static uint32_t rx_level;
static uint32_t cnt; // a counter
static uint32_t cnt_2; // another counter
static activity_data_typ act_state;

Result flash_initialize(flash_action_data_typ* const state)
{
    Result res;

    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }

    if(true == state->first_call)
    {
        debug_line("starting flash_initialize()");
        state->phase = 0;
        state->first_call = false;
        act_state.first_call =true;
    }

    //debug_line("f%ld", state->phase);

    // power on QSPI
    if(0 == state->phase)
    {
        res = act_read_register(&act_state, &(PSM->FRCE_ON), &val);
        if(RESULT_OK == res)
        {
            state->phase++;
            act_state.first_call =true;
        }
        else
        {
            return res;
        }
    }

    if(1 == state->phase)
    {
        val = val | (1 << PSM_FRCE_ON_XIP_OFFSET);
        res = step_write_ap(&(PSM->FRCE_ON), val);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    // reset QSPI
    if(2 == state->phase)
    {
        res = act_read_register(&act_state, &(RESETS->RESET), &val);
        if(RESULT_OK == res)
        {
            state->phase++;
            act_state.first_call =true;
        }
        else
        {
            return res;
        }
    }

    if(3 == state->phase)
    {
        val = val | (1 << RESETS_RESET_IO_QSPI_OFFSET) | (1 << RESETS_RESET_PADS_QSPI_OFFSET);
        res = step_write_ap(&(RESETS->RESET), val);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }
    if(4 == state->phase)
    {
        val = val & ~((uint32_t)(1 << RESETS_RESET_IO_QSPI_OFFSET) | (uint32_t)(1 << RESETS_RESET_PADS_QSPI_OFFSET));
        res = step_write_ap(&(RESETS->RESET), val);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    // wait for reset done
    if(5 == state->phase)
    {
        res = act_read_register(&act_state, &(RESETS->RESET), &val);
        if(RESULT_OK == res)
        {
            uint32_t reset_mask = (1 << RESETS_RESET_IO_QSPI_OFFSET) | (1 << RESETS_RESET_PADS_QSPI_OFFSET);
            if(0 == (val & reset_mask))  // bit is 0 if reset is done
            {
                // Reset is now done
                state->phase++;
                act_state.first_call =true;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    if(6 == state->phase)
    {
        res = step_write_ap(&(PADS_QSPI->VOLTAGE_SELECT), 0); // 3.3V
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(7 == state->phase)
    {
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SCLK),
                               (1<< PADS_QSPI_GPIO_QSPI_SCLK_IE_OFFSET)           // Input enable
                             | (PADS_QSPI_GPIO_QSPI_SCLK_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SCLK_DRIVE_OFFSET)
                             | (1 << PADS_QSPI_GPIO_QSPI_SCLK_PDE_OFFSET)         // Pull down enable
                             | (1 << PADS_QSPI_GPIO_QSPI_SCLK_SCHMITT_OFFSET)     // enable schmitt trigger
                             | (1 << PADS_QSPI_GPIO_QSPI_SCLK_SLEWFAST_OFFSET) ); // slew rate fast

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(8 == state->phase)
    {
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD0),
                               (1<< PADS_QSPI_GPIO_QSPI_SD0_IE_OFFSET)           // Input enable
                             | (PADS_QSPI_GPIO_QSPI_SD0_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD0_DRIVE_OFFSET)
                             | (1 << PADS_QSPI_GPIO_QSPI_SD0_PDE_OFFSET)         // Pull down enable
                             | (1 << PADS_QSPI_GPIO_QSPI_SD0_SCHMITT_OFFSET)     // enable schmitt trigger
                             | (1 << PADS_QSPI_GPIO_QSPI_SD0_SLEWFAST_OFFSET) ); // slew rate fast

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(9 == state->phase)
    {
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD1),
                               (1<< PADS_QSPI_GPIO_QSPI_SD1_IE_OFFSET)           // Input enable
                             | (PADS_QSPI_GPIO_QSPI_SD1_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD1_DRIVE_OFFSET)
                             | (1 << PADS_QSPI_GPIO_QSPI_SD1_PDE_OFFSET)         // Pull down enable
                             | (1 << PADS_QSPI_GPIO_QSPI_SD1_SCHMITT_OFFSET)     // enable schmitt trigger
                             | (1 << PADS_QSPI_GPIO_QSPI_SD1_SLEWFAST_OFFSET) ); // slew rate fast

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(10 == state->phase)
    {
        // put pull-up on SD2/SD3 as these may be used as WPn/HOLDn
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD2),
                               (1<< PADS_QSPI_GPIO_QSPI_SD2_IE_OFFSET)           // Input enable
                             | (PADS_QSPI_GPIO_QSPI_SD2_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD2_DRIVE_OFFSET)
                             | (1 << PADS_QSPI_GPIO_QSPI_SD2_PUE_OFFSET)         // Pull up enable
                             | (1 << PADS_QSPI_GPIO_QSPI_SD2_SCHMITT_OFFSET)     // enable schmitt trigger
                             | (1 << PADS_QSPI_GPIO_QSPI_SD2_SLEWFAST_OFFSET) ); // slew rate fast
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(11 == state->phase)
    {
        // put pull-up on SD2/SD3 as these may be used as WPn/HOLDn
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD3),
                               (1<< PADS_QSPI_GPIO_QSPI_SD3_IE_OFFSET)           // Input enable
                             | (PADS_QSPI_GPIO_QSPI_SD3_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD3_DRIVE_OFFSET)
                             | (1 << PADS_QSPI_GPIO_QSPI_SD3_PUE_OFFSET)         // Pull up enable
                             | (1 << PADS_QSPI_GPIO_QSPI_SD3_SCHMITT_OFFSET)     // enable schmitt trigger
                             | (1 << PADS_QSPI_GPIO_QSPI_SD3_SLEWFAST_OFFSET) ); // slew rate fast
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(12 == state->phase)
    {
        // put pull-up on SD2/SD3 as these may be used as WPn/HOLDn
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SS),
                               (1<< PADS_QSPI_GPIO_QSPI_SS_IE_OFFSET)           // Input enable
                             | (PADS_QSPI_GPIO_QSPI_SS_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SS_DRIVE_OFFSET)
                             | (1 << PADS_QSPI_GPIO_QSPI_SS_PUE_OFFSET)         // Pull up enable
                             | (1 << PADS_QSPI_GPIO_QSPI_SS_SCHMITT_OFFSET)     // enable schmitt trigger
                             | (1 << PADS_QSPI_GPIO_QSPI_SS_SLEWFAST_OFFSET) ); // slew rate fast
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(13 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SCLK_CTRL), 0);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(14 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), 0);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(15 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SD0_CTRL), 0);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(16 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SD1_CTRL), 0);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(17 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SD2_CTRL), 0);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(18 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SD3_CTRL), 0);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(19 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->INTR), 0xcccccc); // write to clear
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(20 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->PROC0_INTE), 0);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(21 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->PROC0_INTF), 0);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(22 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->PROC1_INTE), 0);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(23 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->PROC1_INTF), 0);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(24 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->DORMANT_WAKE_INTE), 0);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(25 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->DORMANT_WAKE_INTF), 0);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(26 == state->phase)
    {
        res = step_write_ap(&(XIP_CTRL->CTRL), 0); // ignore bad memory accesses, keep cache powered
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(27 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->SSIENR), 0); // Disable SSI for further configuration
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(28 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->SER), (1 << XIP_SSI_SER_SER_OFFSET)); // slave selected
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(29 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->BAUDR), (QSPI_BAUDRATE_DIVIDOR << XIP_SSI_BAUDR_SCKDV_OFFSET)); // set baud rate
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(30 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->TXFTLR), 0); // TX FIFO threshold
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(31 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->RXFTLR), 0); // RX FIFO threshold
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(32 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->IMR), 0); // no interrupts masked
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(33 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DMACR), 0); // no DMA
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(34 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DMATDLR), 0); // transmit data water mark level
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(35 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DMARDLR), 4); // receive data water mark level (data sheet says it should not be changed from 4)
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(36 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->RX_SAMPLE_DLY), (1 << XIP_SSI_RX_SAMPLE_DLY_RSD_OFFSET)); // delay in System clock cycles
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(37 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->TXD_DRIVE_EDGE), 0);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(38 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->CTRLR0),
                                    // TODO Quad SPI, 32bit
                                    // SSTE = Slave select toggle enable
                                    (XIP_SSI_CTRLR0_SPI_FRF_STD << XIP_SSI_CTRLR0_SPI_FRF_OFFSET) | // Standard 1-bit SPI serial frames
                                    (7 << XIP_SSI_CTRLR0_DFS_32_OFFSET) | // 8 clocks per data frame
                                    (XIP_SSI_CTRLR0_TMOD_TX_AND_RX << XIP_SSI_CTRLR0_TMOD_OFFSET)  // TX and RX FIFOs are both used for every byte
                                    // CFS = Control Frame size = Microwire only !
                                    // SRL = Shift Register loop (test mode)
                                    // SLV_OE = Slave Output enable
                                    // SPI MOD = 0 = (SCPOL = 0; SCPH = 0)
                                    // FRF = 00 = Motorola SPI
                                    // DFS = invalid (dfs_32 is used) writing has no effect!
                                    );
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(39 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->CTRLR1), 0); // NDF = 0 = number of data frames used with Quad SPI // TODO
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(40 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->SPI_CTRLR0),
                                    (3 << XIP_SSI_SPI_CTRLR0_XIP_CMD_OFFSET) //  SPI Command 0x03 = read
                                  | (0 << XIP_SSI_SPI_CTRLR0_WAIT_CYCLES_OFFSET)
                                  | (XIP_SSI_SPI_CTRLR0_INST_L_8B << XIP_SSI_SPI_CTRLR0_INST_L_OFFSET)
                                  | (6 << XIP_SSI_SPI_CTRLR0_ADDR_L_OFFSET) // in 4 bit increments -> 24 bit = 6
                                    );
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(41 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->ICR), &val); // clear all active interrupts
        if(RESULT_OK == res)
        {
            state->phase++;
            act_state.first_call =true;
        }
        else
        {
            return res;
        }
    }

    if(42 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val); // Clear sticky errors (clear-on-read)
        if(RESULT_OK == res)
        {
            state->phase++;
            act_state.first_call =true;
        }
        else
        {
            return res;
        }
    }

    if(43 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->ICR), &val); // Clear sticky errors (clear-on-read)
        if(RESULT_OK == res)
        {
            state->phase++;
            act_state.first_call =true;
        }
        else
        {
            return res;
        }
    }

    if(44 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->SSIENR), 1); // Re-enable SSI
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    // make sure we are not in XIP mode (Continuous Read Mode)
    // Sequence:
    // 1. CSn = 1, IO = 4'h0 (via pull-down to avoid contention), x32 clocks
    // 2. CSn = 0, IO = 4'hf (via pull-up to avoid contention), x32 clocks
    // 3. CSn = 1 (brief deassertion)
    // 4. CSn = 0, MOSI = 1'b1 driven, x16 clocks
    //
    // Part 4 is the sequence suggested in W25X10CL data sheet.
    // Parts 1 and 2 are to improve compatibility with Micron parts

    // 1. CSn = 1, IO = 4'h0 (via pull-down to avoid contention), x32 clocks
        // First two 32-clock sequences
        // CSn is held high for the first 32 clocks, then asserted low for next 32

    // wait for TFE (Transmit FIFO Empty) = 1
    if(45 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            act_state.first_call =true;
            if(XIP_SSI_SR_TFE_MASK == (val & XIP_SSI_SR_TFE_MASK))
            {
                // TFE (Transmit FIFO Empty) = 1
                state->phase++;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    // wait for busy = idle
    if(46 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            act_state.first_call =true;
            if(0 == (val & XIP_SSI_SR_BUSY_MASK))
            {
                // busy = idle
                state->phase++;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    if(47 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (3 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(48 == state->phase)
    {
        val = (1 < PADS_QSPI_GPIO_QSPI_SD0_OD_OFFSET)          // output disabled
            | (1<< PADS_QSPI_GPIO_QSPI_SD0_IE_OFFSET)          // Input enable
            | (PADS_QSPI_GPIO_QSPI_SD0_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD0_DRIVE_OFFSET)
            | (1 << PADS_QSPI_GPIO_QSPI_SD0_PDE_OFFSET)        // Pull down enable
            | (1 << PADS_QSPI_GPIO_QSPI_SD0_SCHMITT_OFFSET)    // enable schmitt trigger
            | (1 << PADS_QSPI_GPIO_QSPI_SD0_SLEWFAST_OFFSET);  // slew rate fast

        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD0), val);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(49 == state->phase)
    {
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD1), val);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(50 == state->phase)
    {
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD2), val);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(51 == state->phase)
    {
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD3), val);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(52 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), 0);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(53 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), 0);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(54 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), 0);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(55 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), 0);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    // wait for TFE (Transmit FIFO Empty) = 1
    if(56 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            act_state.first_call =true;
            if(XIP_SSI_SR_TFE_MASK == (val & XIP_SSI_SR_TFE_MASK))
            {
                // TFE (Transmit FIFO Empty) = 1
                state->phase++;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    // wait for busy = idle
    if(57 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            act_state.first_call =true;
            if(0 == (val & XIP_SSI_SR_BUSY_MASK))
            {
                // busy = idle
                state->phase++;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    if(58 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (3 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(59 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (2 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    // 2. CSn = 0, IO = 4'hf (via pull-up to avoid contention), x32 clocks

    if(60 == state->phase)
    {
        val = (1 < PADS_QSPI_GPIO_QSPI_SD0_OD_OFFSET)          // output disabled
            | (1<< PADS_QSPI_GPIO_QSPI_SD0_IE_OFFSET)          // Input enable
            | (PADS_QSPI_GPIO_QSPI_SD0_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD0_DRIVE_OFFSET)
            | (1 << PADS_QSPI_GPIO_QSPI_SD0_PUE_OFFSET)        // Pull up enable
            | (1 << PADS_QSPI_GPIO_QSPI_SD0_SCHMITT_OFFSET)    // enable schmitt trigger
            | (1 << PADS_QSPI_GPIO_QSPI_SD0_SLEWFAST_OFFSET);  // slew rate fast

        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD0), val);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(61 == state->phase)
    {
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD1), val);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(62 == state->phase)
    {
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD2), val);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(63 == state->phase)
    {
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD3), val);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(64 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), 0);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(65 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), 0);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(66 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), 0);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(67 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), 0);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    // 3. CSn = 1 (brief deassertion)

    // wait for TFE (Transmit FIFO Empty) = 1
    if(68 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            act_state.first_call =true;
            if(XIP_SSI_SR_TFE_MASK == (val & XIP_SSI_SR_TFE_MASK))
            {
                // TFE (Transmit FIFO Empty) = 1
                state->phase++;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    // wait for busy = idle
    if(69 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            act_state.first_call =true;
            if(0 == (val & XIP_SSI_SR_BUSY_MASK))
            {
                // busy = idle
                state->phase++;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    if(70 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (3 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(71 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (2 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    // 4. CSn = 0, MOSI = 1'b1 driven, x16 clocks

    if(72 == state->phase)
    {
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SCLK),
                               (1<< PADS_QSPI_GPIO_QSPI_SCLK_IE_OFFSET)           // Input enable
                             | (PADS_QSPI_GPIO_QSPI_SCLK_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SCLK_DRIVE_OFFSET)
                             | (1 << PADS_QSPI_GPIO_QSPI_SCLK_PDE_OFFSET)         // Pull down enable
                             | (1 << PADS_QSPI_GPIO_QSPI_SCLK_SCHMITT_OFFSET)     // enable schmitt trigger
                             | (1 << PADS_QSPI_GPIO_QSPI_SCLK_SLEWFAST_OFFSET) ); // slew rate fast

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(73 == state->phase)
    {
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD0),
                               (1<< PADS_QSPI_GPIO_QSPI_SD0_IE_OFFSET)           // Input enable
                             | (PADS_QSPI_GPIO_QSPI_SD0_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD0_DRIVE_OFFSET)
                             | (1 << PADS_QSPI_GPIO_QSPI_SD0_PDE_OFFSET)         // Pull down enable
                             | (1 << PADS_QSPI_GPIO_QSPI_SD0_SCHMITT_OFFSET)     // enable schmitt trigger
                             | (1 << PADS_QSPI_GPIO_QSPI_SD0_SLEWFAST_OFFSET) ); // slew rate fast

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(74 == state->phase)
    {
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD1),
                               (1<< PADS_QSPI_GPIO_QSPI_SD1_IE_OFFSET)           // Input enable
                             | (PADS_QSPI_GPIO_QSPI_SD1_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD1_DRIVE_OFFSET)
                             | (1 << PADS_QSPI_GPIO_QSPI_SD1_PDE_OFFSET)         // Pull down enable
                             | (1 << PADS_QSPI_GPIO_QSPI_SD1_SCHMITT_OFFSET)     // enable schmitt trigger
                             | (1 << PADS_QSPI_GPIO_QSPI_SD1_SLEWFAST_OFFSET) ); // slew rate fast

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(75 == state->phase)
    {
        // put pull-up on SD2/SD3 as these may be used as WPn/HOLDn
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD2),
                               (1<< PADS_QSPI_GPIO_QSPI_SD2_IE_OFFSET)           // Input enable
                             | (PADS_QSPI_GPIO_QSPI_SD2_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD2_DRIVE_OFFSET)
                             | (1 << PADS_QSPI_GPIO_QSPI_SD2_PUE_OFFSET)         // Pull up enable
                             | (1 << PADS_QSPI_GPIO_QSPI_SD2_SCHMITT_OFFSET)     // enable schmitt trigger
                             | (1 << PADS_QSPI_GPIO_QSPI_SD2_SLEWFAST_OFFSET) ); // slew rate fast

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(76 == state->phase)
    {
        // put pull-up on SD2/SD3 as these may be used as WPn/HOLDn
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD3),
                               (1<< PADS_QSPI_GPIO_QSPI_SD3_IE_OFFSET)           // Input enable
                             | (PADS_QSPI_GPIO_QSPI_SD3_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD3_DRIVE_OFFSET)
                             | (1 << PADS_QSPI_GPIO_QSPI_SD3_PUE_OFFSET)         // Pull up enable
                             | (1 << PADS_QSPI_GPIO_QSPI_SD3_SCHMITT_OFFSET)     // enable schmitt trigger
                             | (1 << PADS_QSPI_GPIO_QSPI_SD3_SLEWFAST_OFFSET) ); // slew rate fast

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(77 == state->phase)
    {
        // put pull-up on SD2/SD3 as these may be used as WPn/HOLDn
        res = step_write_ap(&(PADS_QSPI->GPIO_QSPI_SD3),
                               (1<< PADS_QSPI_GPIO_QSPI_SS_IE_OFFSET)           // Input enable
                             | (PADS_QSPI_GPIO_QSPI_SS_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SS_DRIVE_OFFSET)
                             | (1 << PADS_QSPI_GPIO_QSPI_SS_PUE_OFFSET)         // Pull up enable
                             | (1 << PADS_QSPI_GPIO_QSPI_SS_SCHMITT_OFFSET)     // enable schmitt trigger
                             | (1 << PADS_QSPI_GPIO_QSPI_SS_SLEWFAST_OFFSET) ); // slew rate fast

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(78 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (2 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(79 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), 0xff);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(80 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), 0xff);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    // wait for TFE (Transmit FIFO Empty) = 1
    if(81 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            act_state.first_call =true;
            if(XIP_SSI_SR_TFE_MASK == (val & XIP_SSI_SR_TFE_MASK))
            {
                // TFE (Transmit FIFO Empty) = 1
                state->phase++;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    // wait for busy = idle
    if(82 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            act_state.first_call =true;
            if(0 == (val & XIP_SSI_SR_BUSY_MASK))
            {
                // busy = idle
                state->phase++;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    if(83 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (3 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            // action->cur_phase++;
            return RESULT_OK;
        }
        else
        {
            return res;
        }
    }

    return ERR_WRONG_STATE;
}

Result flash_erase_64kb(flash_action_data_typ* const state, uint32_t start_address)
{
    // erase sector of size 64KB
    return flash_erase_param(state, start_address, FLASHCMD_BLOCK_ERASE_64KB);
}

Result flash_erase_32kb(flash_action_data_typ* const state, uint32_t start_address)
{
    // erase sector of size 32KB
    return flash_erase_param(state, start_address, FLASHCMD_BLOCK_ERASE_32KB);
}

Result flash_erase_4kb(flash_action_data_typ* const state, uint32_t start_address)
{
    // erase sector of size 4KB
    return flash_erase_param(state, start_address, FLASHCMD_SECTOR_ERASE);
}

static Result flash_erase_param(flash_action_data_typ* const state, uint32_t start_address, uint32_t erase_cmd)
{
    Result res;

    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }

    if(true == state->first_call)
    {
        debug_line("starting flash_erase(0x%02lx @0x%08lx)", erase_cmd, start_address);
        state->phase = 0;
        state->first_call = false;
        act_state.first_call =true;
    }

    // debug_line("f%ld", state->phase);

    if(0 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (2 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(1 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), FLASHCMD_WRITE_ENABLE);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    // wait for TFE (Transmit FIFO Empty) = 1
    if(2 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            if(XIP_SSI_SR_TFE_MASK == (val & XIP_SSI_SR_TFE_MASK))
            {
                // TFE (Transmit FIFO Empty) = 1
                state->phase++;
                act_state.first_call =true;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    // wait for busy = idle
    if(3 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            if(0 == (val & XIP_SSI_SR_BUSY_MASK))
            {
                // busy = idle
                state->phase++;
                act_state.first_call =true;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    if(4 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (3 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(5 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (2 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(6 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), (0xff & erase_cmd));

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(7 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), (0xff & start_address>>16));

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(8 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), (0xff & start_address>>8));

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(9 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), (0xff & start_address));

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    // wait for TFE (Transmit FIFO Empty) = 1
    if(10 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            if(XIP_SSI_SR_TFE_MASK == (val & XIP_SSI_SR_TFE_MASK))
            {
                // TFE (Transmit FIFO Empty) = 1
                state->phase++;
                act_state.first_call =true;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    // wait for busy = idle
    if(11 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            if(0 == (val & XIP_SSI_SR_BUSY_MASK))
            {
                // busy = idle
                state->phase++;
                act_state.first_call =true;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    if(12 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (3 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            cnt = 5;
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(13 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->RXFLR), &val);
        if((RESULT_OK == res) && (0 < val))
        {
            state->phase++;
            act_state.first_call =true;
        }
        else
        {
            return res;
        }
    }

    if(14 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->DR0), &val);
        if(RESULT_OK == res)
        {
            cnt--;
            if(0 == cnt)
            {
                state->phase++;
                act_state.first_call =true;
            }
            else if(0 == val)
            {
                state->phase = 13; // read RXFLR again
                return ERR_NOT_COMPLETED;
            }
            else
            {
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    // begin of status read loop
    if(15 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (2 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(16 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), (0xff & FLASHCMD_READ_STATUS));

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(17 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->RXFLR), &val);
        if((RESULT_OK == res) && (0 < val))
        {
            state->phase++;
            act_state.first_call =true;
        }
        else
        {
            return res;
        }
    }

    if(18 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->DR0), &val);  // skip a byte
        if(RESULT_OK == res)
        {
            state->phase++;
            act_state.first_call =true;
        }
        else
        {
            return res;
        }
    }

    if(19 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->DR0), &status);
        if(RESULT_OK == res)
        {
            state->phase++;
            act_state.first_call =true;
        }
        else
        {
            return res;
        }
    }

    // wait for TFE (Transmit FIFO Empty) = 1
    if(20 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            if(XIP_SSI_SR_TFE_MASK == (val & XIP_SSI_SR_TFE_MASK))
            {
                // TFE (Transmit FIFO Empty) = 1
                state->phase++;
                act_state.first_call =true;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    // wait for busy = idle
    if(21 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            if(0 == (val & XIP_SSI_SR_BUSY_MASK))
            {
                // busy = idle
                state->phase++;
                act_state.first_call =true;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    if(22 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (3 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            cnt = 5;
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(23 == state->phase)
    {
        if(status & STATUS_REGISTER_BUSY)
        {
            // still busy
            state->phase = 15; // go back to start of loop
            return ERR_NOT_COMPLETED;
        }
        else
        {
            return RESULT_OK;
        }
    }

    return ERR_WRONG_STATE;
}

Result flash_write_page(flash_action_data_typ* const state, uint32_t start_address, uint8_t* data , uint32_t length)
{
    Result res;

    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }

    if(true == state->first_call)
    {
        debug_line("starting flash_write_page(@0x%08lx %ld)", start_address, length);
        // write up to 256 bytes
        if(start_address < 0x10000000)
        {
            debug_line("ERROR: invalid start address(0x%08lx)", start_address);
            return ERR_WRONG_VALUE;
        }
        if(0 != (start_address & 0xffu))
        {
            debug_line("ERROR: start address not aligned (0x%08lx)", start_address);
            return ERR_WRONG_VALUE;
        }
        if(256 < length)
        {
            debug_line("ERROR: write too short (%ld)", length);
            return ERR_WRONG_VALUE;
        }

        state->phase = 0;
        state->first_call = false;
        act_state.first_call =true;
    }

    if(0 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (2 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(1 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), FLASHCMD_WRITE_ENABLE);

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    // wait for TFE (Transmit FIFO Empty) = 1
    if(2 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            if(XIP_SSI_SR_TFE_MASK == (val & XIP_SSI_SR_TFE_MASK))
            {
                // TFE (Transmit FIFO Empty) = 1
                state->phase++;
                act_state.first_call =true;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    // wait for busy = idle
    if(3 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            if(0 == (val & XIP_SSI_SR_BUSY_MASK))
            {
                // busy = idle
                state->phase++;
                act_state.first_call =true;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    if(4 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (3 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(5 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (2 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(6 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), (0xff & FLASHCMD_PAGE_PROGRAM));

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(7 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), (0xff & start_address>>16));

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(8 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), (0xff & start_address>>8));

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(9 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), (0xff & start_address));

        if(RESULT_OK == res)
        {
            cnt = 5;
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(10 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->RXFLR), &val);
        if((RESULT_OK == res) && (0 < val))
        {
            state->phase++;
            act_state.first_call = true;
        }
        else
        {
            return res;
        }
    }

    if(11 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->DR0), &val);
        if(RESULT_OK == res)
        {
            cnt--;
            if(0 == cnt)
            {
                state->phase++;
                act_state.first_call = true;
            }
            else if(0 == val)
            {
                state->phase = 13; // read RXFLR again
                return ERR_NOT_COMPLETED;
            }
            else
            {
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    // start of copy loop
    if(12 == state->phase)
    {
        cnt = 0;  // no byte send
        cnt_2 = 0; // no byte received
        state->phase++;
    }

    if(13 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->TXFLR), &tx_level);
        if(RESULT_OK == res)
        {
            if(tx_level < FIFO_SIZE)
            {
                state->phase++;
                act_state.first_call = true;
            }
            else
            {
                // FIFO is full
                // -> read it again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    if(14 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->RXFLR), &rx_level);
        if(RESULT_OK == res)
        {
            state->phase++;
            act_state.first_call = true;
        }
        else
        {
            return res;
        }
    }

    if(15 == state->phase)
    {
        if(0 < rx_level)
        {
            res = act_read_register(&act_state, &(XIP_SSI->DR0), &val);
            if(RESULT_OK == res)
            {
                cnt_2++; // received a byte
                rx_level--;
                state->phase++;
                act_state.first_call = true;
            }
            else
            {
                return res;
            }
        }
        else
        {
            state->phase++;
        }
    }

    if(16 == state->phase)
    {
        if(length > cnt)
        {
            res = step_write_ap(&(XIP_SSI->DR0), data[cnt]);
            if(RESULT_OK == res)
            {
                cnt++; // send a byte
                tx_level++;
                if(tx_level > FIFO_SIZE)
                {
                    state->phase = 13; // read FIFO levels next
                }
                else
                {
                    state->phase = 15;  // read/write another byte
                }
                return ERR_NOT_COMPLETED;
            }
            else
            {
                return res;
            }
        }
        else
        {
            // we send all data
            state->phase++;
        }
    }

    // wait for TFE (Transmit FIFO Empty) = 1
    if(17 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            if(XIP_SSI_SR_TFE_MASK == (val & XIP_SSI_SR_TFE_MASK))
            {
                // TFE (Transmit FIFO Empty) = 1
                state->phase++;
                act_state.first_call = true;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    // wait for busy = idle
    if(18 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            if(0 == (val & XIP_SSI_SR_BUSY_MASK))
            {
                // busy = idle
                state->phase++;
                act_state.first_call = true;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    if(19 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (3 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }


    // begin of status read loop
    if(20 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (2 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(21 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->DR0), (0xff & FLASHCMD_READ_STATUS));

        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(22 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->RXFLR), &val);
        if((RESULT_OK == res) && (0 < val))
        {
            state->phase++;
            act_state.first_call = true;
        }
        else
        {
            return res;
        }
    }

    if(23 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->DR0), &val);  // skip a byte
        if(RESULT_OK == res)
        {
            state->phase++;
            act_state.first_call = true;
        }
        else
        {
            return res;
        }
    }

    if(24 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->DR0), &status);
        if(RESULT_OK == res)
        {
            state->phase++;
            act_state.first_call = true;
        }
        else
        {
            return res;
        }
    }

    // wait for TFE (Transmit FIFO Empty) = 1
    if(25 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            if(XIP_SSI_SR_TFE_MASK == (val & XIP_SSI_SR_TFE_MASK))
            {
                // TFE (Transmit FIFO Empty) = 1
                state->phase++;
                act_state.first_call = true;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    // wait for busy = idle
    if(26 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_SSI->SR), &val);
        if(RESULT_OK == res)
        {
            if(0 == (val & XIP_SSI_SR_BUSY_MASK))
            {
                // busy = idle
                state->phase++;
                act_state.first_call = true;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    if(27 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), (3 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET));
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(28 == state->phase)
    {
        if(status & STATUS_REGISTER_BUSY)
        {
            // still busy
            state->phase = 20; // go back to start of loop
        }
        else
        {
            return RESULT_OK;
        }
    }

    return ERR_WRONG_STATE;
}

Result flash_enter_XIP(flash_action_data_typ* const state)
{
    Result res;

    if(NULL == state)
    {
        return ERR_ACTION_NULL;
    }

    if(true == state->first_call)
    {
        debug_line("starting enter XiP mode sequence...");

        state->phase = 0;
        state->first_call = false;
        act_state.first_call =true;
    }

    // start of flash_flush_cache()
    if(0 == state->phase)
    {
        res = step_write_ap(&(XIP_CTRL->FLUSH), 1);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    // wait for Flash has completed
    if(1 == state->phase)
    {
        res = act_read_register(&act_state, &(XIP_CTRL->STAT), &val);
        if(RESULT_OK == res)
        {
            if(1 == (val & 1))
            {
                // Flush completed
                state->phase++;
            }
            else
            {
                // read again
                return ERR_NOT_COMPLETED;
            }
        }
        else
        {
            return res;
        }
    }

    if(2 == state->phase)
    {
        res = step_write_ap(&(XIP_CTRL->CTRL) + REG_ALIAS_SET_BITS, 1);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(3 == state->phase)
    {
        res = act_read_register(&act_state, &(IO_QSPI->GPIO_QSPI_SS_CTRL), &val);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(4 == state->phase)
    {
        res = step_write_ap(&(IO_QSPI->GPIO_QSPI_SS_CTRL), val & (uint32_t)~IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_MASK);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }
    // end of flash_flush_cache()

    // start of flash_enter_cmd_xip()

    if(5 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->SSIENR), 0);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(6 == state->phase)
    {
        // 0 << 21 Standard 1-bit SPI serial frames; 31 << 16 32 clocks per data frame; 3 << 8 Send instr + addr, receive data
        res = step_write_ap(&(XIP_SSI->CTRLR0), 0x200300);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(7 == state->phase)
    {
        // 3 << 24 Standard 03h read; 2 << 8 8-bit instruction prefix 6 << 2 24-bit addressing for 03h commands; 0 << 0 Command and address both in serial format
        res = step_write_ap(&(XIP_SSI->SPI_CTRLR0), 0x3000218);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    if(8 == state->phase)
    {
        res = step_write_ap(&(XIP_SSI->SSIENR), 1);
        if(RESULT_OK == res)
        {
            state->phase++;
        }
        else
        {
            return res;
        }
    }

    // end of flash_enter_cmd_xip()

/*
    // This is a hook for steps to be taken in between programming the flash and
    // doing cached XIP reads from the flash. Called by the bootrom before
    // entering flash second stage, and called by the debugger after flash
    // programming.
    void flash_flush_cache()
    {
        XIP_CTRL->FLUSH = 1;
        while(0 == XIP_CTRL->STAT & 1)
        {
            ;
        }
        // Enable the cache
        (XIP_CTRL->CTRL + REG_ALIAS_SET_BITS) = 1;

        IO_QSPI->GPIO_QSPI_SS_CTRL = (IO_QSPI->GPIO_QSPI_SS_CTRL & ~IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_MASK);
    }


    // Put the SSI into a mode where XIP accesses translate to standard
    // serial 03h read commands. The flash remains in its default serial command
    // state, so will still respond to other commands.
    void flash_enter_cmd_xip()
    {
        XIP_SSI->SSIENR = 0;

        XIP_SSI->CTRLR0 = 0x200300; // 0 << 21 Standard 1-bit SPI serial frames; 31 << 16 32 clocks per data frame; 3 << 8 Send instr + addr, receive data
        // ssi->ctrlr0 =
        //         (SSI_CTRLR0_SPI_FRF_VALUE_STD << SSI_CTRLR0_SPI_FRF_LSB) |  // Standard 1-bit SPI serial frames
        //         (31 << SSI_CTRLR0_DFS_32_LSB) |                             // 32 clocks per data frame
        //         (SSI_CTRLR0_TMOD_VALUE_EEPROM_READ << SSI_CTRLR0_TMOD_LSB); // Send instr + addr, receive data

        XIP_SSI->SPI_CTRLR0 = 0x3000218; // 3 << 24 Standard 03h read; 2 << 8 8-bit instruction prefix 6 << 2 24-bit addressing for 03h commands; 0 << 0 Command and address both in serial format

        // ssi->spi_ctrlr0 =
        //         (FLASHCMD_READ_DATA << SSI_SPI_CTRLR0_XIP_CMD_LSB) | // Standard 03h read
        //         (2u << SSI_SPI_CTRLR0_INST_L_LSB) |    // 8-bit instruction prefix
        //         (6u << SSI_SPI_CTRLR0_ADDR_L_LSB) |    // 24-bit addressing for 03h commands
        //         (SSI_SPI_CTRLR0_TRANS_TYPE_VALUE_1C1A  // Command and address both in serial format
        //                 << SSI_SPI_CTRLR0_TRANS_TYPE_LSB);
        XIP_SSI->SSIENR = 1;
    }
*/

    if(9 == state->phase)
    {
        return RESULT_OK;
    }

    return ERR_WRONG_STATE;
}

