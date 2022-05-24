#ifndef __LCD1602_H__
#define __LCD1602_H__

#define uchar unsigned char
#define uint unsigned int

extern uchar code tab[];
extern uchar tab1[];
extern uchar  str[8];

void WriteCommand(uchar com);
void WriteData(uchar dat);
void InitLcd();

#endif