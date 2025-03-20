/*
  automatically created hal/startup.c
  created at: 2025-03-17 01:14:53
  created from mbsp.json
*/

#include <hal/hw/CLOCKS.h>
#include <hal/hw/IO_BANK0.h>
#include <hal/hw/PADS_BANK0.h>
#include <hal/hw/PLL_SYS.h>
#include <hal/hw/PPB.h>
#include <hal/hw/PSM.h>
#include <hal/hw/RESETS.h>
#include <hal/hw/SIO.h>
#include <hal/hw/XOSC.h>

volatile uint32_t ms_since_boot = 0;


void SysTick_Handler(void)
{
    ms_since_boot++;
    
}

/* start up code */
__attribute__((__noreturn__)) void Reset_Handler(void);
extern int main(void);
extern uint32_t __bss_start;
extern uint32_t __bss_end;
extern uint32_t __data_start;
extern uint32_t __data_end;
extern uint32_t __data_in_flash;
__attribute__((__noreturn__)) void Reset_Handler(void)
{
    uint32_t *bss_start_p =  &__bss_start;
    uint32_t *bss_end_p = &__bss_end;
    uint32_t *data_start_p =  &__data_start;
    uint32_t *data_end_p = &__data_end;
    uint32_t *data_src_p = &__data_in_flash;
    __asm volatile ("LDR     R0,=0x20041ffc \n"
    "MOV     SP, R0"); // set stack pointer
    PSM->FRCE_ON = 0x1ffff; // power on all needed blocks
    // wait for powered on blocks to become available
    while (0xffff != (0xffff & PSM->DONE))
    {
        ;
        
    }
    RESETS->RESET = 0x1fbec1d;  // Put everything into reset (just to be sure in case of software reset)
    // everything excludes: SYSCFG, PLL_SYS, PADS_QSPI, PADS_BANK0, JTAG, IOQSPI, IOBANK0, and BUSCTRL
    while (0x1082 != (0x1082 & RESETS->RESET_DONE))
    {
        ;
        
    }
    // configure clock: pico has XOSC = 12 MHz
    XOSC->STARTUP = 47; // = 47 * 256 = 1ms @ 12MHz
    // power up XOSC
    XOSC->CTRL = 0xfabaa0;  // enable @ 1..15 MHz
    // wait for XOSC to stabilize
    while (0 == (XOSC_STATUS_STABLE_MASK & XOSC->STATUS))
    {
        ;
        
    }
    // switch clk_ref and clk_sys to XOSC
    CLOCKS->CLK_REF_CTRL = (CLOCKS_CLK_REF_CTRL_SRC_xosc_clksrc << CLOCKS_CLK_REF_CTRL_SRC_OFFSET);
    CLOCKS->WAKE_EN0 = 0xffffffff; // all clcoks enabled
    CLOCKS->WAKE_EN1 = 0x7fff; // all clcoks enabled
    // wait for switch to happen
    while (4 != CLOCKS->CLK_REF_SELECTED)
    {
        ;
        
    }
    // configure system PLL
    // program the clk_ref divider ( 1)
    PLL_SYS->CS = 1;
    // program feedback divider ( 125)
    PLL_SYS->FBDIV_INT = 125;
    // turn on main power and VCO
    PLL_SYS->PWR = 0;
    // wait for VCO clock to lock
    while (0 == (PLL_SYS_CS_LOCK_MASK & PLL_SYS->CS))
    {
        ;
        
    }
    // set up post dividers and turn them on (6, 2)
    PLL_SYS->PRIM = (6 << PLL_SYS_PRIM_POSTDIV1_OFFSET) |(2 << PLL_SYS_PRIM_POSTDIV2_OFFSET);
    // switch sys aux to PLL
    CLOCKS->CLK_SYS_CTRL = 0;
    // switch sys mux to aux
    CLOCKS->CLK_SYS_CTRL = 1;
    // wait for locked
    while (CLOCKS_CLK_REF_CTRL_SRC_xosc_clksrc != CLOCKS->CLK_SYS_SELECTED)
    {
        ;
        
    }
    // peripheral clock
    // disable clock divider
    CLOCKS->CLK_PERI_CTRL = 0;
    // wait for the generated clock to stop (2 clock cycles of clock source)
    // change the auxiliary mux select control
    CLOCKS->CLK_PERI_CTRL = (CLOCKS_CLK_PERI_CTRL_AUXSRC_clksrc_pll_sys << CLOCKS_CLK_PERI_CTRL_AUXSRC_OFFSET); // clock source is PLL_SYS - can glitch
    // enable the clock divider
    CLOCKS->CLK_PERI_CTRL = (1 << CLOCKS_CLK_PERI_CTRL_ENABLE_OFFSET) | (CLOCKS_CLK_PERI_CTRL_AUXSRC_clksrc_pll_sys << CLOCKS_CLK_PERI_CTRL_AUXSRC_OFFSET);
    // wait for the clock generator to restart (2 clock cycles of clock source)
    __asm volatile ("nop");
    __asm volatile ("nop");
    
    RESETS->RESET = RESETS->RESET & ~ RESETS_RESET_IO_BANK0_MASK; // take IO_BANK0 out of Reset
    PSM->FRCE_ON = PSM->FRCE_ON | PSM_FRCE_ON_SIO_MASK; // make sure that SIO is powered on
    SIO->GPIO_OE_CLR = 1ul << 25;
    SIO->GPIO_OUT_CLR = 1ul << 25;
    PADS_BANK0->GPIO25 = PADS_BANK0_GPIO0_DRIVE_12mA << PADS_BANK0_GPIO0_DRIVE_OFFSET;
    IO_BANK0->GPIO25_CTRL = 5; // 5 == SIO
    SIO->GPIO_OE_SET = 1ul << 25;
    
    
    // initialize variables to their initialization value
    while(data_start_p < data_end_p)
    {
        *data_start_p = *data_src_p;
        data_start_p++;
        data_src_p++;
        
    }
    // initialize global variables to zero
    while(bss_start_p < bss_end_p)
    {
        *bss_start_p = 0;
        bss_start_p++;
        
    }
    ms_since_boot = 0;
    PPB->SYST_CSR = (1 << PPB_SYST_CSR_ENABLE_OFFSET) | (1 << PPB_SYST_CSR_TICKINT_OFFSET) | (1 << PPB_SYST_CSR_CLKSOURCE_OFFSET);     // SysTick on
    PPB->SYST_RVR = 125000;  // reload value = CPU clock in Hz / 1000
    
    for(;;)
    {
        main();
        // main exited - WTF???
        
    }
    
}

