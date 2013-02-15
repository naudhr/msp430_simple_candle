/*
 *               MSP430G2131
 *          .  -----------------
 *         /|\|                 |
 *          | |                 |
 *          --|RST          P1.2|--> PWM ouput
 *            |                 |
 *
 * The LED cathode is connected to the p1.2 through a current-limiting resistor.
 */

#include "msp430.h"
#include "stdint.h"

#define PWM_WINDOW      128 // ~.01sec at vlo
#define DEF_BRIGHTNESS   55 // must be < than UPD_INTERVAL

#define PWM_PIN             BIT2

void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;

    BCSCTL3 |= LFXT1S_2;  // ACLK = VLO

    //P1.2 is timerA.1 PWM output
    P1DIR = PWM_PIN;
    P1SEL = PWM_PIN;

    // making 2 timer signals as described at the User Guide p.368
    TACCR0 = PWM_WINDOW;
    TACCR1 = 0;
    TACCTL1 = OUTMOD_3; //inverting PWM on channel 1
    TACCTL0 = CCIE;
    TACTL = TASSEL_1 | MC_1 | TACLR; // continous, sourcing ACLK

//    WDTCTL = WDTPW | WDTCNTCL | WDTSSEL | WDTIS1;

    _BIS_SR(LPM3_bits + GIE);
}

//Generating random numbers with 16-bit LFSR
uint8_t LFSR_Random(void)
{
    static uint16_t LFSR=0xBADC;
    uint8_t out_byte=0;
    uint8_t i;

    for (i=0; i<8; i++)
    {
        LFSR=(LFSR << 1) | (((LFSR >> 15) ^ (LFSR >> 13) ^ (LFSR >> 12) ^ (LFSR >> 10) ^ (LFSR >> 0)) & 0x0001);
        out_byte|=((LFSR & 0x8000) >> 8) >> i;
    }
    return out_byte;
}

#define nth_bit(x,n) ((x>>n) & 1)

__attribute__((interrupt (TIMERA0_VECTOR))) void TA_0_handler(void)
{
    static uint8_t pwm_target = DEF_BRIGHTNESS;
    static uint8_t set_def = 1;
    static uint16_t cnt = 0;
    static uint16_t n_pwm_windows_to_change_brightness = 0;

//    WDTCTL |= WDTPW | WDTCNTCL;

    // the "candling" algorithm is a borrowed one of YS from radiokot.ru and we.easyelectronics.ru, ysgmbx0@gmail.com
    if(++cnt >= n_pwm_windows_to_change_brightness)
    {
        cnt = 0;
        uint16_t pwm_value = TACCR1;
        if(pwm_value<pwm_target)
            pwm_value++;
        if(pwm_value>pwm_target)
            pwm_value--;
        if(pwm_value==pwm_target)
        {
            set_def = !set_def;
            pwm_target = set_def ? DEF_BRIGHTNESS : LFSR_Random()>>1; // hardcoded to match PWM_WINDOW
        }
        const uint8_t rnd = LFSR_Random();
        n_pwm_windows_to_change_brightness = 1 + (nth_bit(rnd,0) + nth_bit(rnd,1) + nth_bit(rnd,2) + nth_bit(rnd,3) +
                                                  nth_bit(rnd,4) + nth_bit(rnd,5) + nth_bit(rnd,6) + nth_bit(rnd,7)) & 1;
        // 1 <= n_pwm_windows_to_change_brightness <= 2 : 0.01 - 0.03 sec at unstable vlo
        TACCR1 = pwm_value;
    }
}

