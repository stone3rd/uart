#ifndef FLASH_H
#define FLASH_H

#include "def.h"

#ifndef FLASH_PAGESIZE   
#define FLASH_PAGESIZE 512   
#endif

extern void FLASH_ByteWrite (uint16 addr, uint8 byte);   
extern uint8 FLASH_ByteRead (uint16 addr);   
extern void FLASH_PageErase (uint16 addr); 

#endif
