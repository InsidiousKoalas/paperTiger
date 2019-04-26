#include <msp430.h> 
//#include "ledFunks.h"
//#include "vuFunks.h"

/*
 * main.c
 */
#define NUM_SAMPS	1024

int sample, sampFlag, ndx;
unsigned long int avg;
volatile char eosFlag = 0;


int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

/*
 * 	-----------------	LED board initialization  ------------------------
 */

//    P3DIR |= 0x03;		// Set P6.0 & P6.1 as outs
//    P3DIR &= ~0xFC;		// init other pins as ins
//
    volatile int i, sListFlag, coms[3] = {0x76, 0x00, 0x17};
//    sListFlag = ledInit(coms);
//    if(sListFlag)(P1OUT |= BIT0);	// indicate error
//    else(P1OUT &= ~BIT0);


    // initialize display to dashes
//    int disp[4];
//    disp[0] = 0x81;
//    disp[1] = 0x81;
//    disp[2] = 0x81;
//    disp[3] = 0x81;
//
//    ledWrite(0x76,disp);
//    __delay_cycles(0xffff);
//
//    for(i=0; i<4; i++){
//    	disp[i] = 0x80;
//    }
//
//    ledWrite(0x76,disp);


/*
 * 	-----------------	ADC initialization  ------------------------
 */

    ADC12CTL0 = ADC12SHT02 + ADC12ON;         // Sampling time, ADC12 on
    ADC12MCTL0 = ADC12INCH_1;					// multiplex ADC to P6.1
    ADC12CTL1 = ADC12SHP;                     // Use sampling timer
    ADC12IE = 0x01;                           // Enable interrupt
    ADC12CTL0 |= ADC12ENC;					  // Enable encoding

    P1DIR |= BIT0;



//    ADC12CTL0 |= ADC12SC;
//    __bis_SR_register(GIE);        // LPM0 with interrupts enabled (CPUOFF waits for ADC)

/*
 * 	-----------------	Timer initialization  ------------------------
 */

    TA0CCTL0 = CCIE;
    TA0CCR0 = 50;
    TA0CTL = TASSEL_2 + MC_1;	// SMCLK, upmode

/*
 *  ----------------   UART initialization  --------------------------
 */
    P4SEL |= BIT4|BIT5;                       // P3.3,4 = USCI_A0 TXD/RXD
    UCA1CTL1 |= UCSWRST;                      // **Put state machine in reset**
    UCA1CTL1 |= UCSSEL_2;                     // SMCLK
    UCA1BR0 = 9;                              // 1MHz 115200 (see User's Guide)
    UCA1BR1 = 0;                              // 1MHz 115200
    UCA1MCTL |= UCBRS_1 + UCBRF_0;            // Modulation UCBRSx=1, UCBRFx=0
    UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
    UCA1IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt


/*
 * 	----------------   Other initialization --------------------------
 */

    unsigned int sampAry[NUM_SAMPS] = {0};
//    unsigned int avg=0;
    P4DIR |= BIT7;
    __bis_SR_register(GIE);        // enable interrupts



    while(1){

    	// if there is an ADC sample pending
    	if(sampFlag == 1){

    		sampAry[ndx] = sample;		// copy sample val into array
    		ndx++;						// index for next sample
    		if(ndx>NUM_SAMPS)(ndx=0);

//
//    		for(i=0;i<2;i++){
//    			UCA1TXBUF = (sampAry[ndx]&0xFF00)+'0';
//    			sampAry[ndx]<<=1;
//    		}
    		UCA1TXBUF = 'R';
    		UCA1TXBUF = '\n';
    		UCA1TXBUF = '\r';


//    		P4OUT ^= BIT7;		// test if running
//    		TA0CCTL0 = CCIE;	// restart interrupts
//    		__bis_SR_register(GIE);		// reenable globals

    		while (!(UCA1IFG&UCTXIFG));
    		TA0CCTL0 |= CCIE;
//    		eosFlag &= 0x01;	// set End of String flag
    		sampFlag = 0;	// clear sample flag
    	}


    }


	return 0;
}


// ADC10 interrupt service routine
#pragma vector=ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{
	sample = ADC12MEM0;					// clrs IFG
	__bic_SR_register_on_exit(CPUOFF + GIE);	// comment if troublesome
}


#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{

	__bic_SR_register(GIE);
	ADC12CTL0 |= ADC12SC;
	P1OUT ^= BIT0;
	TA0CCTL0 &= ~CCIE;						// disable timer interrupt to prevent second trigger
	sampFlag = 1;
	__bis_SR_register(CPUOFF + GIE);        // LPM0 with interrupts enabled (CPUOFF waits for ADC)

}

// Echo back RXed character, confirm TX buffer is ready first
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
  switch(__even_in_range(UCA1IV,4))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:break;                               // Vector 2 - RXIFG
//    while (!(UCA0IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
//    UCA0TXBUF = UCA0RXBUF;                  // TX -> RXed character
//    break;
  case 4:									  // Vector 4 - TXIFG


	  while (!(UCA1IFG&UCTXIFG));
//	  __bic_SR_register(GIE);
//	  __bic_SR_register(CPUOFF);
//	  __bis_SR_register_on_exit(GIE);
//	  if(eosFlag&0x01 == 1){
//		  TA0CCTL0 |= CCIE;			//
//		  eosFlag ^= 0x01;				// clr eosFlag
//	  }
	  break;

  default: break;
  }
}
