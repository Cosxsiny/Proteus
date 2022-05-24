#ifndef __DS18B20_H__
#define __DS18B20_H__

#define uchar unsigned char
#define uint unsigned int 

void Init_DS18B20(void);
uchar ReadOneChar(void);
void WriteOneChar(uchar dat);
uint ReadTemperature(void);

#endif