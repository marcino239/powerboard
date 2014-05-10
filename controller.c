/*
Copyright (c) 2014 - marcino239

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/


#include <msp430g2231.h>
#include <stdint.h>


#define V_IN		INCH_0	// A0
#define A0              BIT0

#define LED0		BIT2	// P1
#define LED1		BIT3
#define LED2		BIT4
#define LED3		BIT5
#define LED_WARNING	BIT6
#define BUZZER		BIT1
#define PWR_MOSFET	BIT6	// P2

// LEDs and BUZZER are on P1
// PWR_MOSFET are on P2

// cut off voltages for 3S LiPo pack
#define VOLT_0  (41 * 3)
#define VOLT_1  (38 * 3)  //38
#define VOLT_2  (35 * 3)  //35
#define VOLT_3  (32 * 3)  //32
#define VOLT_4  (31 * 3)  //31
#define VOLT_5  (30 * 3)  //30

// equation: v = ADC * 10 * 2.5 * 11.7 / 1023 / 1.811 * 65536 / 65536
#define VOLT_MULT	10347


void setLeds( int led )
{
  // only 1 led lit at a time
  P1OUT |= LED0 + LED1 + LED2 + LED3 + LED_WARNING;  // active state low
  P1OUT &= ~led;
}

// sleeps in LPM3 state
void sleep( count ) {
//  while( --count > 0 )
//    LPM3;

  int i;
  while( --count > 0 )
    for( i=0; i<50; i++ )
      nop();
}


int main(void)
{
  uint16_t v, flag, k;

  // we will be using watchdog as a sleep counter
  BCSCTL1 |= DIVA_1;                        // ACLK/2
  BCSCTL3 |= LFXT1S_2;                      // ACLK = VLO
  WDTCTL = WDT_ADLY_250;                    // Interval timer
  IE1 |= WDTIE;                             // Enable WDT interrupt

  // configure ports
  P1DIR = LED0 + LED1 + LED2 + LED3 + LED_WARNING + BUZZER;
  P1OUT = LED0 + LED1 + LED2 + LED3 + LED_WARNING;  // active state low

  P2DIR = PWR_MOSFET;
  P2SEL = 0;
  P2OUT = 0;                                //active state high

  // set the ADC
  // Vref=Ref, 64x clock sampling, 2.5V reference, ref on, adc on, interrupts enabled
  ADC10CTL0 = SREF_1 + ADC10SHT_3 + REF2_5V + REFON + ADC10ON + ADC10IE;
//  ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE;
  ADC10CTL1 = V_IN;                         // input
  ADC10AE0  = A0;

  flag = 1;
  while( flag )
  {
    ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
    __bis_SR_register(CPUOFF + GIE);        // LPM0, ADC10_ISR will force exit

    v = ((uint32_t)ADC10MEM  * VOLT_MULT) >> 16;

    // check if ok to switch power on
    if( v > VOLT_3 )
      P2OUT |= PWR_MOSFET;

    // check voltage
    if( v > VOLT_0 )
      setLeds( LED0 );
    else if( VOLT_1 <= v && v < VOLT_0 )
      setLeds( LED1 );
    else if( VOLT_2 <= v && v < VOLT_1 )
      setLeds( LED2 );
    else if( VOLT_3 <= v && v < VOLT_2 )
      setLeds( LED3 );
    else if( v < VOLT_3 ) {
      setLeds( LED_WARNING );
      flag = 0;
    }

    sleep( 4 );  // sleep 1s
  }

  // power off
  P2OUT &= ~PWR_MOSFET;

  // this loop has led pulsating and beeper on
  flag = 1;
  setLeds( LED_WARNING );
  k = 0;

  while( flag ) {
    ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
    __bis_SR_register(CPUOFF + GIE);        // LPM0, ADC10_ISR will force exit

    v = ((uint32_t)ADC10MEM  * VOLT_MULT) >> 16;

    // sleep 0.5ms
    sleep( 1 );
    if( k < 1024 ) {
      P1OUT ^= BUZZER;
      P1OUT |= LED_WARNING;
    } else {
      P1OUT &= ~BUZZER;
      P1OUT &= ~LED_WARNING;
    }
    k = (k+1) & 2047;



    // check if ok to switch buzzer off
    if( v < VOLT_4 )
      flag = 0;
  }

  // this loop has led pulsating only
  flag = 1;
  setLeds( LED_WARNING );

  while( flag ) {
    ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
    __bis_SR_register(CPUOFF + GIE);        // LPM0, ADC10_ISR will force exit

    v = ((uint32_t)ADC10MEM  * VOLT_MULT) >> 16;

    // sleep 0.5ms
    sleep( 80 );
    P1OUT ^= LED_WARNING;

    // check if ok to go to sleep
    if( v < VOLT_5 )
      flag = 0;
  }

  // here we switch off the cpu and leds
  P1OUT = LED0 + LED1 + LED2 + LED3 + LED_WARNING;  // active state low
  WDTCTL = WDTPW + WDTHOLD;                  // Stop Watchdog Timer
//  __bis_SR_register( CPUOFF );
  while( 1 )
    LPM3;
}

// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
  __bic_SR_register_on_exit( CPUOFF );        // Clear CPUOFF bit from 0(SR)
}

#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer (void)
{
  _BIC_SR_IRQ( LPM3_bits );                   // Clear LPM3 bits from 0(SR)
}
