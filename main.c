//==============================================================================

//==============================================================================
#include <msp430.h>
#include <msp430f5659.h>
#include "msp430_uart.h"
#include "Display.h"

//int data_in_flag = 0;
char cmd_recv[18]={'d'};               // actual received command from uart
int cmd_recv_pos=0;								// buffer position, operate as a circular one
int cmd_recv_cnt=0;                   // counter for amount of command char received

char cmd_buff[18]={'d'};               // buffer for store the command received from uart, it's a circular buffer
int cmd_buff_pos=0;								// buffer position, operate as a circular one
int cmd_buff_cnt=0;                   // counter for amount of command char received

int broken_flag = 0;					// indicate if buffer info is broken(not in protocol format), 0 means ok, 1 means broken detected

char operation_mode = 'd';			// totally 6 types, 'c' for car, 'p' for people, 'l' for left, 'r' for right, 'v' for velocity, 'd' for default, do nothing
char glitch_check = 'd';               // should be 'i' if a string of info is valid
	  	  	  	  	  	  	  	  	  // the uart string format should be "cin", "piy","vi0023" , first is operation_mode char, second is glitch_check char, third is disp info
	  	  	  	  	  	  	  	  	  // actually when it comes to velocity info, the 3rd digit 0 can be anything, I just use last 3 digit, first one totally ignore
#define ON      2          // for the all kinds of display status
#define OFF     1
#define PRE     0

int disp_car_status = OFF;            // as for all display status, 0 means no change, 1 means don't display, 2 means display
int disp_people_status = OFF;
int disp_left_status = ON;
int disp_right_status = ON;
char velocity_status[3] = {'0'};   // 3 digit speed info, from low to high, velocity_status[0] is the hundred bit etc.
int disp_velocity_status = ON;    // should always be on
int velocity = 0;
int check_vel_first = 0;
int beep_gap=0;                   // counter of the beeper

void clearBufferAndStatus();         // just clear buffer and status
void displayProcess();               // display the screen

void main(void)
{
  /*WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  //UCSCTL3 = SELREF__REFOCLK;                      // internal 32768hz
  UCSCTL2 = FLLD__8 + 127;                                          // set as 8MHz, DCO,  8*32 setting's is 2.45Mhz
  UCSCTL4 = SELA__DCOCLKDIV + SELS__DCOCLKDIV + SELM__DCOCLKDIV;  // set the all the clock(ACLK, MCLK, SMCLK) as DCOCLK  */
		WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
		UCSCTL3 |= SELREF_2; // Set DCO FLL reference = REFO    internal 32768hz crystal
		UCSCTL4 |= SELA_2; // Set ACLK = REFO, MCLK and SMCLK use default value as DCOCLKDIV
		__bis_SR_register(SCG0);
		// Disable the FLL control loop
		UCSCTL0 = 0x0000; // Set lowest possible DCOx, MODx
		UCSCTL1 = DCORSEL_5; // Select DCO range 16MHz operation
		UCSCTL2 = FLLD_1 + 255; // Set DCO Multiplier for 8MHz
								// (N + 1) * FLLRef = Fdco
								// (255 + 1) * 32768 = 8MHz
								// Set FLL Div = fDCOCLK/2

		P4DIR |= BIT4;                            // P1.0 output
		  TA1CCTL0 = CCIE;                          // CCR0 interrupt enabled
		  TA1CCR0 = 1000;
		  TA1CTL = TASSEL_2 | MC_1 | TACLR;         // SMCLK, upmode, clear TAR
		__bic_SR_register(SCG0);
		// Enable the FLL control loop

		// Worst-case settling time for the DCO when the DCO range bits have been
		// changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
		// UG for optimization.
		// 32 x 32 x 8 MHz / 32,768 Hz ~ 262000 = MCLK cycles for DCO to settle
		__delay_cycles(262000);
		  P1DIR |= BIT2;
		  P1OUT |= BIT2;				// Feedback to button control chip, prevent reset
		// Loop until XT1,XT2 & DCO fault flag is cleared
		do {
			UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
			// Clear XT2,XT1,DCO fault flags
			SFRIFG1 &= ~OFIFG; // Clear fault flags
		} while (SFRIFG1 & OFIFG); // Test oscillator fault flag

  Uart_Init(9600, 'n', 'l', '8', 1);		  // init uart 1
  Display_Initial();                         // init display
  displayLeftSide(2);
  displayRightSide(2);
  displayMiddle(2,2,2,888);                 // display all pics available on screen
  int delay=0;
  for(delay=0;delay<10;delay++) __delay_cycles(640000);        // just a delay, add if u want
  displayMiddle(1,1,1,000);                 // start from 000 speed
  //__bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, interrupts enabled
  __no_operation();                         // For debugger

  int loop_tmp=0;                // c language can't init this variables in loop
  while(1){
	  while(cmd_recv_cnt<18) _NOP();      // wait for the buffer get filled
	  Uart_disableRXINT();               // disable uart receiver before processing
	  cmd_buff_cnt = cmd_recv_cnt;       // store the recv count and buff pos
	  cmd_buff_pos = 0;
	  for(loop_tmp=0;loop_tmp<18;loop_tmp++){
		  cmd_buff[loop_tmp] = cmd_recv[loop_tmp];
	  }
	  clearBufferAndStatus();
	  Uart_enableRXINT();               // reable uart receiver before processing
	  if(check_vel_first == 1){         // need to check first 3 volicity digit first, it's follow up bit from last round
		  velocity_status[0] = cmd_buff[cmd_buff_pos];	// store the hundred's digit
		  velocity_status[1] = cmd_buff[cmd_buff_pos+1];	// store the ten's digit
		  velocity_status[2] = cmd_buff[cmd_buff_pos+2];    // store the digit
		  check_vel_first = 0;          // reset this velocity first check after checking first 3 digits
		  if(velocity_status[0]<'0'|| velocity_status[0]>'9'
				  || velocity_status[1]<'0'|| velocity_status[1]>'9'          // if any of these digit is not a valid number, break, go next process
				  || velocity_status[2]<'0'|| velocity_status[2]>'9'){
		  }else{
			  velocity = (velocity_status[0]-'0')*100 + (velocity_status[1]-'0')*10 + (velocity_status[2]-'0'); // calculate the velocity
			  disp_velocity_status = ON;
		  }
		  cmd_buff_pos += 3;       // move forward position for 1 pos, jump over processed one
		  cmd_buff_cnt -= 3;
	  }

	  // main command detect loop
	  while(cmd_buff_cnt>0){        // break judge
		  while(cmd_buff_cnt>0 && (cmd_buff[cmd_buff_pos] != 'c' && cmd_buff[cmd_buff_pos] != 'p'    // detect first valid operation mode char, if no error, should be at pos 0
				  	  	  	   && cmd_buff[cmd_buff_pos] != 'l' && cmd_buff[cmd_buff_pos] != 'r'     // if all invalid, cnt will go down to 0 and break loop;
				  	  	  	   && cmd_buff[cmd_buff_pos] != 'v')\
			   ){
			  cmd_buff_cnt--;	  // total buff info count -1
			  cmd_buff_pos++;     // move to the next buff position
		  }
		  while(cmd_buff_cnt>0){
			  operation_mode = cmd_buff[cmd_buff_pos++];        // store the operation mode char
			  cmd_buff_cnt--;									// buff count -1
			  if(cmd_buff_cnt<=1){
				  broken_flag = 1;
				  break;                      // less than 1 data left in buffer, ignore
			  }
			  switch(operation_mode){
			  case 'c':{
				  	  	  glitch_check = cmd_buff[cmd_buff_pos];
				  	  	  if(glitch_check != 'i') break;        // if no 'i' as second byte income, means broken info, ignore it, restart checking process
				  	  	  switch(cmd_buff[cmd_buff_pos+1]){
				  	  	  case 'y': disp_car_status = ON; break;        // display car, on
				  	  	  case 'n': disp_car_status = OFF; break;		   // display car, off
				  	  	  default:  disp_car_status = PRE; break;		   // display car, remain last state
				  	  	  }
				  	  	  glitch_check = 'd';      // reset glitch_check
				  	  	  cmd_buff_pos += 2;       // move forward position for 2 pos, jump over processed one
				  	  	  cmd_buff_cnt -= 2;
				  	  	  break;
			  	  	   }
			  case 'p':{
				  	  	  glitch_check = cmd_buff[cmd_buff_pos];
						  if(glitch_check != 'i') break;        // if no 'i' as second byte income, means broken info, ignore it
						  switch(cmd_buff[cmd_buff_pos+1]){
						  case 'y': disp_people_status = ON; break;        // display car, on
						  case 'n': disp_people_status = OFF; break;		   // display car, off
						  default:  disp_people_status = PRE; break;		   // display car, remain last state
						  }
						  glitch_check = 'd';      // reset glitch_check
						  cmd_buff_pos += 2;       // move forward position for 2 pos, jump over processed one
						  cmd_buff_cnt -= 2;
						  break;
			  	  	   }
			  case 'l':{
				  	  	  glitch_check = cmd_buff[cmd_buff_pos];
						  if(glitch_check != 'i') break;        // if no 'i' as second byte income, means broken info, ignore it
						  switch(cmd_buff[cmd_buff_pos+1]){
						  case 'y': disp_left_status = ON; break;        // display car, on
						  case 'n': disp_left_status = OFF; break;		   // display car, off
						  default:  disp_left_status = PRE; break;		   // display car, remain last state
						  }
						  glitch_check = 'd';      // reset glitch_check
						  cmd_buff_pos += 2;       // move forward position for 2 pos, jump over processed one
						  cmd_buff_cnt -= 2;
						  break;
			  	  	   }
			  case 'r':{
				  	  	  glitch_check = cmd_buff[cmd_buff_pos];
						  if(glitch_check != 'i') break;        // if no 'i' as second byte income, means broken info, ignore it
						  switch(cmd_buff[cmd_buff_pos+1]){
						  case 'y': disp_right_status = ON; break;        // display car, on
						  case 'n': disp_right_status = OFF; break;		   // display car, off
						  default:  disp_right_status = PRE; break;		   // display car, remain last state
						  }
						  glitch_check = 'd';      // reset glitch_check
						  cmd_buff_pos += 2;       // move forward position for 2 pos, jump over processed one
						  cmd_buff_cnt -= 2;
						  break;
			  	  	   }
			  case 'v':{
				  	  	  glitch_check = cmd_buff[cmd_buff_pos];
						  if(glitch_check != 'i') break;        // if no 'i' as second byte income, means broken info, ignore it
						  if(cmd_buff_cnt>=5){              // if buffer has already store the info this round
							  velocity_status[0] = cmd_buff[cmd_buff_pos+2];	// store the hundred's digit
							  velocity_status[1] = cmd_buff[cmd_buff_pos+3];	// store the ten's digit
							  velocity_status[2] = cmd_buff[cmd_buff_pos+4];    // store the digit
							  if(velocity_status[0]<'0'|| velocity_status[0]>'9'
									  || velocity_status[1]<'0'|| velocity_status[1]>'9'          // if any of these digit is not a valid number, break, go next process
									  || velocity_status[2]<'0'|| velocity_status[2]>'9') {
								  cmd_buff_pos += 1;       // move forward position for 1 pos, jump over processed one
								  cmd_buff_cnt -= 1;
								  break;
							  }
							  velocity = (velocity_status[0]-'0')*100 + (velocity_status[1]-'0')*10 + (velocity_status[2]-'0'); // calculate the velocity
							  disp_velocity_status = ON;
							  glitch_check = 'd';      // reset glitch_check
							  cmd_buff_pos += 5;       // move forward position for 2 pos, jump over processed one
							  cmd_buff_cnt -= 5;
							  break;
						  }else{               // if info goes like  "ciypinvi0 + 023" (023 is int next round buffer)
							  check_vel_first = 1;
							  glitch_check = 'd';      // reset glitch_check
							  cmd_buff_pos += 2;       // move forward position for 2 pos, jump over processed one
							  cmd_buff_cnt -= 2;
						  }

			  	  	   }
			  default: {
				  	  	  break;
			  	  	   }
			  }
		  }

		  displayProcess(disp_left_status, disp_right_status, disp_car_status, disp_people_status, disp_velocity_status, velocity);
		  _nop();

	  }
  }

}

void displayProcess(int leftside, int rightside, int car_flag, int people_flag, int velocity_flag, int velocity){   // i didn't use velocity_flag actually, just update all the time
	displayLeftSide(leftside);
	displayRightSide(rightside);
	displayMiddle(car_flag, people_flag, velocity_flag, velocity);
	//for(tt=0;tt<50;tt++) __delay_cycles(80000);        // just a delay, add if u want
}

void clearBufferAndStatus(){         // sorry for the name, at the end it won't clear status, it saves the previous one until it changes
	  operation_mode = 'd';			// totally 6 types, 'c' for car, 'p' for people, 'l' for left, 'r' for right, 'v' for velocity, 'd' for default, do nothing
	  glitch_check = 'd';               // should be 'i' if a string of info is valid
	  	  	  	  	  	  	  	  	  	  // the uart string format should be "cin", "piy","vi0023" , first is operation_mode char, second is glitch_check char, third is disp info
	  	  	  	  	  	  	  	  	  	  // actually when it comes to velocity info, the 3rd digit 0 can be anything, I just use last 3 digit, first one totally ignore
	  /* disp_car_status = 0;            // as for all display status, 0 means no change, 1 means don't display, 2 means display
	  disp_people_status = 0;
	  disp_left_status = 0;
	  disp_right_status = 0;
	  velocity_status[0] = '0';   // 3 digit speed info, from low to high, velocity_status[0] is the hundred bit etc.
	  velocity_status[1] = '0';   // 3 digit speed info, from low to high, velocity_status[0] is the hundred bit etc.
	  velocity_status[2] = '0';   // 3 digit speed info, from low to high, velocity_status[0] is the hundred bit etc.
	  disp_velocity_status = 0;
	  velocity = 0;   */
	  cmd_recv_pos = 0;
	  cmd_recv_cnt = 0;
}


// Echo back RXed character, confirm TX buffer is ready first
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
	//TA1CCTL0 &= ~CCIE;                          // CCR0 interrupt disbaled, prevent timer process during uart
                                    // Vector 2 - RXIFG
	  while (!(UCA0IFG&UCRXIFG));             // USCI_A0 TX buffer ready?
	  if(!broken_flag){
		  cmd_recv[cmd_recv_pos++] = UCA0RXBUF;   // store the incoming command char
		  cmd_recv_pos = cmd_recv_pos % 18;       // make sure position always
		  cmd_recv_cnt++;
	  }else{
		  cmd_recv[0] = UCA0RXBUF;
		  if((cmd_recv[0] == 'c' || cmd_recv[0] == 'p'    // if previous round broken command detected,   detect first valid operation mode char
	  	  	  	   || cmd_recv[0] == 'l' || cmd_recv[0] == 'r'      // this round, and allign it to the beginning of the buffer
	  	  	  	   || cmd_recv[0] == 'v')){
			  cmd_recv_pos=1;
			  cmd_recv_cnt=1;
			  broken_flag = 0;
		  }
	  }


}

// Timer1 A0 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER1_A0_VECTOR))) TIMER1_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
  beep_gap += 1;
  if(disp_car_status ==2 || disp_people_status==2){
	  if(beep_gap<500)
	  	  P4OUT ^= BIT4;                            // Toggle P1.0
	  else
	  	  P4OUT = 0;
	  beep_gap = beep_gap % 1000;
  }else{
	  beep_gap = 0;
  }

}


/*void uart_1_init(void)
{
  // initialize the uart port1(UCA1) on msp430f5659
  P8SEL |= BIT2 + BIT3;                     // active P8.2 to UCA1TXD and P8.3 to UCA1RXD module function
  P8DIR |= BIT2;                            // TXD need to be set as output direction
  P8DIR &= ~BIT3;                           // RXD input direction
  P8REN |= BIT3;                            // RXD need to set a pulldown resister to get input (don't pull up)

  UCA1CTL1 |= UCSWRST;                      // **Put state machine in reset**
  UCA1CTL1 |= UCSSEL__SMCLK;                // SMCLK
  UCA1BR0 = 6;                              // 1MHz 9600 (see User's Guide)
  UCA1BR1 = 0;                              // 1MHz 9600
  UCA1MCTL = UCBRS_0 + UCBRF_13 + UCOS16;   // 1MHz 9600
  UCA1IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
  UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
}*/
