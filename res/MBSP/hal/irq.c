/*
  automatically created hal/irq.c
  created at: 2025-03-17 01:14:53
  created from mbsp.json
*/

typedef void (*VECTOR_FUNCTION_Type)(void);
void default_Handler(void)    __attribute__ ((weak, interrupt ("IRQ")));
__attribute__((__noreturn__)) void Reset_Handler(void);
void SysTick_Handler(void)    __attribute__ ((interrupt ("IRQ")));
const VECTOR_FUNCTION_Type __VECTOR_TABLE_RAM[64] __attribute__((used, section(".vectors"), aligned(0x100u)))
= {
    /*  0: Initial Stack Pointer    */ (VECTOR_FUNCTION_Type) (0x20041ffc),
    /*  1: Reset Handler            */ Reset_Handler,
    /*  2: NMI Handler        (-14) */ default_Handler,
    /*  3: Hard Fault Handler (-13) */ default_Handler,
    /*  4: reserved           (-12) */ default_Handler,
    /*  5: reserved           (-11) */ default_Handler,
    /*  6: reserved           (-10) */ default_Handler,
    /*  7: reserved           ( -9) */ default_Handler,
    /*  8: reserved           ( -8) */ default_Handler,
    /*  9: reserved           ( -7) */ default_Handler,
    /* 10: reserved           ( -6) */ default_Handler,
    /* 11: SVCall Handler     ( -5) */ default_Handler,
    /* 12: reserved           ( -4) */ default_Handler,
    /* 13: reserved           ( -3) */ default_Handler,
    /* 14: PendSV Handler     ( -2) */ default_Handler,
    /* 15: SysTick Handler    ( -1) */ SysTick_Handler,
    /* 16: TIMER_IRQ_0        (  0) */ default_Handler,
    /* 17: TIMER_IRQ_1        (  1) */ default_Handler,
    /* 18: TIMER_IRQ_2        (  2) */ default_Handler,
    /* 19: TIMER_IRQ_3        (  3) */ default_Handler,
    /* 20: PWM_IRQ_WRAP       (  4) */ default_Handler,
    /* 21: USBCTRL_IRQ        (  5) */ default_Handler,
    /* 22: XIP_IRQ            (  6) */ default_Handler,
    /* 23: PIO0_IRQ_0         (  7) */ default_Handler,
    /* 24: PIO0_IRQ_1         (  8) */ default_Handler,
    /* 25: PIO1_IRQ_0         (  9) */ default_Handler,
    /* 26: PIO1_IRQ_1         ( 10) */ default_Handler,
    /* 27: DMA_IRQ_0          ( 11) */ default_Handler,
    /* 28: DMA_IRQ_1          ( 12) */ default_Handler,
    /* 29: IO_IRQ_BANK0       ( 13) */ default_Handler,
    /* 30: IO_IRQ_QSPI        ( 14) */ default_Handler,
    /* 31: SIO_IRQ_PROC0      ( 15) */ default_Handler,
    /* 32: SIO_IRQ_PROC1      ( 16) */ default_Handler,
    /* 33: CLOCKS_IRQ         ( 17) */ default_Handler,
    /* 34: SPI0_IRQ           ( 18) */ default_Handler,
    /* 35: SPI1_IRQ           ( 19) */ default_Handler,
    /* 36: UART0_IRQ          ( 20) */ default_Handler,
    /* 37: UART1_IRQ          ( 21) */ default_Handler,
    /* 38: ADC_IRQ_FIFO       ( 22) */ default_Handler,
    /* 39: I2C0_IRQ           ( 23) */ default_Handler,
    /* 40: I2C1_IRQ           ( 24) */ default_Handler,
    /* 41: RTC_IRQ            ( 25) */ default_Handler,
    /* 42: reserved           ( 26) */ default_Handler,
    /* 43: reserved           ( 27) */ default_Handler,
    /* 44: reserved           ( 28) */ default_Handler,
    /* 45: reserved           ( 29) */ default_Handler,
    /* 46: reserved           ( 30) */ default_Handler,
    /* 47: reserved           ( 31) */ default_Handler,
    /* 48: reserved           ( 32) */ default_Handler,
    /* 49: reserved           ( 33) */ default_Handler,
    /* 50: reserved           ( 34) */ default_Handler,
    /* 51: reserved           ( 35) */ default_Handler,
    /* 52: reserved           ( 36) */ default_Handler,
    /* 53: reserved           ( 37) */ default_Handler,
    /* 54: reserved           ( 38) */ default_Handler,
    /* 55: reserved           ( 39) */ default_Handler,
    /* 56: reserved           ( 40) */ default_Handler,
    /* 57: reserved           ( 41) */ default_Handler,
    /* 58: reserved           ( 42) */ default_Handler,
    /* 59: reserved           ( 43) */ default_Handler,
    /* 60: reserved           ( 44) */ default_Handler,
    /* 61: reserved           ( 45) */ default_Handler,
    /* 62: reserved           ( 46) */ default_Handler,
    /* 63: reserved           ( 47) */ default_Handler,
    
};
void default_Handler(void)
{
    for(;;) {
        ;
    }
    
}

