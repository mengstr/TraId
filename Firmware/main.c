/*
		===============================================
		TraId - A transistor type & polarity identifier

		Copyright (C) 2013  Mats Engstrom
		===============================================
		
		This program is free software; you can redistribute it and/or
		modify it under the terms of the GNU General Public License
		as published by the Free Software Foundation; either version 2
		of the License, or (at your option) any later version.
		
		This program is distributed in the hope that it will be useful,
		but WITHOUT ANY WARRANTY; without even the implied warranty of
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
		GNU General Public License for more details.
		
		You should have received a copy of the GNU General Public License
		along with this program; if not, write to the Free Software
		Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <htc.h> 
#include <stdint.h>        	// For uint8_t definition
//#include "1LCD.h"			// Uncomment this to enable LCD debugging
//#define DEBUG_HILOCNT		// Uncomment this to flash the results from initial scan

__CONFIG(
	FOSC_INTOSC&
	WDTE_SWDTEN&  // WDT controlled by the SWDTEN bit in the WDTCON register
	MCLRE_ON&
	PWRTE_OFF&
	CP_OFF&
	BOREN_OFF&
	CLKOUTEN_OFF); 
__CONFIG( 
	WRT_OFF & 
	STVREN_ON & 
	BORV_19 & 
	LVP_OFF);  


#define VERSION_MAJ	1			// Firmware version number to be flashed at shutdown
#define VERSION_MIN 1

#define _XTAL_FREQ 	16000000	// Use the the 16 MHz cpu clock
#define TIMEOUT 	4*30		// Autosleep after 30 seconds 

#define IN			0x00		// Modes for the measurement pins
#define LOW			0x01		
#define HIGH 		0x02

#define RES_NONE	0			// #0..6 are results from Measure() added with RES_NPN/PNP

#define RES_EBC		1			// #1..6 are also used directly for SetLed()
#define RES_ECB		2
#define RES_BCE		3
#define RES_BEC		4
#define RES_CBE		5
#define RES_CEB		6
#define RES_SGD		1			
#define RES_SDG		2
#define RES_GDS		3
#define RES_GSD		4
#define RES_DGS		5
#define RES_DSG		6

#define RES_NPN		0x10		// Less code to check single bit than using modulus
#define RES_PNP		0x20
#define RES_NCH		0x40
#define RES_PCH		0x80

#define LED_OFF		0			// For SetLed()
#define LED_NPN		7			
#define LED_PNP		8
#define LED_NCH		7			
#define LED_PCH		8
#define LED_PWR		9

#define ONE1K 		A2			// Pins where the 1K resistors are connected to
#define TWO1K 		C0
#define THREE1K 	C2

#define ONE300K 	A0			// Pins where the 300K resistors are connected to
#define TWO300K 	A1
#define THREE300K 	C1
	
#define ONEADC		0			// ADC Channel numbers for the xxx300K pins
#define TWOADC		1
#define THREEADC	5


#define _AN(a) 		__AN(a)
#define __AN(a) 	ANS##a
#define _TRIS(a) 	__TRIS(a)
#define __TRIS(a) 	TRIS##a
#define _LAT(a) 	__LAT(a)
#define __LAT(a) 	LAT##a

#define HLL ((HIGH<<0)|(LOW<<2) |(LOW<<4))	// 1K for type determination
#define LHL ((LOW<<0) |(HIGH<<2)|(LOW<<4))
#define HHL ((HIGH<<0)|(HIGH<<2)|(LOW<<4))
#define LLH ((LOW<<0) |(LOW<<2) |(HIGH<<4))
#define HLH ((HIGH<<0)|(LOW<<2) |(HIGH<<4))
#define LHH ((LOW<<0) |(HIGH<<2)|(HIGH<<4))

#define III ((IN<<0)  |(IN<<2)  |(IN<<4))	// 300K for type determination

#define LIH	((LOW<<0) |(IN<<2)  |(HIGH<<4))	// 1K for orientation tests
#define HIL	((HIGH<<0)|(IN<<2)  |(LOW<<4))
#define ILH ((IN<<0)  |(LOW<<2) |(HIGH<<4))
#define IHL ((IN<<0)  |(HIGH<<2)|(LOW<<4))
#define LHI ((LOW<<0) |(HIGH<<2)|(IN<<4))
#define HLI ((HIGH<<0)|(LOW<<2) |(IN<<4)) 

#define HII ((HIGH<<0)|(IN<<2)  |(IN<<4))	// 300 K base for NPN orientation tests
#define IHI ((IN<<0)  |(HIGH<<2)|(IN<<4))
#define IIH ((IN<<0)  |(IN<<2)  |(HIGH<<4))

#define LII ((LOW<<0) |(IN<<2)  |(IN<<4))	// 300 K base for PNP orientation tests
#define ILI ((IN<<0)  |(LOW<<2) |(IN<<4))
#define IIL ((IN<<0)  |(IN<<2)  |(LOW<<4))

uint8_t adc1,adc2,adc3;	// Global variables for the ADC readings



//
//	Set the LED port pins correctly so the right (1..9)
//  Charlieplexed LED turns on.  Enter with 0 to turn
//  all LEDs off.
//
void SetLed(uint8_t ledNo) {
	TRISA4=1;			// Save some code by resetting the pins and just
	TRISA5=1;			// make the minimum changes later
	TRISC4=1;
	TRISC5=1;
	LATA4=0; 
	LATA5=0;  
	LATC4=0; 
	LATC5=0;

	switch(ledNo) {
		case 1:
			TRISA4=0; TRISA5=0; LATA5=1;
			break;
		case 2:
			TRISA4=0; TRISA5=0; LATA4=1; 
			break;
		case 3:	
			TRISA5=0; TRISC5=0; LATA5=1;
			break;
		case 4:			
			TRISA5=0; TRISC5=0; LATC5=1;
			break;
		case 5:
			TRISA5=0; TRISC4=0; LATA5=1; 
			break;
		case 6:			
			TRISA5=0; TRISC4=0; LATC4=1;
			break;
		case 7:
			TRISA4=0; TRISC5=0; LATA4=1;
			break;
		case 8:
			TRISA4=0; TRISC5=0; LATC5=1;
			break;
		case 9:
			TRISA4=0; TRISC4=0; LATA4=1;
			break;
	}
}


//
// Turn off the watchdog - used for shutdown mode
//
void WatchDogOff() {
	SWDTEN=0;			// Disable WTD
}


//
//  Set the watchdog timeout to 0.25 seconds and turn on
//  the watchdog. Used to sleep between individual measurements
//  to save power.
//
void WatchDogOn() {
	WDTPS4=0;			// 256 mS interval
	WDTPS3=1;	
	WDTPS2=0;
	WDTPS1=0;
	WDTPS0=0;
	SWDTEN=1;			// Enable WDT
}





void SetOne1K(uint8_t mode) {
	_AN(ONE1K)=0;	
	_TRIS(ONE1K)=0;	
	if (mode==LOW) _LAT(ONE1K)=0;
	if (mode==HIGH)_LAT(ONE1K)=1;
	if (mode==IN) {_AN(ONE1K)=1; _TRIS(ONE1K)=1;}
}

void SetTwo1K(uint8_t mode) {
	_AN(TWO1K)=0;	
	_TRIS(TWO1K)=0;
	if (mode==LOW) _LAT(TWO1K)=0;
	if (mode==HIGH) _LAT(TWO1K)=1;
	if (mode==IN) {_AN(TWO1K)=1; _TRIS(TWO1K)=1;}
}

void SetThree1K(uint8_t mode) {
	_AN(THREE1K)=0; 
	_TRIS(THREE1K)=0; 
	if (mode==LOW) _LAT(THREE1K)=0;
	if (mode==HIGH) _LAT(THREE1K)=1;
	if (mode==IN) {_AN(THREE1K)=1; _TRIS(THREE1K)=1;}
}


void SetOne300K(uint8_t mode) {
	_AN(ONE300K)=0;	
	_TRIS(ONE300K)=0;	
	if (mode==LOW)  _LAT(ONE300K)=0;
	if (mode==HIGH) _LAT(ONE300K)=1;
	if (mode==IN) {_AN(ONE300K)=1; _TRIS(ONE300K)=1;}
}

void SetTwo300K(uint8_t mode) {
	_AN(TWO300K)=0;	
	_TRIS(TWO300K)=0;	
	if (mode==LOW) _LAT(TWO300K)=0;
	if (mode==HIGH) _LAT(TWO300K)=1;
	if (mode==IN) {_AN(TWO300K)=1; _TRIS(TWO300K)=1;}
}

void SetThree300K(uint8_t mode) {
	_AN(THREE300K)=0; 
	_TRIS(THREE300K)=0; 
	if (mode==LOW) _LAT(THREE300K)=0;
	if (mode==HIGH) _LAT(THREE300K)=1;
	if (mode==IN) {_AN(THREE300K)=1; _TRIS(THREE300K)=1;}
}



//
//
//
uint8_t AnalogRead(uint8_t adcNo) {
	uint8_t result;

	ADCON0=(adcNo<<2) | 1;	// Select channel and enable ADC block
	ADCON1=0;				// Left justified, FOsc/2, Vref=Vdd
	__delay_ms(1);			// Wait aquire time
	GO_nDONE=1;				// Start ADC conversion
	while (GO_nDONE==1);	// Wait for conversion to be done
	result=ADRESH;
	ADCON0=0;				// Turn off ADC block
	return result;
}


//
//
//
uint8_t SetAll(uint8_t m1,uint8_t m2) {
	uint8_t v1,v2;

	SetOne1K(m1&0x03);
	SetTwo1K((m1>>2)&0x03);
	SetThree1K((m1>>4)&0x03);
	SetOne300K(m2&0x03);
	SetTwo300K((m2>>2)&0x03);
	SetThree300K((m2>>4)&0x03);

	adc1=AnalogRead(ONEADC);	
	adc2=AnalogRead(TWOADC);	
	adc3=AnalogRead(THREEADC);

	v1=128;v2=128;
	if ((m2&0b00000011)>0) {v1=adc2;v2=adc3;}
	if ((m2&0b00001100)>0) {v1=adc1;v2=adc3;}
	if ((m2&0b00110000)>0) {v1=adc1;v2=adc2;}
	if (v1>v2) return (v1-v2); 
	else return (v2-v1);

}


//
//
//
void SetMode(uint8_t mode) {
	if (mode==1) SetAll(HLL, III);	
	if (mode==2) SetAll(LHL, III);	
	if (mode==3) SetAll(HHL, III);	
	if (mode==4) SetAll(LLH, III);	
	if (mode==5) SetAll(HLH, III);	
	if (mode==6) SetAll(LHH, III);	
}





//
//
//
uint8_t Measure(void) {
	uint8_t	low1,low2,low3;
	uint8_t high1,high2,high3;
	uint8_t lows,highs;
	uint8_t result;
	uint8_t delta1,delta2;

	result=RES_NONE;
	low1=low2=low3=0;
	high1=high2=high3=0;

	for (uint8_t i=1; i<7; i++) {
		CLRWDT();
		SetMode(i);
		if (adc1<10) low1++;
		if (adc2<10) low2++;
		if (adc3<10) low3++;
		if (adc1>245) high1++;
		if (adc2>245) high2++;
		if (adc3>245) high3++;
	}
	CLRWDT();
	
	lows=low1+low2+low3;
	highs=high1+high2+high3;
	
	if ((lows==6) && (highs==6)) { // NFET Signature
		result=RES_NCH;
		// Handle this in the most simplistic way according to the measurements
		// of a 2N7000 NFET. This is probably not correct and will possibly not work
		// in all cases.
		if (low1==1) {
			if (low2==2) result+=RES_GDS;
			if (low2==3) result+=RES_DGS;
		}
		if (low1==2) {
			if (low2==1) result+=RES_SDG;
			if (low2==3) result+=RES_SGD;
		}
		if (low1==3) {
			if (low2==1) result+=RES_DSG;
			if (low2==2) result+=RES_GSD;
		}
	}	

	if ((lows==5) && (highs==4)) { // NPN Signature
		result=RES_NPN;
		if (low1==3) {
			delta1=SetAll(ILH, HII); 	// Test BEC
			delta2=SetAll(IHL, HII);	// Test BCE
			if (delta1<delta2) {
				result+=RES_BEC;
			} else {
				result+=RES_BCE;
			}
		}
		if (low2==3) {
			delta1=SetAll(LIH, IHI);	// Test EBC
			delta2=SetAll(HIL, IHI);	// Test CBE
			if (delta1<delta2) {
				result+=RES_EBC;
			} else {
				result+=RES_CBE;
			}
		}
		if (low3==3) {
			delta1=SetAll(LHI, IIH);	// Test ECB
			delta2=SetAll(HLI, IIH);	// Test CEB
			if (delta1<delta2) {
				result+=RES_ECB;
			} else {
				result+=RES_CEB;
			}
		}
		
	}

	//MPSA92 PNP  1=E 2=B 3=C
	if ((lows==4) && (highs==5)) { // PNP Signature
		result=RES_PNP;
		if (high1==3) {
			delta1=SetAll(ILH, LII); 	// Test BEC
			delta2=SetAll(IHL, LII);	// Test BCE
			if (delta1>delta2) {
				result+=RES_BEC;
			} else {
				result+=RES_BCE;
			}
		}
		if (high2==3) {
			delta1=SetAll(LIH, ILI);	// Test EBC
			delta2=SetAll(HIL, ILI);	// Test CBE
			if (delta1>delta2) {
				result+=RES_EBC;
			} else {
				result+=RES_CBE;
			}
		}
		if (high3==3) {
			delta1=SetAll(LHI, IIL);	// Test ECB
			delta2=SetAll(HLI, IIL);	// Test CEB
			if (delta1>delta2) {
				result+=RES_ECB;
			} else {
				result+=RES_CEB;
			}
		}
	}


#ifdef _1LCD 
	LCDCommand(0x01);	// Clear
	if (result>=100 && result<=199) {
		LCDData('N');
		LCDData('P');
		LCDHexAt(delta1,3);
		LCDHexAt(delta2,6);
		LCDCommand(0xD0);
		LCDHexAt(0xB0+base,8);
		LCDHexAt(0xE0+emitter,11);
		LCDHexAt(0xC0+collector,14);
	}
	if (result>=200) {
		LCDData('P');
		LCDData('N');
		LCDHexAt(delta1,3);
		LCDHexAt(delta2,6);
		LCDCommand(0xD0);
		LCDHexAt(0xB0+base,8);
		LCDHexAt(0xE0+emitter,11);
		LCDHexAt(0xC0+collector,14);
	}
#endif

	return result;
}



//
//
//
void Shutdown(void) {
	WatchDogOff();	// Turn of watchdog so it won't wake us up again
	
	SetLed(0);		// Flash the firmware version number on the LEDs before shutdown
	__delay_ms(300);
	SetLed(VERSION_MAJ);	
	__delay_ms(100);
	SetLed(0);	
	__delay_ms(100);
	SetLed(VERSION_MAJ);	
	__delay_ms(100);
	SetLed(0);	
	__delay_ms(300);
	SetLed(VERSION_MIN);	
	__delay_ms(100);
	SetLed(0);	
	__delay_ms(100);
	SetLed(VERSION_MIN);	
	__delay_ms(100);
	SetLed(0);	

//	VREGPM-1;	// Low power sleep mode (slow)
	ANSELA=0;	// Set all ports as digital
	ANSELC=0;
	TRISA=0;	// Set all ports as output
	TRISC=0;
	LATA=0;		// Set low state on all ports
	LATC=0;
	ADON=0;
	SLEEP();	// Deep sleep until Reset
}


//
//
//
void main(void) {
	uint8_t result;
	uint8_t timeout;

	OSCCON=0xFF;	//16 MHz
	ANSELA=0;
	ANSELC=0;
	nWPUEN=1;
	
#ifdef _1LCD	
	LCDInit();
	LCDCommand(0x01);	
#endif


#ifdef DEBUG_HILOCNT
	uint8_t	low1,low2,low3;
	uint8_t high1,high2,high3;
	uint8_t i,j;
	for (;;) {
		low1=low2=low3=0;
		high1=high2=high3=0;
		for (uint8_t i=1; i<7; i++) {
			CLRWDT();
			SetMode(i);
			if (adc1<10) low1++;
			if (adc2<10) low2++;
			if (adc3<10) low3++;
			if (adc1>245) high1++;
			if (adc2>245) high2++;
			if (adc3>245) high3++;
		}
	
		CLRWDT();
		SetLed(0);	for (j=0; j<100; j++) __delay_ms(10);
		CLRWDT();
		if (low1==0) low1=9; 
		SetLed(low1); __delay_ms(10);
		CLRWDT();
		SetLed(0);	for (j=0; j<30; j++) __delay_ms(10);
		CLRWDT();
		if (low2==0) low2=9; 
		SetLed(low2); __delay_ms(10);
		CLRWDT();
		SetLed(0);	for (j=0; j<30; j++) __delay_ms(10);
		CLRWDT();
		if (low3==0) low3=9; 
		SetLed(low3); __delay_ms(10);
		CLRWDT();
		SetLed(0);	for (j=0; j<30; j++) __delay_ms(10);
		CLRWDT();
		if (high1==0) high1=9; 
		SetLed(high1); __delay_ms(10);
		CLRWDT();
		SetLed(0);	for (j=0; j<30; j++) __delay_ms(10);
		CLRWDT();
		if (high2==0) high2=9; 
		SetLed(high2); __delay_ms(10);
		CLRWDT();
		SetLed(0);	for (j=0; j<30; j++) __delay_ms(10);
		CLRWDT();
		if (high3==0) high3=9; 
		SetLed(high3); __delay_ms(10);
		CLRWDT();
		SetLed(0);	for (j=0; j<30; j++) __delay_ms(10);
		CLRWDT();
	}	
#endif


	// Loop here forever - or at least until  we timeout and go into
	// permanent low-power sleep that can only be awakened by a reset
	timeout=TIMEOUT;
	for (;;) {
//		VREGPM=0; 	// Fast wakeup
		CLRWDT();
		WatchDogOn();
		// Try to identify the connected transistor
		result=Measure();
		CLRWDT();

		// If no transistor is found just briefly flash the power-on LED
		if (result==RES_NONE) {
			SetLed(LED_PWR);
		} else {
			// We have identified a transisor type and pinout, so flash the
			// right LEDs and also reset the timeout
			timeout=TIMEOUT;
			if (result&RES_NPN) SetLed(LED_NPN);
			if (result&RES_PNP) SetLed(LED_PNP);
			if (result&RES_NCH) SetLed(LED_NCH);
			__delay_ms(5);
			SetLed(result&0x0f);
			if (result&RES_NCH) {
				__delay_ms(5);
				SetLed(LED_PWR);
			}	
		}
		__delay_ms(5);
		SetLed(LED_OFF);

		// Check if it's time to shut down and go into a deep sleep
		// that will only be awakened by pressing the Reset/Power button
		if (--timeout==0) Shutdown();

		// Sleep for a few hundred mS to conserve power and then wake up by the watchdog
		// and make a new measurement
		CLRWDT();
		SLEEP();
	}


}
/*


				PIC16LF1503
				+-------v--------+
				|1 Vdd     Vss 14|
		  LedG1 |2 RA5     RA0 13| 300K TP1  R5  (AN0)
		  LedG2 |3 RA4     RA1 12| 300K TP2  R7  (AN1)
		  Reset |4 RA3     RA2 11|   1K TP1  R6  (AN2)
		  LedG4 |5 RC5     RC0 10|   1K TP2  R8  (AN4)
		  LedG3 |6 RC4     RC1  9| 300K TP3  R9  (AN5)
		  Spare |7 RC3     RC2  8|   1K TP3  R10 (AN6)
				+----------------+




RED=1 BLACK=2 BROWN=3

NPN BC547 FRONT Collector-Base-Emitter
======================================

BRK 311 022 Base=1  bec
KRB 311 022 Base=1  bce
RKB 131 202 Base=2  cbe
BKR 131 202 Base=2  ebc
KBR 113 220 Base=3  ecb
RBK 113 220 Base=3  ceb


PNP BC557 FRONT Collector-Base-Emitter
======================================
BRK                 bce
KRB                 bec
RKB                 ebc
BKR                 cbe
KBR                 ceb
RBK                 ecb


NFET 2N7000 FRONT Source-Gate-Drain (6/6)
=================================
RKB 231 132 sgd
BKR 132 231 dgs
KRB 321 312 gsd
BRK 312 321 dsg
RBK 213 123 sdg
KBR 123 213 gds
*/
