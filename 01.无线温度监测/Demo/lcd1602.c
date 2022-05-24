#include<reg51.h>
#include"lcd1602.h"
#include"delay.h"

#define uchar unsigned char
#define uint unsigned int 

sbit RS = P2^7;//1602数据/命令选择端（H：数据寄存器L：指令寄存器） 
sbit RW = P2^6;//1602读/写选择端 
sbit E  = P2^5;//1602使能信号端

uchar code tab[]={'0','1','2','3','4','5','6','7','8','9'};//液晶显示
uchar tab1[]={"min:010  max:040"};//液晶第二行显示内容
uchar  str[8];
/******************************************************************/
/*                   LCD1602写命令操作                            */
/******************************************************************/
void WriteCommand(uchar com)
{
	delay(5);//操作前短暂延时，保证信号稳定
	E=0;
	RS=0;
	RW=0;

	P0=com;
	E=1;
	delay(5);
	E=0;
}
/******************************************************************/
/*                   LCD1602写数据操作                            */
/******************************************************************/
void WriteData(uchar dat)
{
	delay(5);  //操作前短暂延时，保证信号稳定
	E=0;
	RS=1;
	RW=0;

	P0=dat;
	E=1;
	delay(5);
	E=0;
}
/******************************************************************/
/*                   LCD1602初始化程序                            */
/******************************************************************/
void InitLcd()
{
	uchar i;
	delay(15);
//	WriteCommand(0x38); //display mode
//	WriteCommand(0x38); //display mode
	WriteCommand(0x38); //display mode
	WriteCommand(0x06); //显示光标移动位置
	WriteCommand(0x0c); //显示开及光标设置
	WriteCommand(0x01); //显示清屏
	WriteCommand(0x80+0x40);//将光标移到第二行
	for(i=0;i<16;i++)//显示初始化内容
	{
		WriteData(tab1[i]);
		delay(10);
	}
}