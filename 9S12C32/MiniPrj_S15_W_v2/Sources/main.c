/* WRITER
************************************************************************
 ECE 362 - Mini-Project C Source File - Spring 2015
***********************************************************************
	 	   			 		  			 		  		
 Team ID: 24

 Project Name: Turn Down for Watt

 Team Members:

   - Team/Doc Leader: Erik Swan      Signature: ______________________
   
   - Software Leader: Lauren Heintz      Signature: ______________________

   - Interface Leader: Manik Singhal     Signature: ______________________

   - Peripheral Leader: Sahil Sanghani    Signature: ______________________


 Academic Honesty Statement:  In signing above, we hereby certify that we 
 are the individuals who created this HC(S)12 source file and that we have
 not copied the work of any other student (past or present) while completing 
 it. We understand that if we fail to honor this agreement, we will receive 
 a grade of ZERO and be subject to possible disciplinary action.

***********************************************************************

 The objective of this Mini-Project is to .... < ? >


***********************************************************************

 List of project-specific success criteria (functionality that will be
 demonstrated):

 1.

 2.

 3.

 4.

 5.

***********************************************************************

  Date code started: 4/22/2015

  Update history (add an entry every time a significant change is made):

  Date: 4/22   Name: Lauren Heintz   Update: Pots + sliding pots

  Date: 4/26  Name: Lauren Heintz   Update: use TIM int for rotor encoder 
                                            (right now channels programmed to PTT6 and PTT7)

  Date: < ? >  Name: < ? >   Update: < ? >


***********************************************************************
*/

#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>

/* All functions after main should be initialized here */
char inchar(void);
void outchar(char x);
void fdisp(void);
void shiftout(char);
void lcdwait(void);
void send_byte(char);
void send_i(char);
void chgline(char);
void print_c(char);
void pmsglcd(char[]);


/* Variable declarations */
char leftpb	= 0;  // left pushbutton flag
char rghtpb	= 0;  // right pushbutton flag
char prevpb	= 0;  // previous pushbutton state

int POT[6] = {0,0,0,0,0,0}; // 6 POTs   ATD 0-5
int FADER[2] = {0,0}; // 2 FADERS  ATD 6-7
// VARIABLES FOR MENU ENCODER
int lookup[16] = {0,1,2,3,2,0,3,1,1,3,0,2,3,2,1,0}; // lookup table for encoder
int code = 0; // the result of look up table
int state = 0; // variable for previous state and current state for channels 1 & B
int movement = 0; // increments or decrements based on direction of every pulse

   	   			 		  			 		       

/* Special ASCII characters */
#define CR 0x0D		// ASCII return 
#define LF 0x0A		// ASCII new line 

/* LCD COMMUNICATION BIT MASKS (note - different than previous labs) */
#define RS 0x10		// RS pin mask (PTT[4])
#define RW 0x20		// R/W pin mask (PTT[5])
#define LCDCLK 0x40	// LCD EN/CLK pin mask (PTT[6])

/* LCD INSTRUCTION CHARACTERS */
#define LCDON 0x0F	// LCD initialization command
#define LCDCLR 0x01	// LCD clear display command
#define TWOLINE 0x38	// LCD 2-line enable command
#define CURMOV 0xFE	// LCD cursor move instruction
#define LINE1 0x80	// LCD line 1 cursor position
#define LINE2 0xC0	// LCD line 2 cursor position

#define LEFT PTT_PTT1
#define RIGHT PTT_PTT0

	 	   		
/*	 	   		
***********************************************************************
 Initializations
***********************************************************************
*/

void  initializations(void) {

/* Set the PLL speed (bus clock = 24 MHz) */
  CLKSEL = CLKSEL & 0x80; //; disengage PLL from system
  PLLCTL = PLLCTL | 0x40; //; turn on PLL
  SYNR = 0x02;            //; set PLL multiplier
  REFDV = 0;              //; set PLL divider
  while (!(CRGFLG & 0x08)){  }
  CLKSEL = CLKSEL | 0x80; //; engage PLL

/* Disable watchdog timer (COPCTL register) */
  COPCTL = 0x40   ; //COP off; RTI and COP stopped in BDM-mode

/* Initialize asynchronous serial port (SCI) for 9600 baud, interrupts off initially */
  SCIBDH =  0x00; //set baud rate to 9600
  SCIBDL =  0x9C; //24,000,000 / 16 / 156 = 9600 (approx)  
  SCICR1 =  0x00; //$9C = 156
  SCICR2 =  0x0C; //initialize SCI for program-driven operation
  DDRB   =  0x10; //set PB4 for output mode
  PORTB  =  0x10; //assert DTR pin on COM port

/* Initialize peripherals */
  DDRT = 0x3F; // for encoder should be 9F becuase pt 5 and 5 are input for encoder
  PTT = 0x00;
  
  DDRAD = 0; 	
  ATDDIEN = 0xC0; // all analog inputs
  
  ATDCTL2 = 0x80;
  ATDCTL3 = 0x00; // conversion sequence length=8
  ATDCTL4 = 0x85;  
            
/* Initialize interrupts */
  SPICR1 = 0x50;
  SPIBR = 1;
	
/* Initialize the LCD
     - pull LCDCLK high (idle)
     - pull R/W' low (write state)
     - turn on LCD (LCDON instruction)
     - enable two-line mode (TWOLINE instruction)
     - clear LCD (LCDCLR instruction)
     - wait for 2ms so that the LCD can wake up     
*/ 
  PTT_PTT4 = 1;
  PTT_PTT3 = 0;
  send_i(LCDON);
  send_i(TWOLINE);
  send_i(LCDCLR);
  lcdwait();
  
/* Initialize RTI for 2.048 ms interrupt rate */	
  CRGINT_RTIE = 1;
  RTICTL = RTICTL | (0x51);
  
/* Initialize TIM Ch 7 (TC7) for periodic interrupts for encoder every .1 ms
     - enable timer subsystem
     - set channel 7 for output compare
     - set appropriate pre-scale factor and enable counter reset after OC7
     - set up channel 7 to generate 1 ms interrupt rate
     - initially disable TIM Ch 7 interrupts      
*/
/*  INITIALIZATIONS FOR ROTOR ENCODER */
  TSCR1_TEN = 1; // enable time subsystem
  TIOS_IOS7 = 1; // sets channel 7 to output compare
  TSCR2 = 0x0C;  // prescale 16
  TC7 = 30; // count up to 30 which samples every 20us in channel 7 register (1500 is 1ms)
	TIE = 0x80; // enable!!
  
  fdisp();      
	      
}

	 		  			 		  		
/*	 		  			 		  		
***********************************************************************
Main
***********************************************************************
*/
void main(void) {
  DisableInterrupts
	initializations(); 		  			 		  		
	EnableInterrupts;

 for(;;) {
  
/* < start of your main loop > */ 
   if (movement > 30){
    //skip forward
    movement = 0;
   } else if (movement < -30){
    // skip backward
    movement = 0;
   }
  
   } /* loop forever */
   
}   /* do not leave main */




/*
***********************************************************************                       
 RTI interrupt service routine: RTI_ISR
************************************************************************
*/

interrupt 7 void RTI_ISR(void)
{
  	// clear RTI interrupt flagt 
  	CRGFLG = CRGFLG | 0x80; 
    ATDCTL5 = 0x10;
 	  while(!ATDSTAT0_SCF)	{};
 	  
 	  POT[0] = ATDDR0H;  // high eq
 	  POT[1] = ATDDR1H;  // mid eq
 	  POT[2] = ATDDR2H;  // low eq
 	  POT[3] = ATDDR3H;  // fx1
 	  POT[4] = ATDDR4H;  // fx2
 	  POT[5] = ATDDR5H;  // fx3
 	  FADER[0] = ATDDR6H;// speed
 	  FADER[1] = ATDDR7H;// gain

}

/*
***********************************************************************                       
  TIM interrupt service routine	  		
***********************************************************************
*/

interrupt 15 void TIM_ISR(void)
{
  
  // INTERRUPT FOR ROTOR ENCODER
  
  // clear TIM CH 7 interrupt flag 
 	TFLG1 = TFLG1 | 0x80; 
 
  state = state << 1;
  state = state + PTT_PTT7;  // channel A
  state = state << 1;
  state = state + PTT_PTT6;  // channel B
  
  
  // adding in the current state
  state = state & 0x0F; // is int "state" 1 byte??
  code = lookup[state];
  
  if (code == 1) {
    movement++;
  } else if (code == 2){
    movement--;
  }
  // code = 0 is no change, code = 3 is error
  // after the song has skipped appropriately, reset movement back to 0 in the main
  
  
}

/*
***********************************************************************                       
  SCI interrupt service routine		 		  		
***********************************************************************
*/

interrupt 20 void SCI_ISR(void)
{
 


}

/*
***********************************************************************
 Character I/O Library Routines for 9S12C32 
***********************************************************************
 Name:         inchar
 Description:  inputs ASCII character from SCI serial port and returns it
 Example:      char ch1 = inchar();
***********************************************************************
*/

char inchar(void) {
  /* receives character from the terminal channel */
        while (!(SCISR1 & 0x20)); /* wait for input */
    return SCIDRL;
}

/*
***********************************************************************
 Name:         outchar    (use only for DEBUGGING purposes)
 Description:  outputs ASCII character x to SCI serial port
 Example:      outchar('x');
***********************************************************************
*/

void outchar(char x) {
  /* sends a character to the terminal channel */
    while (!(SCISR1 & 0x80));  /* wait for output buffer empty */
    SCIDRL = x;
}

/*
***********************************************************************
  fdisp: Display things on the LCD!        
***********************************************************************
*/

void fdisp(void)
{
  send_i(LCDCLR);
  
  chgline(LINE1);
  pmsglcd("This is the");
  chgline(LINE2);
  pmsglcd("writer! Slave #1"); 
 
}

/*
***********************************************************************
  shiftout: Transmits the character x to external shift 
            register using the SPI.  It should shift MSB first.  
             
            MISO = PM[4]
            SCK  = PM[5]
***********************************************************************
*/
 
void shiftout(char x)

{
 
  // read the SPTEF bit, continue if bit is 1
  // write data to SPI data register
  // wait for 30 cycles for SPI data to shift out
  // SPIDR serial peripheral interface
  int i = 0;
  while (SPISR_SPTEF == 0) {
  }
   
  SPIDR = x;
  
  for (i=0; i<30; i++) {
      asm {
      NOP
      }
  }

}

/*
***********************************************************************
  lcdwait: Delay for approx 2 ms
***********************************************************************
*/

void lcdwait()
{
     asm{
      pshc
      pshx
      pshy
      ldx #2
loopo:
      ldy #7997
loopi:
      dbne y,loopi
      dbne x,loopo
  
      puly
      pulx
      pulc
      }
}

/*
*********************************************************************** 
  send_byte: writes character x to the LCD
***********************************************************************
*/

void send_byte(char x)
{
     // shift out character
     // pulse LCD clock line low->high->low
     // wait 2 ms for LCD to process data
     shiftout(x);
     PTT_PTT4 = 1;
     PTT_PTT4 = 0;
     PTT_PTT4 = 1;
     lcdwait();
}

/*
***********************************************************************
  send_i: Sends instruction byte x to LCD  
***********************************************************************
*/

void send_i(char x)
{
        // set the register select line low (instruction data)
        // send byte
        PTT_PTT2 = 0;
        send_byte(x);
}

/*
***********************************************************************
  chgline: Move LCD cursor to position x
  NOTE: Cursor positions are encoded in the LINE1/LINE2 variables
***********************************************************************
*/

void chgline(char x)
{
        send_i(CURMOV);
        send_i(x);
}

/*
***********************************************************************
  print_c: Print (single) character x on LCD            
***********************************************************************
*/
 
void print_c(char x)
{
        PTT_PTT2 = 1;
        send_byte(x);
}

/*
***********************************************************************
  pmsglcd: print character string str[] on LCD
***********************************************************************
*/

void pmsglcd(char str[])
{
      int i = 0;
      while (str[i] != '\0'){
          print_c(str[i]);
          i++; 
      }
}