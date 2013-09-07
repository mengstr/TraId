#include <stdint.h>         // For uint8_t definition
#include <htc.h> 			// For definitions of LAT/TRIS/PIN

#define _1LCD	

#define _XTAL_FREQ 	16000000		// Define cpu frequency for the __delay_xx()
#define LCDlow 		{LATC3=0;}		// Set LCD pin low level
#define LCDhigh  	{LATC3=1;}		// Set LCD pin high level
#define LCDout 		{TRISC3=0;}		// Set LCD pin as output
#define LCDin 		{TRISC3=1;}		// Set LCD pin as input
#define LCDread		(PORTC&0x08)	// Read from LCD pin
#define LCDbl 		1				// Backlight on


void LCDInit(void);
void LCDCommand(uint8_t);
void LCDData(uint8_t);
uint8_t LCDButton(void);
void LCDHexAt(uint8_t, uint8_t);
