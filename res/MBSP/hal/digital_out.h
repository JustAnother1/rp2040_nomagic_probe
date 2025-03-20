/*
  automatically created hal/digital_out.h
  created at: 2025-03-17 01:14:53
  created from mbsp.json
*/

#include <hal/hw/SIO.h>

static inline void green_led_on(void)
{
    SIO->GPIO_OUT_SET = 1<<25;
}
static inline void green_led_off(void)
{
    SIO->GPIO_OUT_CLR = 1<<25;
}

