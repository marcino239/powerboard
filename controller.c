/*
Copyright (c) 2014 - toxic239

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
#include <msp430.h>

#define V_IN		INCH_0	// A0

#define LED0		BIT1	// P1
#define LED1		BIT2
#define LED2		BIT3
#define LED3		BIT4
#define LED_WARNING	BIT5
#define BUZZER		BIT6
#define PWR_MOSFET	BIT7	// P2

// LEDs and BUZZER are on P1
// PWR_MOSFET are on P2

// cut off voltages for 3S LiPo pack
#define VOLT_0  (4.1 * 10 * 3)
#define VOLT_1  (3.8 * 10 * 3)
#define VOLT_2  (3.5 * 10 * 3)
#define VOLT_3  (3.2 * 10 * 3)
#define VOLT_4  (3.1 * 10 * 3)
#define VOLT_5  (3.0 * 10 * 3)


void setLeds( int led )
{
  // only 1 led lit at a time
  P1OUT |= LED0 + LED1 + LED2 + LED3 + LED_WARNING;  // active state low
  P1OUT &= ~led;
}

// outputs voltage converted to volts * 10
int convertAdcToVolt( int vtemp ) {
  return 1000;
}

int main(void)
{
  int vtemp, v, flag;

  WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer

  // configure ports
  P1DIR = LED0 + LED1 + LED2 + LED3 + LED_WARNING + BUZZER;
  P1OUT = LED0 + LED1 + LED2 + LED3 + LED_WARNING;  // active state low

  P2DIR = PWR_MOSFET;
  P2OUT = PWR_MOSFET;  //active state low

  // set the ADC
  ADC10CTL0 = ADC10SHT_3 + ADC10ON + ADC10IE;
  ADC10CTL1 = V_IN;                         // input
  ADC10AE0 |= BIT0;                         // A0 

  flag = 1;
  while( flag )
  {
    ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
    __bis_SR_register(CPUOFF + GIE);        // LPM0, ADC10_ISR will force exit

    vtemp = ADC10MEM;
    v = convertAdcToVolt( vtemp );

    // check if ok to switch power on
    if( v > VOLT_3 )
      P2OUT &= ~PWR_MOSFET;

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
  }

  // power off
  P2OUT |= PWR_MOSFET;

  // this loop has led pulsating and beeper on
  flag = 1;
  while( flag ) {
  }

  // this loop has led pulsating only
  flag = 1;
  while( flag ) {
  }

  // here we switch off the cpu and leds
  P1OUT = LED0 + LED1 + LED2 + LED3 + LED_WARNING;  // active state low
  __bis_SR_register( CPUOFF );
}

// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
  __bic_SR_register_on_exit( CPUOFF );        // Clear CPUOFF bit from 0(SR)
}
