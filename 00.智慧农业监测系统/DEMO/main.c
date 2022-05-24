#include <reg51.h>	
#include "lcd.h"
#include <intrins.h> 
#define uchar unsigned char
#define uint unsigned int
#define ulong unsigned long
#define NACK	0
#define ACK		1
#define MEASURE_TEMP	0x03	//000 0001  1
#define MEASURE_HUMI	0x05	//000 0010  1
#define STATUS_REG_W	0x06	//000 0011  0
#define STATUS_REG_R	0x07	//000 0011  1
#define RESET			0x1E	//000 1111  0
ulong volt;//测量的电压值
sbit Data=P2^3;   //定义数据线
sbit CLK=P3^3;//定义时钟信号口
sbit DIN=P3^1;//定义2543数据写入口
sbit DOUT=P3^0;//定义2543数据读取口
sbit CS=P3^2;//定义2543片选信号口
sbit Data_P    = P2^4;			// SHT11传感器的数据管脚
sbit Sck_P     = P2^3;			// SHT11传感器的时钟管脚

sbit BEEP =P2^5;//蜂鸣器引脚
sbit Fan = P1^0;//风扇引脚
sbit Win = P1^1;//窗帘引脚
sbit Water = P3^5;//水泵引脚

uchar tmpe,h;
uchar rec_dat[9];   //用于显示的接收数据数组
uchar temp_max = 30;
uchar humi_min = 60;
ulong C2_max = 3500000;
ulong C2_now = 0;

unsigned char temp;							// 保存温度
unsigned char humi;				  		// 保存湿度

enum { TEMP,HUMI };
typedef union              		//定义共用同类型
{
	unsigned int i;
	float f;
}value;


int display = 0;
void delay(uchar ms) 
{  // 延时子程序 
uchar i; 
while(ms--) 
{ 
  for(i = 0;i<250;i++);  
} 
}

char ShtWriteByte(unsigned char value)
{
	unsigned char i,error=0;
	for(i=128;i>0;i>>=1)  // 高位为1，循环右移
	{
		if (i&value)
			Data_P=1;       	// 和要发送的数相与，结果为发送的位
		else
			Data_P=0;
		Sck_P=1;
		_nop_();						// 延时3us
		_nop_();
		_nop_();
		Sck_P=0;
	}
	Data_P=1;    					// 释放数据线
	Sck_P=1;
	error=Data_P;  				// 检查应答信号，确认通讯正常
	_nop_();
	_nop_();
	_nop_();
	Sck_P=0;
	Data_P=1;
	return error; 				// error=1 通讯错误
}

char ShtReadByte(unsigned char ack)
{
	unsigned char i,val=0;
	Data_P=1; 						// 释放数据线
	for(i=0x80;i>0;i>>=1)	// 高位为1，循环右移
	{
		Sck_P=1;
		if(Data_P)
			val=(val|i);    	// 读一位数据线的值
		Sck_P=0;
	}
	Data_P=!ack;    			// 如果是校验，读取完后结束通讯
	Sck_P=1;
	_nop_();							// 延时3us
	_nop_();
	_nop_();
	Sck_P=0;
	_nop_();
	_nop_();
	_nop_();
	Data_P=1; 						// 释放数据线
	return val;
}


void ShtTransStart(void)
{
	Data_P=1;
	Sck_P=0;
	_nop_();
	Sck_P=1;
	_nop_();
	Data_P=0;
	_nop_();
	Sck_P=0;
	_nop_();
	_nop_();
	_nop_();
	Sck_P=1;
	_nop_();
	Data_P=1;
	_nop_();
	Sck_P=0;
}

void ShtConnectReset(void)
{
	unsigned char i;
	Data_P=1; 		   		//准备
	Sck_P=0;
	for(i=0;i<9;i++)  	//DATA保持高，SCK时钟触发9次，发送启动传输，通迅即复位
	{
		Sck_P=1;
		Sck_P=0;
	}
	ShtTransStart();   	//启动传输
}

char ShtMeasure(unsigned char *p_value, unsigned char *p_checksum, unsigned char mode)
{
	unsigned error=0;
	unsigned int i;
	ShtTransStart();  		// 启动传输
	switch(mode)       		// 选择发送命令
	{
		case 1 :   					// 测量温度
			error+=ShtWriteByte(0x03);
			break;
		case 2 :   					// 测量湿度
			error+=ShtWriteByte(0x05);
			break;
		default:
			break;
	}
	for(i=0;i<65535;i++)
		if(Data_P==0)
			break;  					// 等待测量结束
		if(Data_P)
			error+=1;   			// 如果长时间数据线没有拉低，说明测量错误
	*(p_value) =ShtReadByte(1);  		// 读第一个字节，高字节 (MSB)
	*(p_value+1)=ShtReadByte(1); 		// 读第二个字节，低字节 (LSB)
	*p_checksum =ShtReadByte(0);  	// read CRC校验码
	return error;  									// error=1 通讯错误
}

void CalcSHT11(float *p_humidity ,float *p_temperature)
{
	const float C1=-4.0;	 			// 12位湿度精度 修正公式
	const float C2=+0.0405;			// 12位湿度精度 修正公式
	const float C3=-0.0000028;	// 12位湿度精度 修正公式
	const float T1=+0.01;	 			// 14位温度精度 5V条件 修正公式
	const float T2=+0.00008;	 	// 14位温度精度 5V条件 修正公式
	float rh=*p_humidity;	 			// rh: 12位 湿度
	float t=*p_temperature;			// t:  14位 温度
	float rh_lin;								// rh_lin: 湿度 linear值
	float rh_true;							// rh_true: 湿度 ture值
	float t_C;	 								// t_C : 温度 ℃
	t_C=t*0.01 - 40;	 					//补偿温度
	rh_lin=C3*rh*rh + C2*rh + C1;					//相对湿度非线性补偿
	rh_true=(t_C-25)*(T1+T2*rh)+rh_lin;		//相对湿度对于温度依赖性补偿
	*p_temperature=t_C;	 				//返回温度结果
	*p_humidity=rh_true;	 			//返回湿度结果
}

unsigned char TempCorrect(int temp)
{
	if(temp<0)	temp=0;
	if(temp>970)  temp=970;
	if(temp>235)  temp=temp+10;
	if(temp>555)  temp=temp+10;
	if(temp>875)  temp=temp+10;
	temp=(temp%1000)/10;
	return temp;
}

unsigned char HumiCorrect(unsigned int humi)
{
	if(humi>999)  humi=999;
	if((humi>490)&&(humi<951))  humi=humi-10;
	humi=(humi%1000)/10;
	return humi+4;
}

void ReadShtData()
{
	value humi_val,temp_val;  	// 定义两个共同体，一个用于湿度，一个用于温度
	unsigned char error;  							// 用于检验是否出现错误
	unsigned char checksum;  						// CRC
	unsigned int temp1,humi1;						// 临时读取到的温湿度数据

	error=0; 										//初始化error=0，即没有错误
	error+=ShtMeasure((unsigned char*)&temp_val.i,&checksum,1); 	//温度测量
	error+=ShtMeasure((unsigned char*)&humi_val.i,&checksum,2); 	//湿度测量

	if(error!=0) 		  					//如果发生错误，系统复位
		ShtConnectReset();
	else
	{
		humi_val.f=(float)humi_val.i; 				//转换为浮点数
		temp_val.f=(float)temp_val.i;  				//转换为浮点数
		CalcSHT11(&humi_val.f,&temp_val.f);  	//修正相对湿度及温度
		temp1=temp_val.f*10;
		temp=TempCorrect(temp1);
		humi1=humi_val.f*10-50;
		humi=HumiCorrect(humi1);
		humi1=humi1-1;
	}

}


void read2543(uchar addr)
{
	uint ad=0;
	uchar i;
	CLK=0;
	CS=0;//片选段，启动2543
	addr<<=4;//对地址位预处理
	for(i=0;i<12;i++) //12个时钟走完，完成一次读取测量
	{
		if(DOUT==1)
			ad=ad|0x01;//单片机读取ad数据
		DIN=addr&0x80;//2543读取测量地址位
		CLK=1;
		;;;//很短的延时
		CLK=0;//产生下降沿，产生时钟信号
		;;;
		addr<<=1;
		ad<<=1;//将数据移位准备下一位的读写
	}
	CS=1;//关2543
	ad>>=1;
	volt=ad;//取走转换结果
	volt=volt*1221;//例子的满量程为5V，转换分辩率为12位（2的12次方=4096） 。即最大值是4096，5/4096=1221mV,即例子中的1V代表实际1221mV        
}

void main(void)
{
	LcdInit();
	ShtConnectReset();
	DisplayListChar(0,0,"temp:");
	DisplayListChar(8,0,"humi:");
	//DisplayListChar(0,1,"LUX:");
  //DisplayOneChar(5,1,'.');
	DisplayListChar(0,1,"CO2:");
	DisplayOneChar(5,1,'.');
	while(1)
	{
	  ReadShtData();

		DisplayOneChar(13,0,(char)(humi/10+'0'));
		DisplayOneChar(14,0,(char)(humi%10+'0'));
		DisplayOneChar(5,0,(char)(temp/10+'0'));
		DisplayOneChar(6,0,(char)(temp%10+'0'));
		
		//read2543(0);//调用2543驱动程序测量地址为
		//LUX_now=volt;
		//DisplayOneChar(4,1,(char)(volt/1000000+'0'));
  	//DisplayOneChar(6,1,(char)((volt/100000)%10+'0'));
	  //DisplayOneChar(7,1,(char)((volt/10000)%10+'0'));
		read2543(1);//调用2543驱动程序测量地址为
		C2_now=volt;
		DisplayOneChar(4,1,(char)(volt/1000000+'0'));
  	DisplayOneChar(6,1,(char)((volt/100000)%10+'0'));
	  DisplayOneChar(7,1,(char)((volt/10000)%10+'0'));
		
		if(C2_now>C2_max) //CO2浓度大于设定的最大值时 开启窗帘的继电器  同时蜂鸣器报警
		{
			Win = 1;
			BEEP = 0;
		}
		else if(humi<humi_min)//湿度小于设定的最小值时 开启水泵的继电器  同时蜂鸣器报警
		{
			Water = 1;
			BEEP = 0;
		}
		else if(temp>temp_max)//温度大于设定的最大值时 开启风扇的继电器  同时蜂鸣器报警
		{
			Fan = 1;
			BEEP = 0;
		}
		else
		{
			BEEP = 1;//关闭蜂鸣器
			Win = 0;
			Water = 0;
			Fan = 0;
		}
	}				
}
