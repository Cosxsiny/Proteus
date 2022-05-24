#include<reg51.h>     //����ͷ�ļ���һ���������Ҫ�Ķ���ͷ�ļ��������⹦�ܼĴ����Ķ���
#include<math.h>
#include<INTRINS.H>
#include"lcd1602.h"
#include"ds18b20.h"
#include"delay.h"

#define uchar unsigned char
#define uint unsigned int

sbit buzzer=P2^1;//����������
sbit rel=P2^3;

int temp;//����¶�
char temp_max=40,temp_min=10;//�趨���¶� 
char TempH,TempL;
uchar CNCHAR[6] = "��  ";

/*******************************************************************************
* �� �� ��         :UsartConfiguration()
* ��������		   :���ô���
* ��    ��         : ��
* ��    ��         : ��
*******************************************************************************/

void UsartConfiguration()
{
	SCON=0X50;			//����Ϊ������ʽ1
	TMOD=0X20;			//���ü�����������ʽ2
	PCON=0X80;			//�����ʼӱ�
	TH1=0XF3;				//��������ʼֵ���ã�ע�Ⲩ������4800��
	TL1=0XF3;
	ES=1;						//�򿪽����ж�
	EA=1;						//�����ж�
	TR1=1;					//�򿪼�����
}

/******************************************************************/
/*                  ��������������                                */
/******************************************************************/
void alarm()
{
	if(TempH>=temp_max)//���¶ȸ������õ�����±���
	{	
		buzzer=0;
		rel=0;
//		delay(50);
//		buzzer=1;
//		delay(50);
	}
	else if(TempH<temp_min)//���¶ȵ������õ�����±���
	{
		buzzer=0;
		rel=0;
//		delay(50);
//		buzzer=1;
//		delay(50);
	}
	else//�������
	{
	 	buzzer=1;
		rel=1;
	}
}
/******************************************************************/
/*                   ��ʾ��õ��¶�                               */
/******************************************************************/
void handle_T()
{
	
	uchar i,j,m;
	str[0]=0x20;//��ʾΪ��
	str[1]=tab[TempH/100]; //��λ�¶�
  str[2]=tab[(TempH%100)/10]; //ʮλ�¶�
  str[3]=tab[(TempH%100)%10]; //��λ�¶�,��С����
  str[5]=tab[TempL];
	str[4]='.';
  str[6]=0xdf;
  str[7]='C';
	temp=ReadTemperature();//��ȡ�¶�ֵ
	if(temp&0x8000)
	{
		str[0]=0xb0;//���ű�־
		temp=~temp;  // ȡ����1
		temp +=1;
	}
	
	TempH=temp>>4;	//��Ȩ�ر�֪��4λ��������λ
	TempL=temp&0x0F;
	TempL=TempL*6/10;//С�����ƴ���

	alarm();    //�ж��Ƿ���Ҫ����

	WriteCommand(0x80+0x04);//���ָ���һ�е�һ���ַ�
	for(i=0;i<8;i++)// ��ʾ
	{
		WriteData(str[i]);
		delay(10);
	}
	for(j = 1;j<6;j++)
	{
		SBUF = str[j];
		while(!TI);
		TI = 0;
	}
	for(m=0;m<4;m++){
		SBUF = CNCHAR[m];
		while(!TI);
		TI = 0;
	}
}

void display_range(uchar add,int t)
{
	uchar i;
	if(t<0)
	{
		str[0]=0xb0;//���ű�־
	}
	else
		str[0]=tab[abs(t)/100]; //��λ�¶�
   	str[1]=tab[(abs(t)%100)/10]; //ʮλ�¶�
   	str[2]=tab[(abs(t)%100)%10]; //��λ�¶�
	WriteCommand(0x80+0x40+add);
	for(i=0;i<3;i++)
	{
		WriteData(str[i]);
		delay(5);
	}
}


/****************************************************************/
/*                    ������                                    */
/******************************************************************/
void main()
{	
	UsartConfiguration();
	InitLcd();//lcd1602��ʼ��
	while(1)
	{
		handle_T();// �����¶ȣ���á���ʾ������
		delay(500);
  } 
}


