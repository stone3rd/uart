//#include "def.h"
#include "flash.h"
#include "c8051f340.h"

//===============================================================================================

//===============================================================================================
/****************flashд���ֽ�**************************/   
void FLASH_ByteWrite (uint16 addr, uint8 byte)   
{   
   bit EA_SAVE = EA;                   // �����ж�״̬   
   uint8 xdata * data pwrite;           // дָ��   
   EA = 0;                             // ���ж�   
   VDM0CN = 0x80;                      //ʹ��VDD������         
   RSTSRC = 0x02;                      //�ϵ縴λ/VDD��������λ��־     
   FLSCL=0X80;                         //ʹ��FLASH����̬��ʱ����FLASH��ʱ��SYSCLK<=25MHz   
   pwrite = (uint8 xdata *) addr;       //�Ѳ�����ַ��Щָ��   
   PSCTL |= 0x01;                      //����д ,��ֹ��      
   FLKEY  = 0xA5;                      //д��ؼ���         
   FLKEY  = 0xF1;                      //д��ؼ���   
   *pwrite = byte;                     //�ֽ�д��                      
   PSCTL &= ~0x01;                     //��ֹд                     
   EA = EA_SAVE;                       //��ԭ�ж�״̬   
}   
   
/***************flash�����ֽ�******************************/   
uint8 FLASH_ByteRead (uint16 addr)   
{   
   bit EA_SAVE = EA;                   // �����ж�״̬   
   uint8 code * data pread;             // ��ָ��   
   uint8 byte;   
   EA = 0;                             // ���ж�   
   FLSCL=0X80;                         // ʹ��FLASH����̬��ʱ����FLASH��ʱ��SYSCLK<=25MHz   
   pread = (uint8 code *) addr;         // �Ѳ�����ַ����ָ��   
   byte = *pread;                      // �������ֽ�   
   EA = EA_SAVE;                       // ��ԭ�ж�״̬   
   return byte;   
}   
   
/*******************����һҳ 512�ֽ�***********************/   
void FLASH_PageErase (uint16 addr)   
{   
   bit EA_SAVE = EA;                   // �����жϱ�־   
   uint8 xdata * data pwrite;           // FLASH write pointer дָ��   
   EA = 0;                             // ���ж�   
   VDM0CN = 0x80;                      // ʹ��VDD������   
   RSTSRC = 0x02;                      // �ϵ縴λVDD��������λ��־   
   FLSCL=0X80;                         // ʹ��FLASH����̬��ʱ����FLASH��ʱ��SYSCLK<=25MHz   
   pwrite = (uint8 xdata *) addr;   
   FLKEY  = 0xA5;                      // д��ؼ���   
   FLKEY  = 0xF1;                      // д��ؼ���   
   PSCTL |= 0x03;                      //�����������д            
   *pwrite = 0;                        //����ҳ��ʱ���������ҳ�ڵ��κ�һ����ַд��һ�������ֽڡ�       
   PSCTL &= ~0x03;                     //��ֹд����ֹ��          
   EA = EA_SAVE;                       // ��ԭ�ж�״̬   
} 