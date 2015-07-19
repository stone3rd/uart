#include "Flash.h"
#include "c8051F340.h"


//void FLASH_ByteWrite(unsigned int addr, char byte);
//unsigned char FLASH_ByteRead(unsigned int addr);
//void FLASH_PageErase(unsigned int addr);

void FLASH_ByteWrite(unsigned int addr, char byte)
{
	bit EA_SAVE = EA;
	char xdata *data pwrite;	//FLASH write pointer
	
	EA = 0;
	
	VDM0CN = 0x80;	//Enable VDD monitor
	
	RSTSRC = 0x02;	//Enable VDD monitor as a reset source
	
	pwrite = (char xdata *)addr;
	
	FLKEY = 0xA5;
	FLKEY = 0xF1;
	PSCTL |= 0x01;	//PSWE = 1;
	
	VDM0CN = 0x80;
	
	RSTSRC = 0x02;
	
	*pwrite = byte;
	
	PSCTL &= ~0x01;
	
	EA = EA_SAVE;
}

void FLASH_StringWrite(unsigned int addr, char *str, char len)
{
	unsigned int addroffset = addr;
	unsigned char i = 0;

	for(i = 0; i < len; i++)
	{
		FLASH_ByteWrite(addroffset,*(str + i));
		addroffset++;
	}
}

unsigned char FLASH_ByteRead(unsigned int addr)
{
	bit EA_SAVE = EA;
	char code *data pread;
	unsigned char byte;
	
	EA = 0;
	
	pread = (char code *)addr;
	
	byte = *pread;
	
	EA = EA_SAVE;
	
	return byte;
}

void FLASH_PageErase(unsigned int addr)
{
	bit EA_SAVE = EA;
	char xdata *data pwrite;
	
	EA = 0;
	
	VDM0CN = 0x80;
	
	RSTSRC = 0x02;
	
	pwrite = (char xdata *)addr;
	
	FLKEY = 0xA5;
	FLKEY = 0xF1;
	PSCTL |= 0x03;
	
	VDM0CN = 0x80;
	
	RSTSRC = 0x02;
	*pwrite = 0;
	
	PSCTL &= ~0x03;
	
	EA= EA_SAVE;
}

