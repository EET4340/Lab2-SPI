#include <xc.h>
#include <stdio.h>
#include "LCD.h"


	#pragma config FOSC=HSPLL
	#pragma config WDTEN=OFF
	#pragma config XINST=OFF


//Wire EEPROM as below
//PIC Board    EEPROM
//======================
//    RD7  <->  CS
//    RD6  <->  SCK
//    RD5  <->  SO
//    RD4  <->  SI
//    3.3V <->  Vcc, WP, HOLD
//    GND  <->  Vss
//
////////////////////////////////////

void InitPins(void);
void ConfigInterrupts(void);
void ConfigPeriph(void);

unsigned char SPIReadWrite(unsigned char byte);

#define _XTAL_FREQ 32000000L

int rx;
int count;


void main(void)
{
	count = 0;
	OSCTUNEbits.PLLEN = 1;  
	LCDInit();
	LCDClear();
	InitPins();
	ConfigPeriph();
	
	//Read address 0x0280 of EEPROM
	LATDbits.LATD7 = 0;  //enable CS
	SPIReadWrite(0b00000011);  //Read command
	SPIReadWrite(0x00);		//24 bit address (0x000280)
	SPIReadWrite(0x02);
        SPIReadWrite(0x80);   
	rx = SPIReadWrite(0);  //Read value - data sent is dummy data
	LATDbits.LATD7 = 1;  //disable CS
    
    //Display the value on LCD
	lprintf(1, "Read %d", rx);
    
	ConfigInterrupts();

	while (1)
	{
		lprintf(0, "%d", count);
		__delay_ms(250);	
        ++count;
	}
}

unsigned char SPIReadWrite(unsigned char byte)
{
	unsigned char r;
	SSP2BUF = byte; //transmit byte
	while(!PIR3bits.SSP2IF); //Wait until completed
	PIR3bits.SSP2IF = 0; //Clear flag so it is ready for next transfer
	r = SSP2BUF;  //read received byte
	return r;
}

void InitPins(void)
{
	LATD = 0b10000000; 	//LED's off, CS high
	TRISD = 0b00100000;  //RD5 is input (SI)  all the rest outputs
	TRISB = 0b00000001;	//Button0 is input;
	INTCON2bits.RBPU = 0;  //enable weak pullups on port
}

void ConfigInterrupts(void)
{

	RCONbits.IPEN = 0; //no priorities.  This is the default.

	//Configure your interrupts here

	//set up INT0 to interrupt on falling edge
	INTCON2bits.INTEDG0 = 0;  //interrupt on falling edge
	INTCONbits.INT0IE = 1;  //Enable the interrupt
	//note that we don't need to set the priority because we disabled priorities (and INT0 is ALWAYS high priority when priorities are enabled.)
	INTCONbits.INT0IF = 0;  //Always clear the flag before enabling interrupts
	
	
	INTCONbits.GIE = 1;  //Turn on interrupts
}

void ConfigPeriph(void)
{

	//Configure peripherals here
    //Configure MSSP 2 for SPI at 2 MHz, mode 0,0
	SSP2STATbits.CKE = 1;
	SSP2CON1bits.CKP = 0;  //SPI mode 0,0
	SSP2CON1bits.SSPM = 0b0001;	//SPI Master - FOSC/16 = 2 MHz
	SSP2CON1bits.SSPEN = 1;	//Enable MSSP
}


void __interrupt(high_priority) HighIsr(void)
{
	//Check the source of the interrupt
	if (INTCONbits.INT0IF == 1)
	{
		//source is INT0

		//Write address 0x0280 of EEPROM
		LATDbits.LATD7 = 0;  //enable CS
		SPIReadWrite(0b00000110);  //WREN command
		LATDbits.LATD7 = 1;  //disable CS
		Nop();
		LATDbits.LATD7 = 0;  //enable CS
		SPIReadWrite(0b00000010);  //Write command
		SPIReadWrite(0x00);		//24 bit address (0x000280)
		SPIReadWrite(0x02);
                SPIReadWrite(0x80); 
		SPIReadWrite(count);  //Write value
		LATDbits.LATD7 = 1;  //disable CS
		__delay_ms(6); //wait for write cycle to finish
		
		lprintf(1, "Wrote %d", count);
		INTCONbits.INT0IF = 0; //must clear the flag to avoid recursive interrupts
	}		
}
