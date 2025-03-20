/*
  automatically created main.c
  created at: 2025-03-17 01:14:53
  created from mbsp.json
*/

#include <hal/digital_out.h>
#include <hal/time_ms.h>
#include <stdbool.h>

bool on = false;
uint32_t cnt = 0;


int main(void)
{
    uint32_t last_time = 0;
    for(;;)
    {
        if(0 == cnt)
        {
            if(true == on)
            {
                on = false;
                cnt = 400;
                green_led_off();
                
            }
            else
            {
                on = true;
                cnt = 600;
                green_led_on();
                
            }
            
        }
        cnt--;
        
        while(last_time == ms_since_boot
        )
        {
            __asm volatile ("wfe");
            
        }
        last_time = ms_since_boot
        ;
        
    }
    
}

