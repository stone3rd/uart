//#include "def.h"
#include "flash.h"
#include "c8051f340.h"

//===============================================================================================

//===============================================================================================
/****************flash写单字节**************************/   
void FLASH_ByteWrite (uint16 addr, uint8 byte)   
{   
   bit EA_SAVE = EA;                   // 保存中断状态   
   uint8 xdata * data pwrite;           // 写指针   
   EA = 0;                             // 关中断   
   VDM0CN = 0x80;                      //使能VDD监视器         
   RSTSRC = 0x02;                      //上电复位/VDD监视器复位标志     
   FLSCL=0X80;                         //使能FLASH单稳态定时器，FLASH读时间SYSCLK<=25MHz   
   pwrite = (uint8 xdata *) addr;       //把操作地址给些指针   
   PSCTL |= 0x01;                      //允许写 ,禁止擦      
   FLKEY  = 0xA5;                      //写入关键字         
   FLKEY  = 0xF1;                      //写入关键字   
   *pwrite = byte;                     //字节写入                      
   PSCTL &= ~0x01;                     //禁止写                     
   EA = EA_SAVE;                       //还原中断状态   
}   
   
/***************flash读单字节******************************/   
uint8 FLASH_ByteRead (uint16 addr)   
{   
   bit EA_SAVE = EA;                   // 保存中断状态   
   uint8 code * data pread;             // 读指针   
   uint8 byte;   
   EA = 0;                             // 关中断   
   FLSCL=0X80;                         // 使能FLASH单稳态定时器，FLASH读时间SYSCLK<=25MHz   
   pread = (uint8 code *) addr;         // 把操作地址给读指针   
   byte = *pread;                      // 读单个字节   
   EA = EA_SAVE;                       // 还原中断状态   
   return byte;   
}   
   
/*******************擦除一页 512字节***********************/   
void FLASH_PageErase (uint16 addr)   
{   
   bit EA_SAVE = EA;                   // 保存中断标志   
   uint8 xdata * data pwrite;           // FLASH write pointer 写指针   
   EA = 0;                             // 关中断   
   VDM0CN = 0x80;                      // 使能VDD监视器   
   RSTSRC = 0x02;                      // 上电复位VDD监视器复位标志   
   FLSCL=0X80;                         // 使能FLASH单稳态定时器，FLASH读时间SYSCLK<=25MHz   
   pwrite = (uint8 xdata *) addr;   
   FLKEY  = 0xA5;                      // 写入关键字   
   FLKEY  = 0xF1;                      // 写入关键字   
   PSCTL |= 0x03;                      //允许擦，允许写            
   *pwrite = 0;                        //擦除页的时候，向待擦除页内的任何一个地址写入一个数据字节。       
   PSCTL &= ~0x03;                     //禁止写，禁止擦          
   EA = EA_SAVE;                       // 还原中断状态   
} 