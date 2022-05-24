#include<reg51.h>
#include"lcd1602.h"
#include"delay.h"

#define uchar unsigned char
#define uint unsigned int 

sbit RS = P2^7;//1602����/����ѡ��ˣ�H�����ݼĴ���L��ָ��Ĵ����� 
sbit RW = P2^6;//1602��/дѡ��� 
sbit E  = P2^5;//1602ʹ���źŶ�

uchar code tab[]={'0','1','2','3','4','5','6','7','8','9'};//Һ����ʾ
uchar tab1[]={"min:010  max:040"};//Һ���ڶ�����ʾ����
uchar  str[8];
/******************************************************************/
/*                   LCD1602д�������                            */
/******************************************************************/
void WriteCommand(uchar com)
{
	delay(5);//����ǰ������ʱ����֤�ź��ȶ�
	E=0;
	RS=0;
	RW=0;

	P0=com;
	E=1;
	delay(5);
	E=0;
}
/******************************************************************/
/*                   LCD1602д���ݲ���                            */
/******************************************************************/
void WriteData(uchar dat)
{
	delay(5);  //����ǰ������ʱ����֤�ź��ȶ�
	E=0;
	RS=1;
	RW=0;

	P0=dat;
	E=1;
	delay(5);
	E=0;
}
/******************************************************************/
/*                   LCD1602��ʼ������                            */
/******************************************************************/
void InitLcd()
{
	uchar i;
	delay(15);
//	WriteCommand(0x38); //display mode
//	WriteCommand(0x38); //display mode
	WriteCommand(0x38); //display mode
	WriteCommand(0x06); //��ʾ����ƶ�λ��
	WriteCommand(0x0c); //��ʾ�����������
	WriteCommand(0x01); //��ʾ����
	WriteCommand(0x80+0x40);//������Ƶ��ڶ���
	for(i=0;i<16;i++)//��ʾ��ʼ������
	{
		WriteData(tab1[i]);
		delay(10);
	}
}