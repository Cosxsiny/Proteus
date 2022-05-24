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
ulong volt;//�����ĵ�ѹֵ
sbit Data=P2^3;   //����������
sbit CLK=P3^3;//����ʱ���źſ�
sbit DIN=P3^1;//����2543����д���
sbit DOUT=P3^0;//����2543���ݶ�ȡ��
sbit CS=P3^2;//����2543Ƭѡ�źſ�
sbit Data_P    = P2^4;			// SHT11�����������ݹܽ�
sbit Sck_P     = P2^3;			// SHT11��������ʱ�ӹܽ�

sbit BEEP =P2^5;//����������
sbit Fan = P1^0;//��������
sbit Win = P1^1;//��������
sbit Water = P3^5;//ˮ������

uchar tmpe,h;
uchar rec_dat[9];   //������ʾ�Ľ�����������
uchar temp_max = 30;
uchar humi_min = 60;
ulong C2_max = 3500000;
ulong C2_now = 0;

unsigned char temp;							// �����¶�
unsigned char humi;				  		// ����ʪ��

enum { TEMP,HUMI };
typedef union              		//���干��ͬ����
{
	unsigned int i;
	float f;
}value;


int display = 0;
void delay(uchar ms) 
{  // ��ʱ�ӳ��� 
uchar i; 
while(ms--) 
{ 
  for(i = 0;i<250;i++);  
} 
}

char ShtWriteByte(unsigned char value)
{
	unsigned char i,error=0;
	for(i=128;i>0;i>>=1)  // ��λΪ1��ѭ������
	{
		if (i&value)
			Data_P=1;       	// ��Ҫ���͵������룬���Ϊ���͵�λ
		else
			Data_P=0;
		Sck_P=1;
		_nop_();						// ��ʱ3us
		_nop_();
		_nop_();
		Sck_P=0;
	}
	Data_P=1;    					// �ͷ�������
	Sck_P=1;
	error=Data_P;  				// ���Ӧ���źţ�ȷ��ͨѶ����
	_nop_();
	_nop_();
	_nop_();
	Sck_P=0;
	Data_P=1;
	return error; 				// error=1 ͨѶ����
}

char ShtReadByte(unsigned char ack)
{
	unsigned char i,val=0;
	Data_P=1; 						// �ͷ�������
	for(i=0x80;i>0;i>>=1)	// ��λΪ1��ѭ������
	{
		Sck_P=1;
		if(Data_P)
			val=(val|i);    	// ��һλ�����ߵ�ֵ
		Sck_P=0;
	}
	Data_P=!ack;    			// �����У�飬��ȡ������ͨѶ
	Sck_P=1;
	_nop_();							// ��ʱ3us
	_nop_();
	_nop_();
	Sck_P=0;
	_nop_();
	_nop_();
	_nop_();
	Data_P=1; 						// �ͷ�������
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
	Data_P=1; 		   		//׼��
	Sck_P=0;
	for(i=0;i<9;i++)  	//DATA���ָߣ�SCKʱ�Ӵ���9�Σ������������䣬ͨѸ����λ
	{
		Sck_P=1;
		Sck_P=0;
	}
	ShtTransStart();   	//��������
}

char ShtMeasure(unsigned char *p_value, unsigned char *p_checksum, unsigned char mode)
{
	unsigned error=0;
	unsigned int i;
	ShtTransStart();  		// ��������
	switch(mode)       		// ѡ��������
	{
		case 1 :   					// �����¶�
			error+=ShtWriteByte(0x03);
			break;
		case 2 :   					// ����ʪ��
			error+=ShtWriteByte(0x05);
			break;
		default:
			break;
	}
	for(i=0;i<65535;i++)
		if(Data_P==0)
			break;  					// �ȴ���������
		if(Data_P)
			error+=1;   			// �����ʱ��������û�����ͣ�˵����������
	*(p_value) =ShtReadByte(1);  		// ����һ���ֽڣ����ֽ� (MSB)
	*(p_value+1)=ShtReadByte(1); 		// ���ڶ����ֽڣ����ֽ� (LSB)
	*p_checksum =ShtReadByte(0);  	// read CRCУ����
	return error;  									// error=1 ͨѶ����
}

void CalcSHT11(float *p_humidity ,float *p_temperature)
{
	const float C1=-4.0;	 			// 12λʪ�Ⱦ��� ������ʽ
	const float C2=+0.0405;			// 12λʪ�Ⱦ��� ������ʽ
	const float C3=-0.0000028;	// 12λʪ�Ⱦ��� ������ʽ
	const float T1=+0.01;	 			// 14λ�¶Ⱦ��� 5V���� ������ʽ
	const float T2=+0.00008;	 	// 14λ�¶Ⱦ��� 5V���� ������ʽ
	float rh=*p_humidity;	 			// rh: 12λ ʪ��
	float t=*p_temperature;			// t:  14λ �¶�
	float rh_lin;								// rh_lin: ʪ�� linearֵ
	float rh_true;							// rh_true: ʪ�� tureֵ
	float t_C;	 								// t_C : �¶� ��
	t_C=t*0.01 - 40;	 					//�����¶�
	rh_lin=C3*rh*rh + C2*rh + C1;					//���ʪ�ȷ����Բ���
	rh_true=(t_C-25)*(T1+T2*rh)+rh_lin;		//���ʪ�ȶ����¶������Բ���
	*p_temperature=t_C;	 				//�����¶Ƚ��
	*p_humidity=rh_true;	 			//����ʪ�Ƚ��
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
	value humi_val,temp_val;  	// ����������ͬ�壬һ������ʪ�ȣ�һ�������¶�
	unsigned char error;  							// ���ڼ����Ƿ���ִ���
	unsigned char checksum;  						// CRC
	unsigned int temp1,humi1;						// ��ʱ��ȡ������ʪ������

	error=0; 										//��ʼ��error=0����û�д���
	error+=ShtMeasure((unsigned char*)&temp_val.i,&checksum,1); 	//�¶Ȳ���
	error+=ShtMeasure((unsigned char*)&humi_val.i,&checksum,2); 	//ʪ�Ȳ���

	if(error!=0) 		  					//�����������ϵͳ��λ
		ShtConnectReset();
	else
	{
		humi_val.f=(float)humi_val.i; 				//ת��Ϊ������
		temp_val.f=(float)temp_val.i;  				//ת��Ϊ������
		CalcSHT11(&humi_val.f,&temp_val.f);  	//�������ʪ�ȼ��¶�
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
	CS=0;//Ƭѡ�Σ�����2543
	addr<<=4;//�Ե�ַλԤ����
	for(i=0;i<12;i++) //12��ʱ�����꣬���һ�ζ�ȡ����
	{
		if(DOUT==1)
			ad=ad|0x01;//��Ƭ����ȡad����
		DIN=addr&0x80;//2543��ȡ������ַλ
		CLK=1;
		;;;//�̵ܶ���ʱ
		CLK=0;//�����½��أ�����ʱ���ź�
		;;;
		addr<<=1;
		ad<<=1;//��������λ׼����һλ�Ķ�д
	}
	CS=1;//��2543
	ad>>=1;
	volt=ad;//ȡ��ת�����
	volt=volt*1221;//���ӵ�������Ϊ5V��ת���ֱ���Ϊ12λ��2��12�η�=4096�� �������ֵ��4096��5/4096=1221mV,�������е�1V����ʵ��1221mV        
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
		
		//read2543(0);//����2543�������������ַΪ
		//LUX_now=volt;
		//DisplayOneChar(4,1,(char)(volt/1000000+'0'));
  	//DisplayOneChar(6,1,(char)((volt/100000)%10+'0'));
	  //DisplayOneChar(7,1,(char)((volt/10000)%10+'0'));
		read2543(1);//����2543�������������ַΪ
		C2_now=volt;
		DisplayOneChar(4,1,(char)(volt/1000000+'0'));
  	DisplayOneChar(6,1,(char)((volt/100000)%10+'0'));
	  DisplayOneChar(7,1,(char)((volt/10000)%10+'0'));
		
		if(C2_now>C2_max) //CO2Ũ�ȴ����趨�����ֵʱ ���������ļ̵���  ͬʱ����������
		{
			Win = 1;
			BEEP = 0;
		}
		else if(humi<humi_min)//ʪ��С���趨����Сֵʱ ����ˮ�õļ̵���  ͬʱ����������
		{
			Water = 1;
			BEEP = 0;
		}
		else if(temp>temp_max)//�¶ȴ����趨�����ֵʱ �������ȵļ̵���  ͬʱ����������
		{
			Fan = 1;
			BEEP = 0;
		}
		else
		{
			BEEP = 1;//�رշ�����
			Win = 0;
			Water = 0;
			Fan = 0;
		}
	}				
}
