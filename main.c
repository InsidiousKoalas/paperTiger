#include <msp430.h> 
//#include "ledFunks.h"
//#include "vuFunks.h"

/*
 * main.c
 */
#define NUM_SAMPS	1024
#define UART_W8		0x3FFF

unsigned int *findFreq(int*, unsigned int);
int zeroX(int*, unsigned int);
void initClockTo16MHz();

int sample, sampFlag, pushData, ndx;
unsigned long int avg;
volatile char eosFlag = 0;

/*
 * P3.3 is TX
 *
 *
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


    volatile int i, j,sListFlag;


    // Button to push data to computer
    P1DIR &= ~BIT1;
    P1REN |= BIT1;
    P1OUT |= BIT1;


/*
 * 	-----------------	ADC initialization  ------------------------
 */

    UCSCTL0 = DCO0 | DCO1 | DCO2 | DCO3 | DCO4;
    initClockTo16MHz();
//    BCSCTL1 = RSEL0 + RSEL1 + RSEL2;
//    UCSCTL1 =
//    DCOCTL0 = DCO0 + DCO1 + DCO2;
    ADC12CTL0 = ADC12SHT02 + ADC12ON;         // Sampling time, ADC12 on
    ADC12MCTL0 = ADC12INCH_1;					// multiplex ADC to P6.1
    ADC12CTL1 = ADC12SHP | ADC12CONSEQ_0 | ADC12SSEL_0;                     // Use sampling timer
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
    TA0CCR0 = 20;

    unsigned int fs = 864/TA0CCR0*1000;		// tuned to produce accurate notes

    TA0CTL = TASSEL_2 + MC_1;	// SMCLK, upmode

/*
 *  ----------------   UART initialization  --------------------------
 */
    P4SEL |= BIT4|BIT5;                       // P3.3,4 = USCI_A0 TXD/RXD
    UCA1CTL1 |= UCSWRST;                      // **Put state machine in reset**
    UCA1CTL1 |= UCSSEL_2;                     // SMCLK
    UCA1BR0 = 139;                              // 16MHz, 115200 (see User's Guide)
    UCA1BR1 = 0;                              // 1MHz 115200
    UCA1MCTL |= UCBRS_1 + UCBRF_0;            // Modulation UCBRSx=1, UCBRFx=0
    UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
    UCA1IE |= UCRXIE;                         // Enable USCI_A0 TX interrupt


    // send init over UART
    __bis_SR_register(GIE);

    UCA1TXBUF = 'o';
    __delay_cycles(UART_W8);
    UCA1TXBUF = 'n';;
    __delay_cycles(UART_W8);
    UCA1TXBUF = '\r';
    __delay_cycles(UART_W8);
    UCA1TXBUF = '\n';
    __delay_cycles(UART_W8);
    UCA1TXBUF = '\r';
    __delay_cycles(UART_W8);
    UCA1TXBUF = '\n';
    unsigned char tx[4], tx2[5];


/*
 * 	----------------   Other initialization --------------------------
 */

    int sampAry[NUM_SAMPS] = {0};
//    unsigned int avg=0;
    P4DIR |= BIT7;

    TA0CCTL0 = CCIE;
    __bis_SR_register(GIE);        // enable interrupts


    unsigned int *streamPtr;
    int freq;


    while(1){

    	// if there is an ADC sample pending
    	if(sampFlag == 1){

    		sampAry[ndx] = sample;		// copy sample val into array
    		ndx++;						// index for next sample
    		if(ndx>NUM_SAMPS){
    			ndx = 0;
    			//    			streamPtr = findFreq(sampAry, fs);
    			freq = zeroX(sampAry, fs);

    			tx2[4] = (freq%10)+'0';
    			freq /= 10;
    			tx2[3] = (freq%10)+'0';
    			freq /= 10;
    			tx2[2] = (freq%10)+'0';
    			freq /= 10;
    			tx2[1] = (freq%10)+'0';
//    			freq /= 10;
//    			tx2[0] = (freq%10)+'0';

    			for(j=1;j<5;j++){
    				UCA1TXBUF = tx2[j];
    				__delay_cycles(UART_W8);
    			}
    			UCA1TXBUF = '\r';
    			__delay_cycles(UART_W8);
    			UCA1TXBUF = '\n';
    			__delay_cycles(UART_W8);


    			//    			for(i=0; i<64; i++){
    			//    				for(j=0; j<16; j++){
    			//    					UCA1TXBUF = streamPtr[i]&0x01+'0';
    			////    					UCA1TXBUF = 'g';
    			//    					streamPtr[i] >>= 0x01;
    			//    					__delay_cycles(UART_W8);
    			//    					UCA1TXBUF = '\r';
    			//    					__delay_cycles(UART_W8);
    			//    					UCA1TXBUF = '\n';
    			//    					__delay_cycles(UART_W8);
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
//    		freq = zeroX(sampAry, fs);
//    		streamPtr = findFreq(sampAry, fs);
//
//    		tx2[4] = (freq%10)+'0';
//    		freq /= 10;
//    		tx2[3] = (freq%10)+'0';
//    		freq /= 10;
//    		tx2[2] = (freq%10)+'0';
//    		freq /= 10;
//    		tx2[1] = (freq%10)+'0';
//    		freq /= 10;
//    		tx2[0] = (freq%10)+'0';
//
//    		for(j=0;j<5;j++){
//    			UCA1TXBUF = tx2[j];
//    			__delay_cycles(UART_W8);
//    		}

//    		 LPF data
//    		for(i=0; i < NUM_SAMPS; i++){
////    			if(i>=NUM_SAMPS)(i=0);
//
//    			tx[3] = (streamPtr[i]%10)+'0';
//    			streamPtr[i] /= 10;
//    			tx[2] = (streamPtr[i]%10)+'0';
//    			streamPtr[i] /= 10;
//    			tx[1] = (streamPtr[i]%10)+'0';
//    			streamPtr[i] /= 10;
//    			tx[0] = streamPtr[i]+'0';
//
//    			//    			if(i>=64)(i=0);
//
//    			UCA1TXBUF = tx[0];
//    			__delay_cycles(UART_W8);
//    			//    			for(j=0;j<1;j++){
////    				UCA1TXBUF = tx[j];
////    				__delay_cycles(UART_W8);
////    			}
//
//
//    			UCA1TXBUF = '\r';
//    			__delay_cycles(UART_W8);
//    			UCA1TXBUF = '\n';
//    			__delay_cycles(UART_W8);
//
//    		}

    		UCA1TXBUF = '\r';
    		__delay_cycles(UART_W8);
    		UCA1TXBUF = '\n';
    		__delay_cycles(UART_W8);
    		UCA1TXBUF = '\r';
    		__delay_cycles(UART_W8);
    		UCA1TXBUF = '\n';
    		__delay_cycles(UART_W8);

    		for(i=0; i < NUM_SAMPS; i++){
//    			if(i>=NUM_SAMPS)(i=0);

    			tx[3] = (sampAry[i]%10)+'0';
    			sampAry[i] /= 10;
    			tx[2] = (sampAry[i]%10)+'0';
    			sampAry[i] /= 10;
    			tx[1] = (sampAry[i]%10)+'0';
    			sampAry[i] /= 10;
    			tx[0] = (sampAry[i]%10)+'0';

//    			if(i>=64)(i=0);

        		for(j=0;j<4;j++){
        			UCA1TXBUF = tx[j];
        			__delay_cycles(UART_W8);
        		}


    			UCA1TXBUF = '\r';
    			__delay_cycles(UART_W8);
    			UCA1TXBUF = '\n';
    			__delay_cycles(UART_W8);

    		}

    		tx[3] = (freq%10)+'0';
    		freq /= 10;
    		tx[2] = (freq%10)+'0';
    		freq /= 10;
    		tx[1] = (freq%10)+'0';
    		freq /= 10;
    		tx[0] = (freq%10)+'0';


    		for(j=0;j<4;j++){
    			UCA1TXBUF = tx[j];
    			__delay_cycles(UART_W8);
    		}


    		pushData = 0;
    		UCA1TXBUF = '\r';
    		__delay_cycles(UART_W8);
    		UCA1TXBUF = '\n';
    		__delay_cycles(UART_W8);
    		UCA1TXBUF = '\r';
    		__delay_cycles(UART_W8);
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
//	sampFlag = 1;
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
	case 2:                                   // Vector 2 - RXIFG
		while (!(UCA1IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
		//    UCA0TXBUF = UCA0RXBUF;                  // TX -> RXed character
		__bic_SR_register_on_exit(CPUOFF);
		break;
	case 4:break;                             // Vector 4 - TXIFG
	default: break;
	}
}




/*
 * XOR Acorr
 *
 */
unsigned int * findFreq(int * sampAry, unsigned int fs){
	int i, j, avg = 2056;
	char pos[NUM_SAMPS];
	int aCorr[NUM_SAMPS];		// array of positive locations

	for(i=0; i<NUM_SAMPS; i++){
		if((sampAry[i]-avg)&0x8000)(pos[i] = 0);		// array is negative
		else(pos[i] = 1);								// array is pos
	}

	// autocorrelate positives array
	for(i=0; i<NUM_SAMPS; i++){
		aCorr[i] = 0;
		// for 0 to end of overlap, find correlation
		for(j=0; j<i; j++){
			aCorr[i] += pos[j]^pos[i];
		}
	}

//	int diff[2] = { 0 };
//
//	for(i=0; i<NUM_SAMPS; i++){
//		if(aCorr[i]<diff[0]){
//			diff[1] = diff[0];
//			diff[0] = aCorr[i];
//
//		}
//		dist++;
//	}



	return aCorr;
}




/*
 *
 * zero cross frequency function
 *
 *
 */
int zeroX(int *sampAry, unsigned int fs){
	int avg = 2056, i, j;
	long int sum=0;
	int period=0, periodFlg=0, num=12;

	int stream[NUM_SAMPS], pos[NUM_SAMPS];


	// LPF
	pos[0] = 0;
	int td[7]={0}, idx=-1;

	for(i=1; i<NUM_SAMPS-num; i++){

		//LPF
		for(j=0; j<num; j++){
			sum = sum+sampAry[i+j];
		}

		stream[i] = sum/num;
//		stream[i] = sampAry[i];
		sum=0;
		if((stream[i]-avg) & 0x8000)(pos[i] = 0);
		else(pos[i] = 1);

		if(pos[i-1] == 0 && pos[i] == 1){
			periodFlg = 1;
			idx++;
			if(idx>6){
				break;
			}
		}

		if(periodFlg == 1){
			td[idx]++;
		}
	}


	sum=0;
	for(i=0; i<6; i++){
		sum += td[i];
	}
	sum /= 6;



	int freq;

	freq = fs/sum;


	return freq;
}



void initClockTo16MHz()
{
    UCSCTL3 |= SELREF_2;                      // Set DCO FLL reference = REFO
    UCSCTL4 |= SELA_2;                        // Set ACLK = REFO
    __bis_SR_register(SCG0);                  // Disable the FLL control loop
    UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
    UCSCTL1 = DCORSEL_5;                      // Select DCO range 16MHz operation
    UCSCTL2 = FLLD_0 + 487;                   // Set DCO Multiplier for 16MHz
                                              // (N + 1) * FLLRef = Fdco
                                              // (487 + 1) * 32768 = 16MHz
                                              // Set FLL Div = fDCOCLK
    __bic_SR_register(SCG0);                  // Enable the FLL control loop

    // Worst-case settling time for the DCO when the DCO range bits have been
    // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
    // UG for optimization.
    // 32 x 32 x 16 MHz / 32,768 Hz = 500000 = MCLK cycles for DCO to settle
    __delay_cycles(500000);//
    // Loop until XT1,XT2 & DCO fault flag is cleared
    do
    {
        UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG); // Clear XT2,XT1,DCO fault flags
        SFRIFG1 &= ~OFIFG;                          // Clear fault flags
    }while (SFRIFG1&OFIFG);                         // Test oscillator fault flag
}
