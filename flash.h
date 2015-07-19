#ifndef FLASH_H
#define FLASH_H

#ifndef FLASH_PAGESIZE
#define FLASH_PAGESIZE 512
#endif


extern void FLASH_ByteWrite(unsigned int addr, char byte);
extern void FLASH_StringWrite(unsigned int addr, char *str, char len);
extern unsigned char FLASH_ByteRead(unsigned int addr);
extern void FLASH_PageErase(unsigned int addr);


#endif
