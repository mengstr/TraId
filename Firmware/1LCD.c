#include "1LCD.h"

void LCD1(void);
void LCD0(void);
void LCDLatch(void);
void LCDNybble(uint8_t, uint8_t);
void LCDData(uint8_t);
void LCDCommand(uint8_t);
void LCDInit(void);


void LCD1(void) {
	LCDlow;
	__delay_us(1);
	LCDhigh;
	__delay_us(30);
}


void LCD0(void) {
	LCDlow;
	__delay_us(20);
	LCDhigh;
	__delay_us(150);
}


void LCDLatch(void) {
	LCDlow;
	__delay_us(250);
	LCDhigh;
	__delay_us(500);
}


void LCDNybble(uint8_t nybble, uint8_t isData) {
	if (LCDbl) LCD1(); else LCD0();
	LCD1(); // E high
	if (isData) LCD1(); else LCD0();
	if (nybble&0x08) LCD1(); else LCD0();
	if (nybble&0x04) LCD1(); else LCD0();
	if (nybble&0x02) LCD1(); else LCD0();
	if (nybble&0x01) LCD1(); else LCD0();
	LCDLatch();
	
	if (LCDbl) LCD1(); else LCD0();
	LCD0(); // E low
	if (isData) LCD1(); else LCD0();
	if (nybble&0x08) LCD1(); else LCD0();
	if (nybble&0x04) LCD1(); else LCD0();
	if (nybble&0x02) LCD1(); else LCD0();
	if (nybble&0x01) LCD1(); else LCD0();
	LCDLatch();
}

void LCDData(uint8_t d) {
	LCDNybble(d>>4, 1);
	LCDNybble(d, 1);
}

void LCDCommand(uint8_t d) {
	LCDNybble(d>>4, 0);
	LCDNybble(d, 0);
}



void LCDInit(void) {
  	uint8_t initseq[] = {0x03, 0x03, 0x03, 0x02, 0x28, 0x0c, 0x06, 0x14, 0x01};
  	LCDout;
  	LCDhigh;
	__delay_ms(50);
	__delay_ms(50);
	for (uint8_t i=0; i<sizeof(initseq); i++) {
		LCDCommand(initseq[i]);
		__delay_ms(50);
		__delay_ms(50);
	}

}


uint8_t LCDButton(void) {
	uint8_t pressed=0;

	LCDin;
	LCDlow;
	if (!LCDread) pressed=1;
    LCDout;
    LCDhigh;
    __delay_us(50);
	return pressed;
}



void LCDHexAt(uint8_t v, uint8_t pos) {
	uint8_t tmp;

	if (pos<8) LCDCommand(0x80+pos);	
	else LCDCommand(0xB8+pos);
	
	tmp='0'+(v>>4);
	if (tmp>'9') tmp+=7;
	LCDData(tmp);

	tmp='0'+(v&0x0F);
	if (tmp>'9') tmp+=7;
	LCDData(tmp);
}
