/*  READER
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

  Date: 4/23   Name: Lauren Heintz   Update: create reader, code for reading encoder

  Date: 4/25  Name: Lauren Heintz   Update: LCD code

  Date: 4/26  Name: Lauren Heintz   Update: Encoder and LCD menu finished
  
  Date: 4/26  Name: Lauren Heintz   Update: Change pin assignments to match PCB schematic


***********************************************************************
*/

#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>

/* All functions after main should be initialized here */
char inchar(void);
void outchar(char x);
void list_disp(void);
void shiftout(char);
void lcdwait(void);
void send_byte(char);
void send_i(char);
void chgline(char);
void print_c(char);
void pmsglcd(char[]);
void cur_forward(void);
void cur_backward(void);
void song_disp(void);


/* Variable declarations */
char leftpb	= 0;  // left pushbutton flag
char rghtpb	= 0;  // right pushbutton flag
char prevpb	= 0;  // previous pushbutton state

//int POT[6] = {0,0,0,0,0,0}; // 6 POTs   ATD 0-5
//int FADER[2] = {0,0}; // 2 FADERS  ATD 6-7
int lookup[16] = {0,1,2,3,2,0,3,1,1,3,0,2,3,2,1,0}; // lookup table for encoder
int code = 0;
int direction = 0;
int state = 0;
int CW = 0;
int CCW = 0;
char temp;
//int counta = 0;
char * songs[5] = {"Song 1","Song 2","Song 3","Song 4","Song 5"};
char * artists[5] = {"Artist 1","Artist 2","Artist 3","Artist 4","Artist 5"};
int track_no = 0; // an index value pointing to a track # in array
int counter = 0; // a variable used to watched activity of menu encoder (# of clicks)
int song_select = 0; // signals whether the pushbutton on menuencoder was hit
int menu_mode = 1; // is asserted if it's in "menumode" aka scrolling through songs
                   // freezes screen on the song so only back can be used to go back
int brightness = 0;
int back = 0;
   	   			 		  			 		       

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
//#define RIGHT PTT_PTT0
#define DIN_SH PTAD_PTAD5 // Dont forget to change backto ptad7
#define CLK_SH PTT_PTT0	 	   		
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
//  SCIBDH =  0x00; //set baud rate to 9600
 // SCIBDL =  0x9C; //24,000,000 / 16 / 156 = 9600 (approx)  
//  SCICR1 =  0x00; //$9C = 156
//  SCICR2 =  0x0C; //initialize SCI for program-driven operation
//  DDRB   =  0x10; //set PB4 for output mode
//  PORTB  =  0x10; //assert DTR pin on COM port
  

/* Initialize peripherals */
  DDRT = 0xFF; // 1F becuase pt 5,6 are input for encoder, 7 is button
  PTT = 0x00;
  
  // CHANGE THESE.........?????
  DDRAD = 0x80; 	//change
  ATDDIEN = 0xFE; // all ATD except ATD0 (brightness pot) are digIO
  ATDCTL2 = 0x80;
  ATDCTL3 = 0x08; // conversion sequence length=1
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
  PTT_PTT5 = 1;
  PTT_PTT3 = 0;
  send_i(LCDON);
  send_i(TWOLINE);
  send_i(LCDCLR);
  lcdwait();
  list_disp();
  //send_byte(0xAA);
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
  TSCR1_TEN = 1; // enable time subsystem
  TIOS_IOS7 = 1; // sets channel 7 to output compare
  TSCR2 = 0x0C;  // prescale 16
  TC7 = 300; // count up to 1500 which is in channel 7 register
	TIE = 0x80; // enable!!
            
//  list_disp();
 
// SCI Ports         
  ///DDRM = 0x03;
  //PTT_PTT2 = 1;
  //PTT_PTT1 = 1;	      
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
   
   // moves cursor down one song
   if ((CW == 1) & (menu_mode==1)){
    cur_forward();
    CW = 0;
    counter++;
   }
   
   // moves cursor up one song
   if ((CCW == 1) & (menu_mode==1)){
    CCW = 0;
    counter--;
    cur_backward();
   }
   
   // selects song 
   if (song_select==1){
     song_select = 0;
     menu_mode = 0;
     song_disp();
   }
   
   // goes "back"
   if (back == 1){
    back = 0;
    // LEFT PB WAS USED AS "back", this should be changed to back button
    send_i(LCDCLR);
    track_no--;
    cur_forward();
    menu_mode = 1;
   }
   
 }
   /* loop forever */
   
}   /* do not leave main */




/*
***********************************************************************                       
 RTI interrupt service routine: RTI_ISR
************************************************************************
*/

interrupt 7 void RTI_ISR(void)
{
  	 // ATD conv sequence routine
  	
  	// clear RTI interrupt flag
  	CRGFLG = CRGFLG | 0x80; 
    ATDCTL5 = 0x10;
 	  while(!ATDSTAT0_SCF)	{};
 	  
 	  brightness = ATDDR0H;
    /*
    if(PTAD_PTAD6 == 0 && (prevpb & 0x01)) {
	    rghtpb = 1;  
	  }  	

	  if(PTAD_PTAD7 == 0 && (prevpb & 0x02)) {
	    leftpb = 1;  
	  } 
	  
	  prevpb = (PTAD_PTAD7 << 1) | PTAD_PTAD6;
	  */
	  
	  if (PTAD_PTAD5 == 0){
	    back = 1;
	  }
	  
    
}


/*
***********************************************************************                       
  TIM interrupt service routine	  		
***********************************************************************
*/

interrupt 15 void TIM_ISR(void)
{
  // clear TIM CH 7 interrupt flag 
 	TFLG1 = TFLG1 | 0x80; 
 
  state = state << 1;
  state = state + PTAD_PTAD2;  // channel A
  state = state << 1;
  state = state + PTAD_PTAD1;  // channel B
  
  
  // adding in the current state
  state = state & 0x0F; // is int "state" 1 byte??
  code = lookup[state];
  
  if (code == 1) {
    direction++;
  } else if (code == 2){
    direction--;
  }
  // code = 0 is no change, code = 3 is error
  
  if (direction == 4) {
    CW = 1; 
    direction = 0;
  } else if (direction == -4){
    CCW = 1;
    direction = 0;
  }
  // deal with CW and CCW in the main and reset flag there
  
  // PTAD3 is where the "select" / push button from encoder is coming
  if (PTAD_PTAD3 == 0) {
    song_select = 1;
  }
  
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
  list_disp: Display list on the LCD!        
***********************************************************************
*/

void list_disp(void)
{
  send_i(LCDCLR);
  
  chgline(LINE1);
  pmsglcd(">");
  pmsglcd(songs[0]);  
  chgline(LINE2);
  pmsglcd(" ");
  pmsglcd(songs[1]);
 
}

/*
***********************************************************************
  song_disp: Display song on the LCD!        
***********************************************************************
*/

void song_disp(void)
{
  send_i(LCDCLR);
  
  chgline(LINE1);
  pmsglcd(songs[track_no]);  
  chgline(LINE2);
  pmsglcd(artists[track_no]);
  
  // send correct time to corner
  chgline(0xCC);
  pmsglcd("0:00");  
 
 
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
 temp = x;  
//  PTT = PTT | 0x80;
  PTT_PTT7 = 0;
  PTT_PTT6 = 1;

  asm{
       pshc
       pshd
       pshx
       movb temp,1,-SP
       pulb
       ldaa #9
loop1: dbeq A,exit
       lslb
       bcc  sendz
       bset PTAD,$80
       bclr PTT,$01
       bset PTT,$01
       bclr PTT,$01
       bra  loop1
       
sendz: bclr PTAD,$80
       bclr PTT,$01
       bset PTT,$01
       bclr PTT,$01       
       bra  loop1
exit:  
       pulx
       puld
       pulc
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
     PTT_PTT5 = 1;
     PTT_PTT5 = 0;
     PTT_PTT5 = 1;
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
        PTT_PTT4 = 0;
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
        PTT_PTT4 = 1;
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

/*
***********************************************************************
  cur_forward: moves cursor forward to next song
***********************************************************************
*/

void cur_forward(void)
{
    if (track_no < 4){
      track_no++;
    }
    
    chgline(LINE1);
    pmsglcd(" ");
    pmsglcd(songs[track_no - 1]);
    chgline(LINE2);
    pmsglcd(">");
    pmsglcd(songs[track_no]);
      
}

/*
***********************************************************************
  cur_backward: moves LCD cursor back up to previous song
***********************************************************************
*/

void cur_backward(void)
{
    if (track_no > 0){
      track_no--;
    }
    
    chgline(LINE1);
    pmsglcd(">");
    pmsglcd(songs[track_no]);
    chgline(LINE2);
    pmsglcd(" ");
    pmsglcd(songs[track_no+1]);
      
}