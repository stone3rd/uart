/************************************************************************************/
/* 实验平台：品诺电子C8051F340核心板
/* 功能说明：1、串口波特率设置为115200，使用外部晶振22.1184MHz；
			 2、按下S2则发送从串口发送"ABC"，同时点亮LED2；
			 3、按下S3则发送从串口发送"abc"，同时点亮LED3。
/* 作者信息：品诺电子(http://free-design.taobao.com)
/************************************************************************************/

#include "c8051f340.h"
#include "def.h"
#include "flash.h"
#include <string.h>

#define	BAUDRATE 115200 	//串口波特率，需要更改波特率时，在此更改数值即可
#define SYSCLK 22118400UL	//采用外部晶振时，系统的时钟频率为22118400hz

#define BUFF_SIZE  100

//===============================================================================================
typedef struct
{
	uint8 PhoneNumber[12];	//11位号码+'\0'
	uint8 Text[20];	//短信长度，先预设为20,实际短信可以70个汉字140个英文字母
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

sbit S2=P2^0;		   	// S ='0' 表示按键被按下
sbit S3=P2^1;			// S ='0' 表示按键被按下

sbit LED2=P2^2;		   	// LED ='1' 表示亮灯，LED ='0' 表示灭灯
sbit LED3=P2^3;		   	// LED ='1' 表示亮灯，LED ='0' 表示灭灯

bit uartSending=0;	 	//全局变量，标志串口是否正处于发送中(忙状态)

uint8 xdata UART_RX_BUFF[BUFF_SIZE];
uint8 xdata BuffIn = 0;

uint8 SMCnt[3];		//短信标号
SMS_t mysms;

//AT指令
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

void delay_1ms(unsigned int z) //延时函数
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
函数功能：将外部晶振配置为系统时钟。
入口参数：无。
返    回：无。
备    注：无。
********************************************************************/
void InitOSC(void)
{
/*
	P0MDIN &= ~0xC0;		//P0.6, P0.7配置为模拟输入，用作外部晶振
	P0SKIP |=  0xC0;		//交叉开关跳过P0.6, P0.7脚，用作外部晶振

	OSCXCN = (0x60 | 7);	//配置为外部晶体不分频，配置晶体频率为10M~30M
	Delay(1024);            //等待外部晶振开始工作，不可省略一定要做的步骤
	while (!(OSCXCN & 0x80));	//一直读取标志位，直到外部晶体稳定
	RSTSRC = 0x06;			// Enable missing clock detector and VDD Monitor reset

	CLKSEL = 0x01;			//选择外部晶体为时钟
	OSCICN = 0x00;			//关闭内部晶振
*/
	OSCICN |= 0x03;
	RSTSRC  = 0x04;
}
//////////////////////////Pino Electronics///////////////////////////

/********************************************************************
函数功能：初始化串口0。
入口参数：无。
返    回：无。
备    注：无。
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

	/*初始化定时器1*/
	TMOD|=0x20;			//定时器1工作在8位自动重装方式
	TL1=0;
	TR1=1;
	
	/*初始化串口*/
	S0MODE=0;			//8位的UART工作方式
	REN0=1;				//接收允许
	PS0=1;				//为了在中断里输出字符，串口的优先级设为最高
	ES0=1;				//打开串口中断

	/*配置引脚*/
	P0MDOUT |= 0x10;	//串口TXD引脚配置为推挽输出方式，用于UART发送端
	XBR0|=0x01;			//在外设连接寄存器0上把UART0连接到引脚P0.4和P0.5
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
	while(uartSending); //等待发送完毕
	SBUF0=byte; //将数据写入到串口缓冲
	uartSending=1;	 //设置发送标志	
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
短信内容，以十六进制0x1A结束。
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
*@brief    给指定号码打电话   
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
*@brief:读取短信内容
*
*@param    index-短信编号
*@param    mode-读取短信模式，0-正常，1-不改变指定消息记录状态
*@param    psms,短信分析内容存储数据结构
*
*@return:	0-读取成功，1-读取失败 
*/
uint8 AnalyzeMsg(uint8 index, uint8 mode, SMS_t *psms)
{
/*提取的短信内容格式如下：
AT+CMGR=13,0

+CMGR: "REC READ","+8615261172865","","15/07/11,20:52:52+32"
hello
*/
	//AT+CMGR=xxx,0\r\0    长度：15
	uint8 at_cmgr[15] = {"AT+CMGR="};
	uint8 len[4];	//可能为3位数
	uint8 m[2];
	uint8 res = 0,i = 0;
	uint8 * ptr;

	len[0] = index/100 + 48; len[1] = (index/10)%10 + 48; len[2] = index%10 + 48; len[3] = '\0';
	m[0] = mode + 48; m[1] = '\0';

	strcat(at_cmgr,len);	//添加短信标识
	strcat(at_cmgr,",");
	strcat(at_cmgr,m);	//mode
	strcat(at_cmgr,"\r");	//这里不能使用单引号

	sim900a_send_cmd(at_cmgr,"OK",200);
	
	//等待sim900a返回短信内容
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
		//获取号码
		ptr = strstr(UART_RX_BUFF,",");	//这里是按照我的标准，输入完整的查询短信命令
		ptr = strstr(ptr+1,",");
		ptr += 5;
	
		//这里暂时只考虑了11位的手机号码的情况。对于错误输入没有进行判断,鲁棒性不佳
		for(i = 0; i < 11; i++)
		{
			*(psms->PhoneNumber + i) = *(ptr + i);	
		}
		*(psms->PhoneNumber + i) = '\0';

		//获取短信内容	
		ptr = strrchr(UART_RX_BUFF,'"');	//查找UART中最后一个“号的位置
		ptr += 3;i = 0;
		
		//这里对于短信的内容也没有进行一个严格的输入判断，存在风险
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
	


	PCA0MD &= ~0x40;	//WDTE = 0 关闭看门狗，上电默认打开
	EA=0;

	/*配置引脚*/
	P2MDOUT=0xC;		//P2.2和P2.3设为推挽输出方式，用于点亮LED
	XBR1= 0x40;			//打开交叉开关，使能弱上拉，要使GPIO生效，必须打开交叉开关

	InitOSC();			//外部晶振配置
	InitUart0();		//初始化串口
	EA=1;				//允许所有中断	
	
	
	sim900a_send_cmd(AT_CMGF,"OK",50);//设置为文本模式	 
	sim900a_send_cmd(AT_CSCS,"OK",50);//设置GSM字符集
	sim900a_send_cmd(AT_CNMI,"OK",50);//设置新消息提示
	delay_1ms(500);	
	
	//readmsg();
	//resetbuf();

	AnalyzeMsg(40,0,&mysms);
	SendEnMessage(mysms.PhoneNumber,mysms.Text);	//发送一条英文短信
	while (1)
	{

		 #if 1
		//检测是否有短信发送过来
		//提示内容eg:+CMTI: "SM",14
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
			//程序启动，LED3闪烁提示
			LED3 ^= 1;
			delay_1ms(1000);
		}
		#endif
		//按键s2
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

		//按键s3
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
函数功能：串口中断处理。
入口参数：无。
返    回：无。
备    注：无。
********************************************************************/
/*串口中断服务程序*/
void Uart0ISR(void) interrupt 4 
{
	if(RI0 == 1)
	{
		RI0 = 0;
		UART_RX_BUFF[BuffIn] = 	SBUF0;
		BuffIn = (BuffIn + 1)%BUFF_SIZE;
	}
	if(TI0 ==1)	//发送数据
	{
		TI0 = 0;
		uartSending = 0;
	}
}	   	     
//////////////////////////Pino Electronics///////////////////////////
