#include<reg51.h>     //包含头文件，一般情况不需要改动，头文件包含特殊功能寄存器的定义
#include<math.h>
#include<INTRINS.H>
#include"lcd1602.h"
#include"ds18b20.h"
#include"delay.h"

#define uchar unsigned char
#define uint unsigned int

sbit buzzer=P2^1;//蜂鸣器引脚
sbit rel=P2^3;

int temp;//测得温度
char temp_max=40,temp_min=10;//设定的温度 
char TempH,TempL;
uchar CNCHAR[6] = "℃  ";

/*******************************************************************************
* 函 数 名         :UsartConfiguration()
* 函数功能		   :设置串口
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/

void UsartConfiguration()
{
	SCON=0X50;			//设置为工作方式1
	TMOD=0X20;			//设置计数器工作方式2
	PCON=0X80;			//波特率加倍
	TH1=0XF3;				//计数器初始值设置，注意波特率是4800的
	TL1=0XF3;
	ES=1;						//打开接收中断
	EA=1;						//打开总中断
	TR1=1;					//打开计数器
}

/******************************************************************/
/*                  蜂鸣器报警程序                                */
/******************************************************************/
void alarm()
{
	if(TempH>=temp_max)//当温度高于设置的最高温报警
	{	
		buzzer=0;
		rel=0;
//		delay(50);
//		buzzer=1;
//		delay(50);
	}
	else if(TempH<temp_min)//当温度低于设置的最低温报警
	{
		buzzer=0;
		rel=0;
//		delay(50);
//		buzzer=1;
//		delay(50);
	}
	else//报警解除
	{
	 	buzzer=1;
		rel=1;
	}
}
/******************************************************************/
/*                   显示测得的温度                               */
/******************************************************************/
void handle_T()
{
	
	uchar i,j,m;
	str[0]=0x20;//显示为空
	str[1]=tab[TempH/100]; //百位温度
  str[2]=tab[(TempH%100)/10]; //十位温度
  str[3]=tab[(TempH%100)%10]; //个位温度,带小数点
  str[5]=tab[TempL];
	str[4]='.';
  str[6]=0xdf;
  str[7]='C';
	temp=ReadTemperature();//读取温度值
	if(temp&0x8000)
	{
		str[0]=0xb0;//负号标志
		temp=~temp;  // 取反加1
		temp +=1;
	}
	
	TempH=temp>>4;	//由权重表知移4位就是整数位
	TempL=temp&0x0F;
	TempL=TempL*6/10;//小数近似处理

	alarm();    //判断是否需要报警

	WriteCommand(0x80+0x04);//光标指向第一行第一个字符
	for(i=0;i<8;i++)// 显示
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
		str[0]=0xb0;//负号标志
	}
	else
		str[0]=tab[abs(t)/100]; //百位温度
   	str[1]=tab[(abs(t)%100)/10]; //十位温度
   	str[2]=tab[(abs(t)%100)%10]; //个位温度
	WriteCommand(0x80+0x40+add);
	for(i=0;i<3;i++)
	{
		WriteData(str[i]);
		delay(5);
	}
}


/****************************************************************/
/*                    主函数                                    */
/******************************************************************/
void main()
{	
	UsartConfiguration();
	InitLcd();//lcd1602初始化
	while(1)
	{
		handle_T();// 处理温度：获得、显示、报警
		delay(500);
  } 
}


