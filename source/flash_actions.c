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

#include "flash_actions.h"
#include "hal/hw/RESETS.h" // TODO

        /* TODO
static Result read_register(action_data_typ* const action, uint32_t address, uint32_t* read_value)
{
    if(0 == *(action->sub_phase))
    {
        return do_read_ap(action, address);
    }

    if(1 == *(action->cur_phase))
    {
        return do_get_Result_data(action);
    }

    if(2 == *(action->cur_phase))
    {
        return RESULT_OK;

        char buf[9];
        int_to_hex(buf, action->read_0, 8);
        buf[8] = 0;
        reply_packet_add(buf);
        // debug_line("read 0x%08lx", action->read_0);
        action->intern[INTERN_MEMORY_OFFSET] = action->intern[INTERN_MEMORY_OFFSET] + 4;
        *(action->cur_phase) = 0;
        if(action->gdb_parameter.address_length.length <= action->intern[INTERN_MEMORY_OFFSET])
        {
            // finished
            reply_packet_send();
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
        */


Result flash_initialize(action_data_typ* const action)
{
    // Result res;
    action->sub_phase = 0;
    return RESULT_OK;  // TODO
    // res = read_register(action, &(RESETS->RESET_DONE));
    // if(ERR_NOT_COMPLETED)

/*









    uint8_t buf[2];
    buf[0] = 0xff;
    buf[1] = 0xff;

{
    uint32_t reset_mask = (1 << RESETS_RESET_IO_QSPI_OFFSET) | (1 << RESETS_RESET_PADS_QSPI_OFFSET);

    // power on QSPI
    PSM->FRCE_ON =  PSM->FRCE_ON | (1 << PSM_FRCE_ON_XIP_OFFSET);

    // reset QSPI
    RESETS->RESET = RESETS->RESET | reset_mask;

    RESETS->RESET = RESETS->RESET & ~reset_mask;

    // wait for reset done
    do{
        ;
    } while(reset_mask != (RESETS->RESET_DONE & reset_mask));
}
{
    PADS_QSPI->VOLTAGE_SELECT = 0;  // 3.3V
    PADS_QSPI->GPIO_QSPI_SCLK = (1<< PADS_QSPI_GPIO_QSPI_SCLK_IE_OFFSET)         // Input enable
                              | (PADS_QSPI_GPIO_QSPI_SCLK_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SCLK_DRIVE_OFFSET)
                              | (1 << PADS_QSPI_GPIO_QSPI_SCLK_PDE_OFFSET)       // Pull down enable
                              | (1 << PADS_QSPI_GPIO_QSPI_SCLK_SCHMITT_OFFSET)   // enable schmitt trigger
                              | (1 << PADS_QSPI_GPIO_QSPI_SCLK_SLEWFAST_OFFSET); // slew rate fast
    PADS_QSPI->GPIO_QSPI_SD0 = (1<< PADS_QSPI_GPIO_QSPI_SD0_IE_OFFSET)           // Input enable
                              | (PADS_QSPI_GPIO_QSPI_SD0_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD0_DRIVE_OFFSET)
                              | (1 << PADS_QSPI_GPIO_QSPI_SD0_PDE_OFFSET)        // Pull down enable
                              | (1 << PADS_QSPI_GPIO_QSPI_SD0_SCHMITT_OFFSET)    // enable schmitt trigger
                              | (1 << PADS_QSPI_GPIO_QSPI_SD0_SLEWFAST_OFFSET);  // slew rate fast
    PADS_QSPI->GPIO_QSPI_SD1 = (1<< PADS_QSPI_GPIO_QSPI_SD1_IE_OFFSET)           // Input enable
                              | (PADS_QSPI_GPIO_QSPI_SD1_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD1_DRIVE_OFFSET)
                              | (1 << PADS_QSPI_GPIO_QSPI_SD1_PDE_OFFSET)        // Pull down enable
                              | (1 << PADS_QSPI_GPIO_QSPI_SD1_SCHMITT_OFFSET)    // enable schmitt trigger
                              | (1 << PADS_QSPI_GPIO_QSPI_SD1_SLEWFAST_OFFSET);  // slew rate fast
// put pull-up on SD2/SD3 as these may be used as WPn/HOLDn
    PADS_QSPI->GPIO_QSPI_SD2 =  (1<< PADS_QSPI_GPIO_QSPI_SD2_IE_OFFSET)          // Input enable
                              | (PADS_QSPI_GPIO_QSPI_SD2_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD2_DRIVE_OFFSET)
                              | (1 << PADS_QSPI_GPIO_QSPI_SD2_PUE_OFFSET)        // Pull up enable
                              | (1 << PADS_QSPI_GPIO_QSPI_SD2_SCHMITT_OFFSET)    // enable schmitt trigger
                              | (1 << PADS_QSPI_GPIO_QSPI_SD2_SLEWFAST_OFFSET);  // slew rate fast
    PADS_QSPI->GPIO_QSPI_SD3 =  (1<< PADS_QSPI_GPIO_QSPI_SD3_IE_OFFSET)          // Input enable
                              | (PADS_QSPI_GPIO_QSPI_SD3_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD3_DRIVE_OFFSET)
                              | (1 << PADS_QSPI_GPIO_QSPI_SD3_PUE_OFFSET)        // Pull up enable
                              | (1 << PADS_QSPI_GPIO_QSPI_SD3_SCHMITT_OFFSET)    // enable schmitt trigger
                              | (1 << PADS_QSPI_GPIO_QSPI_SD3_SLEWFAST_OFFSET);  // slew rate fast
    PADS_QSPI->GPIO_QSPI_SS = (1<< PADS_QSPI_GPIO_QSPI_SS_IE_OFFSET)             // Input enable
                              | (PADS_QSPI_GPIO_QSPI_SS_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SS_DRIVE_OFFSET)
                              | (1 << PADS_QSPI_GPIO_QSPI_SS_PUE_OFFSET)         // Pull up enable
                              | (1 << PADS_QSPI_GPIO_QSPI_SS_SCHMITT_OFFSET)     // enable schmitt trigger
                              | (1 << PADS_QSPI_GPIO_QSPI_SS_SLEWFAST_OFFSET);   // slew rate fast
}
{
    IO_QSPI->GPIO_QSPI_SCLK_CTRL = 0;
    IO_QSPI->GPIO_QSPI_SS_CTRL = 0;
    IO_QSPI->GPIO_QSPI_SD0_CTRL = 0;
    IO_QSPI->GPIO_QSPI_SD1_CTRL = 0;
    IO_QSPI->GPIO_QSPI_SD2_CTRL = 0;
    IO_QSPI->GPIO_QSPI_SD3_CTRL = 0;
    IO_QSPI->INTR = 0xcccccc; // write to clear
    IO_QSPI->PROC0_INTE = 0;
    IO_QSPI->PROC0_INTF = 0;
    IO_QSPI->PROC1_INTE = 0;
    IO_QSPI->PROC1_INTF = 0;
    IO_QSPI->DORMANT_WAKE_INTE = 0;
    IO_QSPI->DORMANT_WAKE_INTF = 0;
}
    XIP_CTRL->CTRL = 0;  // ignore bad memory accesses, keep cache powered
{
    volatile uint32_t help = 0;
    (void)help;

    // Disable SSI for further configuration
    XIP_SSI->SSIENR = 0;

    XIP_SSI->SER = (1 << XIP_SSI_SER_SER_OFFSET); // slave selected
    XIP_SSI->BAUDR = (QSPI_BAUDRATE_DIVIDOR << XIP_SSI_BAUDR_SCKDV_OFFSET);  // set baud rate
    XIP_SSI->TXFTLR = 0; // TX FIFO threshold
    XIP_SSI->RXFTLR = 0; // RX FIFO threshold
    XIP_SSI->IMR = 0;  // no interrupts masked
    //XIP_SSI->IMR = 0x3f; // all interrupts are masked
    XIP_SSI->DMACR = 0;  // no DMA
    XIP_SSI->DMATDLR = 0; // transmit data watermark level
    XIP_SSI->DMARDLR = 4; // receive data watermark level (data sheet says it should not be changed from 4)
    XIP_SSI->RX_SAMPLE_DLY = (1 << XIP_SSI_RX_SAMPLE_DLY_RSD_OFFSET);  // delay in System clock cycles
    XIP_SSI->TXD_DRIVE_EDGE = 0;

    // Set up the SSI controller for standard SPI mode,i.e. for every byte sent we get one back
    XIP_SSI->CTRLR0 =  //TODO Quad SPI, 32bit
            // SSTE = Slave select toggle enable
            (XIP_SSI_CTRLR0_SPI_FRF_STD << XIP_SSI_CTRLR0_SPI_FRF_OFFSET) | // Standard 1-bit SPI serial frames
            (7 << XIP_SSI_CTRLR0_DFS_32_OFFSET) | // 8 clocks per data frame
            (XIP_SSI_CTRLR0_TMOD_TX_AND_RX << XIP_SSI_CTRLR0_TMOD_OFFSET);  // TX and RX FIFOs are both used for every byte
            // CFS = Control Frame size = Microwire only !
            // SRL = Shift Register loop (test mode)
            // SLV_OE = Slave Output enable
            // SPI MOD = 0 = (SCPOL = 0; SCPH = 0)
            // FRF = 00 = Motorola SPI
            // DFS = invalid (dfs_32 is used) writing has no effect!
    XIP_SSI->CTRLR1 = 0; // NDF = 0 = number of data frames used with Quad SPI // TODO
    XIP_SSI->SPI_CTRLR0 =  // TODO
              (3 << XIP_SSI_SPI_CTRLR0_XIP_CMD_OFFSET) //  SPI Command 0x03 = read
            | (0 << XIP_SSI_SPI_CTRLR0_WAIT_CYCLES_OFFSET)
            | (XIP_SSI_SPI_CTRLR0_INST_L_8B<<XIP_SSI_SPI_CTRLR0_INST_L_OFFSET)
            | (6<<XIP_SSI_SPI_CTRLR0_ADDR_L_OFFSET); // in 4 bit increments -> 24 bit = 6
    help = XIP_SSI->ICR;  // clear all active interrupts
    // Clear sticky errors (clear-on-read)
    help = XIP_SSI->SR;
    help = XIP_SSI->ICR;

    // Re-enable
    XIP_SSI->SSIENR = 1;
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
{
    // wait for TFE (Transmit FIFO Empty) = 1
    while(XIP_SSI_SR_TFE_MASK != (XIP_SSI->SR & XIP_SSI_SR_TFE_MASK))
    {
        ;
    }
    // wait for busy = idle
    while(XIP_SSI_SR_BUSY_MASK == (XIP_SSI->SR & XIP_SSI_SR_BUSY_MASK))
    {
        ;
    }
    IO_QSPI->GPIO_QSPI_SS_CTRL = (IO_QSPI->GPIO_QSPI_SS_CTRL & ~((uint32_t)IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_MASK))
            | (3 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET);
}
{
    uint32_t val = (1 < PADS_QSPI_GPIO_QSPI_SD0_OD_OFFSET)          // output disabled
                 | (1<< PADS_QSPI_GPIO_QSPI_SD0_IE_OFFSET)          // Input enable
                 | (PADS_QSPI_GPIO_QSPI_SD0_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD0_DRIVE_OFFSET)
                 | (1 << PADS_QSPI_GPIO_QSPI_SD0_PDE_OFFSET)        // Pull down enable
                 | (1 << PADS_QSPI_GPIO_QSPI_SD0_SCHMITT_OFFSET)    // enable schmitt trigger
                 | (1 << PADS_QSPI_GPIO_QSPI_SD0_SLEWFAST_OFFSET);  // slew rate fast

    PADS_QSPI->GPIO_QSPI_SD0 = val;
    PADS_QSPI->GPIO_QSPI_SD1 = val;
    PADS_QSPI->GPIO_QSPI_SD2 = val;
    PADS_QSPI->GPIO_QSPI_SD3 = val;
}
    // Brief delay (~6000 cycles) for pulls to take effect
    delay_us(50);
    flash_put_get(NULL, NULL, 4, 0);  // output is disabled !
{
    // wait for TFE (Transmit FIFO Empty) = 1
    while(XIP_SSI_SR_TFE_MASK != (XIP_SSI->SR & XIP_SSI_SR_TFE_MASK))
    {
        ;
    }
    // wait for busy = idle
    while(XIP_SSI_SR_BUSY_MASK == (XIP_SSI->SR & XIP_SSI_SR_BUSY_MASK))
    {
        ;
    }
    IO_QSPI->GPIO_QSPI_SS_CTRL = (IO_QSPI->GPIO_QSPI_SS_CTRL & ~((uint32_t)IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_MASK))
            | (3 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET);
}
    delay_us(50);
    IO_QSPI->GPIO_QSPI_SS_CTRL = (IO_QSPI->GPIO_QSPI_SS_CTRL & ~((uint32_t)IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_MASK))
            | (2 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET);
    delay_us(50);

// 2. CSn = 0, IO = 4'hf (via pull-up to avoid contention), x32 clocks
    pads_qspi_output_disabled_pull_up();
    // Brief delay (~6000 cycles) for pulls to take effect
    delay_us(50);
    flash_put_get(NULL, NULL, 4, 0); // output is disabled !

// 3. CSn = 1 (brief deassertion)
{
    // wait for TFE (Transmit FIFO Empty) = 1
    while(XIP_SSI_SR_TFE_MASK != (XIP_SSI->SR & XIP_SSI_SR_TFE_MASK))
    {
        ;
    }
    // wait for busy = idle
    while(XIP_SSI_SR_BUSY_MASK == (XIP_SSI->SR & XIP_SSI_SR_BUSY_MASK))
    {
        ;
    }
    IO_QSPI->GPIO_QSPI_SS_CTRL = (IO_QSPI->GPIO_QSPI_SS_CTRL & ~((uint32_t)IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_MASK))
            | (3 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET);
}
    delay_us(50);
    IO_QSPI->GPIO_QSPI_SS_CTRL = (IO_QSPI->GPIO_QSPI_SS_CTRL & ~((uint32_t)IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_MASK))
            | (2 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET);

// 4. CSn = 0, MOSI = 1'b1 driven, x16 clocks
{
    PADS_QSPI->VOLTAGE_SELECT = 0;  // 3.3V
    PADS_QSPI->GPIO_QSPI_SCLK = (1<< PADS_QSPI_GPIO_QSPI_SCLK_IE_OFFSET)         // Input enable
                              | (PADS_QSPI_GPIO_QSPI_SCLK_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SCLK_DRIVE_OFFSET)
                              | (1 << PADS_QSPI_GPIO_QSPI_SCLK_PDE_OFFSET)       // Pull down enable
                              | (1 << PADS_QSPI_GPIO_QSPI_SCLK_SCHMITT_OFFSET)   // enable schmitt trigger
                              | (1 << PADS_QSPI_GPIO_QSPI_SCLK_SLEWFAST_OFFSET); // slew rate fast
    PADS_QSPI->GPIO_QSPI_SD0 = (1<< PADS_QSPI_GPIO_QSPI_SD0_IE_OFFSET)           // Input enable
                              | (PADS_QSPI_GPIO_QSPI_SD0_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD0_DRIVE_OFFSET)
                              | (1 << PADS_QSPI_GPIO_QSPI_SD0_PDE_OFFSET)        // Pull down enable
                              | (1 << PADS_QSPI_GPIO_QSPI_SD0_SCHMITT_OFFSET)    // enable schmitt trigger
                              | (1 << PADS_QSPI_GPIO_QSPI_SD0_SLEWFAST_OFFSET);  // slew rate fast
    PADS_QSPI->GPIO_QSPI_SD1 = (1<< PADS_QSPI_GPIO_QSPI_SD1_IE_OFFSET)           // Input enable
                              | (PADS_QSPI_GPIO_QSPI_SD1_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD1_DRIVE_OFFSET)
                              | (1 << PADS_QSPI_GPIO_QSPI_SD1_PDE_OFFSET)        // Pull down enable
                              | (1 << PADS_QSPI_GPIO_QSPI_SD1_SCHMITT_OFFSET)    // enable schmitt trigger
                              | (1 << PADS_QSPI_GPIO_QSPI_SD1_SLEWFAST_OFFSET);  // slew rate fast
// put pull-up on SD2/SD3 as these may be used as WPn/HOLDn
    PADS_QSPI->GPIO_QSPI_SD2 =  (1<< PADS_QSPI_GPIO_QSPI_SD2_IE_OFFSET)          // Input enable
                              | (PADS_QSPI_GPIO_QSPI_SD2_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD2_DRIVE_OFFSET)
                              | (1 << PADS_QSPI_GPIO_QSPI_SD2_PUE_OFFSET)        // Pull up enable
                              | (1 << PADS_QSPI_GPIO_QSPI_SD2_SCHMITT_OFFSET)    // enable schmitt trigger
                              | (1 << PADS_QSPI_GPIO_QSPI_SD2_SLEWFAST_OFFSET);  // slew rate fast
    PADS_QSPI->GPIO_QSPI_SD3 =  (1<< PADS_QSPI_GPIO_QSPI_SD3_IE_OFFSET)          // Input enable
                              | (PADS_QSPI_GPIO_QSPI_SD3_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SD3_DRIVE_OFFSET)
                              | (1 << PADS_QSPI_GPIO_QSPI_SD3_PUE_OFFSET)        // Pull up enable
                              | (1 << PADS_QSPI_GPIO_QSPI_SD3_SCHMITT_OFFSET)    // enable schmitt trigger
                              | (1 << PADS_QSPI_GPIO_QSPI_SD3_SLEWFAST_OFFSET);  // slew rate fast
    PADS_QSPI->GPIO_QSPI_SS = (1<< PADS_QSPI_GPIO_QSPI_SS_IE_OFFSET)             // Input enable
                              | (PADS_QSPI_GPIO_QSPI_SS_DRIVE_4mA << PADS_QSPI_GPIO_QSPI_SS_DRIVE_OFFSET)
                              | (1 << PADS_QSPI_GPIO_QSPI_SS_PUE_OFFSET)         // Pull up enable
                              | (1 << PADS_QSPI_GPIO_QSPI_SS_SCHMITT_OFFSET)     // enable schmitt trigger
                              | (1 << PADS_QSPI_GPIO_QSPI_SS_SLEWFAST_OFFSET);   // slew rate fast
}
    delay_us(50);
    IO_QSPI->GPIO_QSPI_SS_CTRL = (IO_QSPI->GPIO_QSPI_SS_CTRL & ~((uint32_t)IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_MASK))
            | (2 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET);
    flash_put_get(buf, NULL, 2, 0);
{
    // wait for TFE (Transmit FIFO Empty) = 1
    while(XIP_SSI_SR_TFE_MASK != (XIP_SSI->SR & XIP_SSI_SR_TFE_MASK))
    {
        ;
    }
    // wait for busy = idle
    while(XIP_SSI_SR_BUSY_MASK == (XIP_SSI->SR & XIP_SSI_SR_BUSY_MASK))
    {
        ;
    }
    IO_QSPI->GPIO_QSPI_SS_CTRL = (IO_QSPI->GPIO_QSPI_SS_CTRL & ~((uint32_t)IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_MASK))
            | (3 << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_OFFSET);
}

*/
}

/*

// Put bytes from one buffer, and get bytes into another buffer.
// These can be the same buffer.
// If tx is NULL then send zeroes.
// If rx is NULL then all read data will be dropped.
//
// If rx_skip is nonzero, this many bytes will first be consumed from the FIFO,
// before reading a further count bytes into *rx.
// E.g. if you have written a command+address just before calling this function.
static void flash_put_get(const uint8_t *tx, uint8_t *rx, size_t count, size_t rx_skip)
{
    size_t tx_count = count;
    size_t rx_count = count;
    while((0 != tx_count) || (0 != rx_skip) || (0 != rx_count))
    {
        // NB order of reads, for pessimism rather than optimism
        uint32_t tx_level = XIP_SSI->TXFLR;
        uint32_t rx_level = XIP_SSI->RXFLR;
        if(   (0 != tx_count)   // we still want to send something
           && ((tx_level + rx_level) < MAX_IN_FLIGHT)) // if the rx bufer will not be filled even if we receive bytes for everything in the send buffer
        {
            if(NULL != tx)
            {
                XIP_SSI->DR0 = *tx; // send a byte
                tx++;
            }
            else
            {
                XIP_SSI->DR0 = 0; // send zero
            }
            --tx_count; // one less byte to send
        }
        if(0 != rx_level) // we received something
        {
            uint8_t rxbyte = (uint8_t)XIP_SSI->DR0; // get the mext received byte
            if(0 != rx_skip)
            {
                --rx_skip; // we should skip this byte
            }
            else
            {
                if(NULL != rx) // the user is interested in what we read
                {
                    *rx++ = rxbyte; // then give the use the received byte
                }
                --rx_count; // one less byte that needs to arrive
            }
        }
    }
}


*/




Result flash_erase_64kb(action_data_typ* const action, uint32_t start_address)
{
    // TODO erase sector of size 64KB
    (void)start_address;
    (void)action;
    return RESULT_OK;
}

Result flash_erase_32kb(action_data_typ* const action, uint32_t start_address)
{
    // TODO erase sector of size 32KB
    (void)start_address;
    (void)action;
    return RESULT_OK;
}

Result flash_erase_4kb(action_data_typ* const action, uint32_t start_address)
{
    // TODO erase sector of size 4KB
    (void)start_address;
    (void)action;
    return RESULT_OK;
}

Result flash_write_page(action_data_typ* const action, uint32_t start_address, uint8_t* data , uint32_t length)
{
    // write up to 256 bytes
    // TODO
    (void)start_address;
    (void)action;
    (void)data;
    (void)length;
    return RESULT_OK;
}


