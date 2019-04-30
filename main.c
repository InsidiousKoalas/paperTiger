#include <msp430.h> 

/*
 * main.c
 */
#define NUM_SAMPS	1024
#define UART_W8		0x3FFF

//unsigned int *findFreq(int*, unsigned int);
int* zeroX(int*, unsigned int);
int vuMeter(int*);
void xFerData(unsigned int*);
void initClockTo16MHz();

int sample, sampFlag, pushData, ndx;
unsigned long int avg;
volatile char sendFlag = 0;

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

    // Reset constituent MSP430 display controller
    P1DIR |= BIT3;
    P1OUT |= BIT3;
    __delay_cycles(0xFFFF);
    P1OUT &= ~BIT3;
    __delay_cycles(0xFFFF);
    P1OUT |= BIT3;


/*
 * 	-----------------	ADC initialization  ------------------------
 */

    initClockTo16MHz();
    ADC12CTL0 = ADC12SHT02 + ADC12ON;           				// Sampling time, ADC12 on
    ADC12MCTL0 = ADC12INCH_1;									// multiplex ADC to P6.1
    ADC12CTL1 = ADC12SHP | ADC12CONSEQ_0 | ADC12SSEL_0;         // Use sampling timer
    ADC12IE = 0x01;                             				// Enable interrupt
    ADC12CTL0 |= ADC12ENC;					    				// Enable encoding



    P1DIR |= BIT0;

/*
 * 	-----------------	Timer initialization  ------------------------
 */

    TA0CCR0 = 400;

    unsigned int fs = 1600/TA0CCR0*10000+300;		// 	sampling frequency; 16MHz/TA0CCR0 + correction factor

    TA0CTL = TASSEL_2 + MC_1;	// SMCLK, upmode

/*
 *  ----------------   UART0 initialization  --------------------------
 */
    P3SEL |= BIT3+BIT4;                       // P3.3,4 = USCI_A0 TXD/RXD
    UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
    UCA0CTL1 |= UCSSEL_2;                     // SMCLK
    UCA0BR0 = 139;                              // 1MHz 115200 (see User's Guide)
    UCA0BR1 = 0;                              // 1MHz 115200
    UCA0MCTL |= UCBRS_1 + UCBRF_0;            // Modulation UCBRSx=1, UCBRFx=0
    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
    UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt


/*
 *  ----------------   UART1 initialization  --------------------------
 */
    P4SEL |= BIT4|BIT5;                       // P3.3,4 = USCI_A0 TXD/RXD
    UCA1CTL1 |= UCSWRST;                      // **Put state machine in reset**
    UCA1CTL1 |= UCSSEL_2;                     // SMCLK
    UCA1BR0 = 139;                            // 16MHz, 115200 (see User's Guide)
    UCA1BR1 = 0;                              // 1MHz 115200
    UCA1MCTL |= UCBRS_1 + UCBRF_0;            // Modulation UCBRSx=1, UCBRFx=0
    UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**


    // send initialization text over UART

    UCA1TXBUF = 'o';
    __delay_cycles(UART_W8);
    UCA1TXBUF = 'n';
    __delay_cycles(UART_W8);
    UCA1TXBUF = '\r';
    __delay_cycles(UART_W8);
    UCA1TXBUF = '\n';
    __delay_cycles(UART_W8);
    UCA1TXBUF = '\r';
    __delay_cycles(UART_W8);
    UCA1TXBUF = '\n';
    unsigned char tx[4], tx2[5];	// UART buffers


/*
 * 	----------------   Other initialization --------------------------
 */

    int sampAry[NUM_SAMPS] = {0};
//    unsigned int avg=0;
    P4DIR |= BIT7;
    P1DIR &= ~BIT2;				   // Slave ready
    TA0CCTL0 = CCIE;
    __bis_SR_register(GIE);        // enable interrupts


    unsigned int *buffData;


    while(1){

    	// if there is an ADC sample pending
    	if(sampFlag == 1){

    		sampAry[ndx] = sample;		// copy sample val into array
    		ndx++;						// index for next sample

    		// if at the end of the array...
    		if(ndx>NUM_SAMPS){
    			UCA0IE |= UCRXIE;			// enable receive UART interrupt (check to see if slave is ready)
    			ndx = 0;					// reset ndx to 0
    			buffData = zeroX(sampAry, fs);		// [freq, mag] = zeroCrossing ( array of samples, sample freq)

    			// TX data
    			if(P1IN & BIT2){				// if slave is ready
    				TA0CCTL0 &= ~CCIE;			// disable timer interrupts
    				__bic_SR_register(GIE);		// disable global interrupts

    				// duplicate sample array for dual UART transmissions
    				int temp[2];
    				for(i=0; i<2; i++){
    					temp[i] = buffData[i];
    				}

    				// Send data to salve uC
    				UCA0IE &= ~UCRXIE;		// clr UART receive interrupt
    				xFerData(temp);			// send data to slave
    				UCA0IE |= UCRXIE;		// reenable UART interrupt

    				sendFlag = 0;
    			}

    			// Transmit frequencies to computer
    			UCA1IE |= UCRXIE;				// enable UCA1 interrupts

    			// convert ints to chars
    			tx2[4] = (buffData[0]%10)+'0';
    			buffData[0] /= 10;
    			tx2[3] = (buffData[0]%10)+'0';
    			buffData[0] /= 10;
    			tx2[2] = (buffData[0]%10)+'0';
    			buffData[0] /= 10;
    			tx2[1] = (buffData[0]%10)+'0';

    			// transmit char array
    			for(j=1;j<5;j++){
    				UCA1TXBUF = tx2[j];
    				__delay_cycles(UART_W8);
    			}

    			// display amplitude information
    			UCA1TXBUF = '\t';			// space out
    			__delay_cycles(UART_W8);

    			UCA1TXBUF = (buffData[1]&0x00FF) + '0';		// amplitude char
    			__delay_cycles(UART_W8);

    			// print carriage return, new line
    			UCA1TXBUF = '\r';
    			__delay_cycles(UART_W8);
    			UCA1TXBUF = '\n';
    			__delay_cycles(UART_W8);

    		}

    		// tidy up
    		P4OUT ^= BIT7;				// test if running
    		UCA0IE &= ~UCRXIE;			// disable UART interrupt from uC
    		__bis_SR_register(GIE);		// reenable globals
    		TA0CCTL0 |= CCIE;			// reenable timer interrupt

    		sampFlag = 0;				// clear sample flag
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


/*
 *
 * zero cross frequency function
 *
 *
 */
int* zeroX(int *sampAry, unsigned int fs){
	int avg = 2053, i, j;
	long int sum=0;
	float avgDist;
	int periodFlg=0, num=12;

	int stream[NUM_SAMPS], pos[NUM_SAMPS];
//	int levels[16] = {2592,2938,3141,3285,
//			3396,3488,3565,3631,
//			3690,3743,3791,3834,
//			3874,3946,3978,4008};

	int levels[16] = {2592,2938,3141,3285,
			3396,3700,3800,3850,
			3875,3900,3950,3970,
			3990,4000,4010,4020};

	// LPF
	pos[0] = 0;
	int td[8]={0}, idx=-1, max = 0;

	// for each data point in sampAry
	for(i=1; i<NUM_SAMPS-num; i++){
		if(sampAry[i]>max)(max=sampAry[i]);		// record max value of array

		//LPF
		for(j=0; j<num; j++){
			sum = sum+sampAry[i+j];
		}

		stream[i] = sum/num;
		sum=0;
		if((stream[i]-avg) & 0x8000)(pos[i] = 0);
		else(pos[i] = 1);

		if(pos[i-1] == 0 && pos[i] == 1){
			periodFlg = 1;
			idx++;
			if(idx>7){
				break;
			}
		}

		if(periodFlg == 1){
			td[idx]++;
		}
	}


	float freq;
	int ret[2];

	avgDist=0;

	// Low frequency, average only 3 zero crossings (0-248 Hz)
	if((td[1] > 170) && ((td[2]) > 170)){
		for(i=1; i<4; i++){
			avgDist += td[i];
		}
		avgDist = avgDist*0.334;
	}

	// Midrange, average 5 zero crossings (249-601 Hz)
	else if((td[1] > 70) && ((td[2]) > 70)){
		for(i=1; i<6; i++){
			avgDist += td[i];
		}
		avgDist = avgDist*0.2;
	}

	// Highest frequencies (Up to ~21 kHz), average 6 zero crossings
	else{
		for(i=1; i<7; i++){
			avgDist += td[i];
		}
		avgDist = avgDist*0.1667;
	}

	freq = fs/avgDist;

	ret[0] = freq;

	// VU data
	i=0;
	while(levels[i] < max){
		i++;
		if(i==15){
			break;
		}
	}

//	// shift up for ASCII table
//	if(i>9)(i+=7);

	ret[1] = i;

	return ret;
}



/*
 * Send 2 inegers as bytes over UART to LCD display uC.
 */
void xFerData(unsigned int* bData){
	char buffer[4], i, j, tx2[4];
//	buffData[1] |= 0xFF00;		// set last nibble in array = 1111 for tracking
	UCA0IE |= UCRXIE;
//	buffer[0] = (buffData[0]>>8) & 0xFF;
//	buffer[1] = buffData[0] & 0xFF;
//	buffer[2] = buffData[1];
//	buffer[3] = 0xFF;

	tx2[3] = (bData[0]%10)+'0';
	bData[0] /= 10;
	tx2[2] = (bData[0]%10)+'0';
	bData[0] /= 10;
	tx2[1] = (bData[0]%10)+'0';
	bData[0] /= 10;
	tx2[0] = (bData[0]%10)+'0';

	for(j=0;j<4;j++){
		UCA0TXBUF = tx2[j];
		__delay_cycles(UART_W8);
	}


	UCA0TXBUF = bData[1]+'0';
	__delay_cycles(UART_W8);



//	buffer[0] = 'r';
//	buffer[1] = 'x';
//	buffer[2] = '?';
//	buffer[3] = '\0';
//
//	for(i=0; i<4; i++){
//		UCA0TXBUF = buffer[i];
//		__delay_cycles(UART_W8);
//	}
	UCA0IE &= ~UCRXIE;

}



/*
 * Set SMCLK to 16MHz
 */
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
