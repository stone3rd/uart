/************************************************************************************/
/* ʵ��ƽ̨��Ʒŵ����C8051F340���İ�
/* ����˵����1�����ڲ���������Ϊ115200��ʹ���ⲿ����22.1184MHz��
			 2������S2���ʹӴ��ڷ���"ABC"��ͬʱ����LED2��
			 3������S3���ʹӴ��ڷ���"abc"��ͬʱ����LED3��
/* ������Ϣ��Ʒŵ����(http://free-design.taobao.com)
/************************************************************************************/

#include "c8051f340.h"
#include "def.h"
#include "flash.h"
#include <string.h>

#define	BAUDRATE 115200 	//���ڲ����ʣ���Ҫ���Ĳ�����ʱ���ڴ˸�����ֵ����
#define SYSCLK 22118400UL	//�����ⲿ����ʱ��ϵͳ��ʱ��Ƶ��Ϊ22118400hz

#define BUFF_SIZE  100

//===============================================================================================
typedef struct
{
	uint8 PhoneNumber[12];	//11λ����+'\0'
	uint8 Text[20];	//���ų��ȣ���Ԥ��Ϊ20,ʵ�ʶ��ſ���70������140��Ӣ����ĸ
}SMS_t;

//===============================================================================================
static void Delay (unsigned int x);
static void delay_1ms(unsigned int z);
void InitOSC(void);
void InitUart0(void);

uint8 sim900a_send_cmd(uint8 *cmd, uint8 *ack, uint16 waittime);
void send_byte(uint8 byte);
uint8 AnalyzeMsg(uint8 index, uint8 mode, SMS_t *psms);

//===============================================================================================

sbit S2=P2^0;		   	// S ='0' ��ʾ����������
sbit S3=P2^1;			// S ='0' ��ʾ����������

sbit LED2=P2^2;		   	// LED ='1' ��ʾ���ƣ�LED ='0' ��ʾ���
sbit LED3=P2^3;		   	// LED ='1' ��ʾ���ƣ�LED ='0' ��ʾ���

bit uartSending=0;	 	//ȫ�ֱ�������־�����Ƿ������ڷ�����(æ״̬)

uint8 xdata UART_RX_BUFF[BUFF_SIZE];
uint8 xdata BuffIn = 0;

uint8 SMCnt[3];		//���ű��
SMS_t mysms;

//ATָ��
uint8 code ATD[] = {"ATD"};
uint8 code AT[] = {"AT\r"}; 
uint8 code AT_CMGF[] = {"AT+CMGF=1\r"};
uint8 code AT_CSCS[] = {"AT+CSCS=\"GSM\"\r"};
uint8 code AT_CNMI[] = {"AT+CNMI=2,1,\r"};
uint8 code AT_Phone[] = {"AT+CMGS=\"15261172865\"\r"};
uint8 code AT_Msg[]={'h','e','l','l','o',0x1A,'\r','\0'};
uint8 code AT_CMGR[]={"AT+CMGR="};
//===============================================================================================
void Delay (unsigned int x)
{
	while(--x);
}

void delay_1ms(unsigned int z) //��ʱ����
{
	unsigned int x,y;
	for(x=z;x>0;x--)
		for(y=110;y>0;y--);
}

void resetbuf(void)
{
	uint8 i;
	for(i = 0; i < BUFF_SIZE; i++)
	{
		 UART_RX_BUFF[i] = 0;
	}
	BuffIn = 0;
}
/********************************************************************
�������ܣ����ⲿ��������Ϊϵͳʱ�ӡ�
��ڲ������ޡ�
��    �أ��ޡ�
��    ע���ޡ�
********************************************************************/
void InitOSC(void)
{
/*
	P0MDIN &= ~0xC0;		//P0.6, P0.7����Ϊģ�����룬�����ⲿ����
	P0SKIP |=  0xC0;		//���濪������P0.6, P0.7�ţ������ⲿ����

	OSCXCN = (0x60 | 7);	//����Ϊ�ⲿ���岻��Ƶ�����þ���Ƶ��Ϊ10M~30M
	Delay(1024);            //�ȴ��ⲿ����ʼ����������ʡ��һ��Ҫ���Ĳ���
	while (!(OSCXCN & 0x80));	//һֱ��ȡ��־λ��ֱ���ⲿ�����ȶ�
	RSTSRC = 0x06;			// Enable missing clock detector and VDD Monitor reset

	CLKSEL = 0x01;			//ѡ���ⲿ����Ϊʱ��
	OSCICN = 0x00;			//�ر��ڲ�����
*/
	OSCICN |= 0x03;
	RSTSRC  = 0x04;
}
//////////////////////////Pino Electronics///////////////////////////

/********************************************************************
�������ܣ���ʼ������0��
��ڲ������ޡ�
��    �أ��ޡ�
��    ע���ޡ�
********************************************************************/
void InitUart0(void)
{
	if (SYSCLK/BAUDRATE/2/256 < 1) {
		TH1 = -(SYSCLK/BAUDRATE/2);
		CKCON &= ~0x0B;                  // T1M = 1; SCA1:0 = xx
		CKCON |=  0x08;
	} else if (SYSCLK/BAUDRATE/2/256 < 4) {
		TH1 = -(SYSCLK/BAUDRATE/2/4);
		CKCON &= ~0x0B;                  // T1M = 0; SCA1:0 = 01
		CKCON |=  0x01;
	} else if (SYSCLK/BAUDRATE/2/256 < 12) {
		TH1 = -(SYSCLK/BAUDRATE/2/12);
		CKCON &= ~0x0B;                  // T1M = 0; SCA1:0 = 00
	} else {
		TH1 = -(SYSCLK/BAUDRATE/2/48);
		CKCON &= ~0x0B;                  // T1M = 0; SCA1:0 = 10
		CKCON |=  0x02;
	}

	/*��ʼ����ʱ��1*/
	TMOD|=0x20;			//��ʱ��1������8λ�Զ���װ��ʽ
	TL1=0;
	TR1=1;
	
	/*��ʼ������*/
	S0MODE=0;			//8λ��UART������ʽ
	REN0=1;				//��������
	PS0=1;				//Ϊ�����ж�������ַ������ڵ����ȼ���Ϊ���
	ES0=1;				//�򿪴����ж�

	/*��������*/
	P0MDOUT |= 0x10;	//����TXD��������Ϊ���������ʽ������UART���Ͷ�
	XBR0|=0x01;			//���������ӼĴ���0�ϰ�UART0���ӵ�����P0.4��P0.5
}

uint8 sim900a_send_cmd(uint8 *cmd, uint8 *ack, uint16 waittime)
{
	uint8 res = 0;

	resetbuf();

	while((*cmd) != '\0')
	{
		send_byte(*cmd);
		cmd++;
	}
	if(ack&&waittime)
	{
		while(--waittime)
		{
			delay_1ms(10);
			if(strstr(UART_RX_BUFF,ack))
			{
				break;
			}
		}
		if(waittime == 0)
			res = 1;
	}
	return res;
}


/*
*
*send a byte to uart
*/
void send_byte(uint8 byte)
{
	while(uartSending); //�ȴ��������
	SBUF0=byte; //������д�뵽���ڻ���
	uartSending=1;	 //���÷��ͱ�־	
}


/*
*@fn    SendEnMessage
*
*@brief
*
*@param
*
*@return
*/

void SendEnMessage(uint8 *phonenumber, uint8 *pText)
{
/*
AT+CSCS="GSM"
AT+CMGF=1
AT+CMGS="15261172865"
�������ݣ���ʮ������0x1A������
*/
	uint8 textend[3] = {0x1A,'\r','\0'};
	uint8 pnend[3] = {'"','\r','\0'};

	uint8 at_cmgs[22] = {"At+CMGS=\""};

	sim900a_send_cmd(AT_CSCS,"OK",50);
	sim900a_send_cmd("AT+CMGF=1\r","OK",50);

	strcat(at_cmgs,phonenumber);
	strcat(at_cmgs,pnend);
	sim900a_send_cmd(at_cmgs,"OK",50);

   	strcat(pText,textend);
	sim900a_send_cmd(pText,"OK",50);
}


/*
*@fn    CallUp
*
*@brief    ��ָ�������绰   
*
*@param    
*
*@return
*
*/
/*
void CallUp(uint8 *phonenumber)
{
	uint8 atd[17]= {'A','T','D','\0'};

	strcat(atd,phonenumber);
	strcat(atd,";\r");
	
	sim900a_send_cmd(atd,"OK",50);
}
*/
/*
*@fn	AnalyzeMsg
*
*@brief:��ȡ��������
*
*@param    index-���ű��
*@param    mode-��ȡ����ģʽ��0-������1-���ı�ָ����Ϣ��¼״̬
*@param    psms,���ŷ������ݴ洢���ݽṹ
*
*@return:	0-��ȡ�ɹ���1-��ȡʧ�� 
*/
uint8 AnalyzeMsg(uint8 index, uint8 mode, SMS_t *psms)
{
/*��ȡ�Ķ������ݸ�ʽ���£�
AT+CMGR=13,0

+CMGR: "REC READ","+8615261172865","","15/07/11,20:52:52+32"
hello
*/
	//AT+CMGR=xxx,0\r\0    ���ȣ�15
	uint8 at_cmgr[15] = {"AT+CMGR="};
	uint8 len[4];	//����Ϊ3λ��
	uint8 m[2];
	uint8 res = 0,i = 0;
	uint8 * ptr;

	len[0] = index/100 + 48; len[1] = (index/10)%10 + 48; len[2] = index%10 + 48; len[3] = '\0';
	m[0] = mode + 48; m[1] = '\0';

	strcat(at_cmgr,len);	//��Ӷ��ű�ʶ
	strcat(at_cmgr,",");
	strcat(at_cmgr,m);	//mode
	strcat(at_cmgr,"\r");	//���ﲻ��ʹ�õ�����

	sim900a_send_cmd(at_cmgr,"OK",200);
	
	//�ȴ�sim900a���ض�������
	while(1)
	{
		if(strstr(UART_RX_BUFF,"OK"))
		{
			break;
		}
		if(strstr(UART_RX_BUFF,"ERROR"))
		{
			res = 1;
			break;
		}
	}

	if(res != 1)
	{
		//��ȡ����
		ptr = strstr(UART_RX_BUFF,",");	//�����ǰ����ҵı�׼�����������Ĳ�ѯ��������
		ptr = strstr(ptr+1,",");
		ptr += 5;
	
		//������ʱֻ������11λ���ֻ��������������ڴ�������û�н����ж�,³���Բ���
		for(i = 0; i < 11; i++)
		{
			*(psms->PhoneNumber + i) = *(ptr + i);	
		}
		*(psms->PhoneNumber + i) = '\0';

		//��ȡ��������	
		ptr = strrchr(UART_RX_BUFF,'"');	//����UART�����һ�����ŵ�λ��
		ptr += 3;i = 0;
		
		//������ڶ��ŵ�����Ҳû�н���һ���ϸ�������жϣ����ڷ���
		while(*(ptr + i) != 0x0D)
		{
			*(psms->Text + i) = *(ptr + i);
			i++;
		}
		*(psms->Text + i) = '\0';
	}
	return res;	
}

int main(void)
{
	bit key2_down_flag=0;
	bit key3_down_flag=0;
	uint8 i = 0,getmsg = 0;
	uint8 *smflag = NULL;
	

	//uint8 callflag = 0;
	


	PCA0MD &= ~0x40;	//WDTE = 0 �رտ��Ź����ϵ�Ĭ�ϴ�
	EA=0;

	/*��������*/
	P2MDOUT=0xC;		//P2.2��P2.3��Ϊ���������ʽ�����ڵ���LED
	XBR1= 0x40;			//�򿪽��濪�أ�ʹ����������ҪʹGPIO��Ч������򿪽��濪��

	InitOSC();			//�ⲿ��������
	InitUart0();		//��ʼ������
	EA=1;				//���������ж�	
	
	
	sim900a_send_cmd(AT_CMGF,"OK",50);//����Ϊ�ı�ģʽ	 
	sim900a_send_cmd(AT_CSCS,"OK",50);//����GSM�ַ���
	sim900a_send_cmd(AT_CNMI,"OK",50);//��������Ϣ��ʾ
	delay_1ms(500);	
	
	//readmsg();
	//resetbuf();

	AnalyzeMsg(40,0,&mysms);
	SendEnMessage(mysms.PhoneNumber,mysms.Text);	//����һ��Ӣ�Ķ���
	while (1)
	{

		 #if 1
		//����Ƿ��ж��ŷ��͹���
		//��ʾ����eg:+CMTI: "SM",14
		if(getmsg == 0)
		{
			smflag = strstr(UART_RX_BUFF,"SM");
			delay_1ms(100);
			SMCnt[0] = *(smflag + 4);
			SMCnt[1] = *(smflag + 5);
			SMCnt[2] = '\0';
			
		}
		if(smflag != 0)
		{
			getmsg = 1;
			//����������LED3��˸��ʾ
			LED3 ^= 1;
			delay_1ms(1000);
		}
		#endif
		//����s2
		if (S2 == 0)
		{
			Delay(3000);
			if(S2 == 0)
			{
				if(key2_down_flag == 0)
				{
					LED2^=1;
					key2_down_flag = 1;
				}
			} 
		}
		else
		{
			key2_down_flag = 0;
		}

		//����s3
		if (S3 == 0)
		{
			Delay(3000);
			if(S3 == 0)
			{
				if(key3_down_flag == 0)
				{
					for(i = 0; i < 10; i++)
					{
						send_byte(UART_RX_BUFF[i]);
					}
					i = 0;
					key3_down_flag = 1;
				}
			}
		}
		else
		{
			key3_down_flag = 0;
		}
	}// end of while(1)   
	
	return 0;                 
}
//////////////////////////Pino Electronics///////////////////////////

/********************************************************************
�������ܣ������жϴ���
��ڲ������ޡ�
��    �أ��ޡ�
��    ע���ޡ�
********************************************************************/
/*�����жϷ������*/
void Uart0ISR(void) interrupt 4 
{
	if(RI0 == 1)
	{
		RI0 = 0;
		UART_RX_BUFF[BuffIn] = 	SBUF0;
		BuffIn = (BuffIn + 1)%BUFF_SIZE;
	}
	if(TI0 ==1)	//��������
	{
		TI0 = 0;
		uartSending = 0;
	}
}	   	     
//////////////////////////Pino Electronics///////////////////////////
