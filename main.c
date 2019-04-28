#include <msp430.h> 
//#include "ledFunks.h"
//#include "vuFunks.h"

/*
 * main.c
 */
#define NUM_SAMPS	1024


unsigned int *findFreq(int*, unsigned int);

int sample, sampFlag, pushData, ndx;
unsigned long int avg;
volatile char eosFlag = 0;

/*
 * Algorithm:
 *
 *
 * 		A1:		Trigger timer interrupt (25 kHz)
 * 					- Start ADC Conversion
 * 					- Toggle indicator LED
 * 					- Set sample flag
 * 					- Turn off CPU for remained of ADC conversion
 *
 * 		A2:		ADC interrupt
 * 					- write ADC12MEM to sample
 * 		A3:		Main loop
 * 					- if(sampFlag == 1) then
 * 						=> sampAry[ndx] = sample
 * 						=> if(ndx == end of array) then
 * 							- disable all interrupts
 * 							- f = find_freq();
 * 							- ndx = 0;
 * 							- reenable UART interrupt
 * 							- print note over UART
 * 		A4: 	find_freq()
 * 					- avg(sampAry)
 * 					- sampAry = sampAry - avg
 * 					- make bitstream arry
 * 						=> if(sampAry[i] >= 0) then
 * 							- streamAry[i] = 1;
 * 						=>	else
 * 							- streamAry[i] = 0;
 * 					- Autocorrelate bistream arry
 * 						=>
 *
 *
 *
 */



int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

/*
 * 	-----------------	Variable and Pin initialization  ------------------------
 */


    volatile int i, j,sListFlag, coms[3] = {0x76, 0x00, 0x17};


    // Button to push data to computer
    P1DIR &= ~BIT1;
    P1REN |= BIT1;
    P1OUT |= BIT1;


/*
 * 	-----------------	ADC initialization  ------------------------
 */

    ADC12CTL0 = ADC12SHT02 + ADC12ON;         // Sampling time, ADC12 on
    ADC12MCTL0 = ADC12INCH_1;					// multiplex ADC to P6.1
    ADC12CTL1 = ADC12SHP;                     // Use sampling timer
    ADC12IE = 0x01;                           // Enable interrupt
    ADC12CTL0 |= ADC12ENC;					  // Enable encoding

    P1DIR |= BIT0;


    // test ADC
//    ADC12CTL0 |= ADC12SC;
//    __bis_SR_register(GIE);        // LPM0 with interrupts enabled (CPUOFF waits for ADC)

/*
 * 	-----------------	Timer initialization  ------------------------
 */

//    TA0CCTL0 = CCIE;
    TA0CCR0 = 40;

    unsigned int fs = 1000/TA0CCR0*1000;

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
    UCA1IE |= UCRXIE;                         // Enable USCI_A0 TX interrupt


    // send init over UART
    UCA1TXBUF = 'o';
    __delay_cycles(0xFFF);
    UCA1TXBUF = 'n';
    __delay_cycles(0xFFF);
    UCA1TXBUF = '\r';
    __delay_cycles(0xFFF);
    UCA1TXBUF = '\n';
    __delay_cycles(0xFFF);
    UCA1TXBUF = '\r';
    __delay_cycles(0xFFF);
    UCA1TXBUF = '\n';
    unsigned char tx[4];


/*
 * 	----------------   Other initialization --------------------------
 */

    int sampAry[NUM_SAMPS] = {0};
//    unsigned int avg=0;
    P4DIR |= BIT7;

    TA0CCTL0 = CCIE;
    __bis_SR_register(GIE);        // enable interrupts


    unsigned int *streamPtr;
    float freq;


    while(1){

    	// if there is an ADC sample pending
    	if(sampFlag == 1){

    		sampAry[ndx] = sample;		// copy sample val into array
    		ndx++;						// index for next sample
    		if(ndx>NUM_SAMPS){
    			streamPtr = findFreq(sampAry, fs);

//    			for(i=0; i<64; i++){
//    				for(j=0; j<16; j++){
//    					UCA1TXBUF = streamPtr[i]&0x01+'0';
////    					UCA1TXBUF = 'g';
//    					streamPtr[i] >>= 0x01;
//    					__delay_cycles(0xFFF);
//    					UCA1TXBUF = '\r';
//    					__delay_cycles(0xFFF);
//    					UCA1TXBUF = '\n';
//    					__delay_cycles(0xFFF);
//    				}
//    			}

    		}

    		P4OUT ^= BIT7;		// test if running

    		__bis_SR_register(GIE);		// reenable globals
    		TA0CCTL0 |= CCIE;
//    		eosFlag = 1;
    		sampFlag = 0;	// clear sample flag
    	}






    	// TX data
    	if(!(P1IN & BIT1)){
    		TA0CCTL0 &= ~CCIE;
    		__bic_SR_register(GIE);
    		UCA1IE |= UCRXIE;


    		for(i=ndx+1; i != ndx; i++){
    			if(i>=NUM_SAMPS)(i=0);

    			tx[3] = (sampAry[i]%10)+'0';
    			sampAry[i] /= 10;
    			tx[2] = (sampAry[i]%10)+'0';
    			sampAry[i] /= 10;
    			tx[1] = (sampAry[i]%10)+'0';
    			sampAry[i] /= 10;
    			tx[0] = (sampAry[i]%10)+'0';

    			for(j=0;j<4;j++){
    				UCA1TXBUF = tx[j];
    				__delay_cycles(0xFFF);
    			}


    			UCA1TXBUF = '\r';
    			__delay_cycles(0xFFF);
    			UCA1TXBUF = '\n';
    			__delay_cycles(0xFFF);

    		}

    		pushData = 0;
    		UCA1TXBUF = '\r';
    		__delay_cycles(0xFFF);
    		UCA1TXBUF = '\n';
    		__delay_cycles(0xFFF);
    		UCA1TXBUF = '\r';
    		__delay_cycles(0xFFF);
    		UCA1TXBUF = '\n';

//    		for(i=0; i<(8*sizeof(int)); i++){
//    			UCA1TXBUF = *()
//    		}
//
//    		eosFlag = 0;

    		TA0CCTL0 |= CCIE;
    		__bis_SR_register(GIE);
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
	while(!(UCA1IFG & UCTXIFG));		// wait for TX buffer
}



/*
 * 		A4: 	find_freq()
 * 					- avg(sampAry)
 * 					- sampAry = sampAry - avg
 * 					- make bitstream arry
 * 						=> if(sampAry[i] >= 0) then
 * 							- streamAry[i] = 1;
 * 						=>	else
 * 							- streamAry[i] = 0;
 * 					- Autocorrelate bistream arry
 * 						=>
 */

unsigned int* findFreq(int* sampAry, unsigned int fs){
	int i,j,sizeInt, leng;

	sizeInt = 8*sizeof(int);

	leng = NUM_SAMPS/(sizeInt);

	unsigned int stream[64];
	int avg = 2047;			// assumes microphone is in middle (may need adjustment)


	// store stream as int array for later XOR
	for(i=0; i<leng; i++){
		// store each bit
		for(j=0; j<sizeInt; j++){
			stream[i]<<=1;
			if((sampAry[i+j]-avg)>=0){
				stream[i] |= 0x01;		// modify only LSB
			}
			else{
				stream[i] &= ~0x01;		// modify only LSB
			}
		}
	}

	// autocorrelate
	int acorr[NUM_SAMPS] = {0};
	int indices = 0;

	// for every possible shift
	for(j=0;j<1024; j++){
		indices = j/sizeInt

		// pass in only necessary number of ints
		for(i=0; i<indices; i++){
			one = stream[i] ^ stream[NUM_SAMPS];
			sum =
			aCorr[j] =

		}
	}


	return stream;
}
































