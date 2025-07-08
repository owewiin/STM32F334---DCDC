/****************************************************************************************
	* 
	* @author  文七電源
	* @淘寶店舖鏈接：https://shop227154945.taobao.com/
	* @file          : LCDDriver.c
  * @brief         : 1.4寸 LCD的底層驅動代碼
  * @version V1.0
  * @date    07-11-2022
  * @LegalDeclaration ：本文檔內容難免存在Bug，僅限於交流學習，禁止用於任何的商業用途
	* @Copyright   著作權文七電源所有
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
*/
/***************************************************************************
*本程序適用STM32F334
* GND   電源地
* VCC   接5V或3.3V電源
* SCL   接PB3（SCL）
* SDA   接PB5（SDA）
* RES   接PB15
* DC    接PB12
* CS    接GND
* BL	接3.3V
*******************************************************************************/
#include "LCDDriver.h"
#include "function.h"
#include "LCDdata.h"
/*
** ===================================================================
**     Funtion Name :void SPI_WriteData(uint8_t Data)
**     Description :向SPI總線傳輸一個8位數據
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void SPI_WriteData(uint8_t Data)
{
	unsigned char i=0;
	for(i=8;i>0;i--)
	{
		if(Data&0x80) 
		{
			GPIOB->BSRR=GPIO_PIN_5;
		}			
		else 
		{
			GPIOB->BRR=GPIO_PIN_5;//數據低
		} 
		GPIOB->BRR=GPIO_PIN_3;__NOP();
		GPIOB->BSRR=GPIO_PIN_3;__NOP();
		Data<<=1; 
	}
}
/*
** ===================================================================
**     Funtion Name :void Lcd_WriteData(uint8_t Data)
**     Description :向液晶屏寫一個8位指令
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void Lcd_WriteIndex(uint8_t Index)
{
   //LCD_CS_CLR;
   LCD_RS_CLR;
	 SPI_WriteData(Index);
   //LCD_CS_SET;
}
/*
** ===================================================================
**     Funtion Name :void Lcd_WriteData(uint8_t Data)
**     Description :向液晶屏寫一個8位數據
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void Lcd_WriteData(uint8_t Data)
{
   //LCD_CS_CLR;
   LCD_RS_SET;
   SPI_WriteData(Data);
   //LCD_CS_SET; 
}
/*
** ===================================================================
**     Funtion Name :void LCD_WriteData_16Bit(uint16_t Data)
**     Description :向液晶屏寫一個16位數據
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void LCD_WriteData_16Bit(uint16_t Data)
{
   //LCD_CS_CLR;
   LCD_RS_SET;
	 SPI_WriteData(Data>>8); 	//寫入高8位數據
	 SPI_WriteData(Data); 			//寫入低8位數據
   //LCD_CS_SET; 
}
/*
** ===================================================================
**     Funtion Name :void Lcd_WriteReg(uint8_t Index,uint8_t Data)
**     Description :寫指令 數據
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void Lcd_WriteReg(uint8_t Index,uint8_t Data)
{
	Lcd_WriteIndex(Index);
  Lcd_WriteData(Data);
}
/*
** ===================================================================
**     Funtion Name :void Lcd_Reset(void)
**     Description :復位LCD
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void Lcd_Reset(void)
{
	LCD_RST_CLR;
	HAL_Delay(100);
	LCD_RST_SET;
	HAL_Delay(50);
}
/*
** ===================================================================
**     Funtion Name :void Lcd_Init(void)
**     Description :LCD初始化
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void Lcd_Init(void)
{	
	LCD_SCL_SET;//特別注意
	LCD_RST_CLR;
	HAL_Delay(100);
	LCD_RST_SET;
	HAL_Delay(100);
	Lcd_WriteIndex(0x11); 		
	HAL_Delay(120);     
	//-----------------------ST7789V Frame rate setting-----------------//
	//************************************************
	Lcd_WriteIndex(0x3A);        //65k mode
	Lcd_WriteData(0x05);
	Lcd_WriteIndex(0xC5); 		//VCOM
	Lcd_WriteData(0x1A);
	Lcd_WriteIndex(0x36);                 // 屏幕顯示方向設置
	Lcd_WriteData(0x00);
	//-------------ST7789V Frame rate setting-----------//
	Lcd_WriteIndex(0xb2);		//Porch Setting
	Lcd_WriteData(0x05);
	Lcd_WriteData(0x05);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x33);
	Lcd_WriteData(0x33);

	Lcd_WriteIndex(0xb7);			//Gate Control
	Lcd_WriteData(0x05);			//12.2v   -10.43v
	//--------------ST7789V Power setting---------------//
	Lcd_WriteIndex(0xBB);//VCOM
	Lcd_WriteData(0x3F);

	Lcd_WriteIndex(0xC0); //Power control
	Lcd_WriteData(0x2c);

	Lcd_WriteIndex(0xC2);		//VDV and VRH Command Enable
	Lcd_WriteData(0x01);

	Lcd_WriteIndex(0xC3);			//VRH Set
	Lcd_WriteData(0x0F);		//4.3+( vcom+vcom offset+vdv)

	Lcd_WriteIndex(0xC4);			//VDV Set
	Lcd_WriteData(0x20);				//0v

	Lcd_WriteIndex(0xC6);				//Frame Rate Control in Normal Mode
	Lcd_WriteData(0X01);			//111Hz

	Lcd_WriteIndex(0xd0);				//Power Control 1
	Lcd_WriteData(0xa4);
	Lcd_WriteData(0xa1);

	Lcd_WriteIndex(0xE8);				//Power Control 1
	Lcd_WriteData(0x03);

	Lcd_WriteIndex(0xE9);				//Equalize time control
	Lcd_WriteData(0x09);
	Lcd_WriteData(0x09);
	Lcd_WriteData(0x08);
	//---------------ST7789V gamma setting-------------//
	Lcd_WriteIndex(0xE0); //Set Gamma
	Lcd_WriteData(0xD0);
	Lcd_WriteData(0x05);
	Lcd_WriteData(0x09);
	Lcd_WriteData(0x09);
	Lcd_WriteData(0x08);
	Lcd_WriteData(0x14);
	Lcd_WriteData(0x28);
	Lcd_WriteData(0x33);
	Lcd_WriteData(0x3F);
	Lcd_WriteData(0x07);
	Lcd_WriteData(0x13);
	Lcd_WriteData(0x14);
	Lcd_WriteData(0x28);
	Lcd_WriteData(0x30);

	Lcd_WriteIndex(0XE1); //Set Gamma
	Lcd_WriteData(0xD0);
	Lcd_WriteData(0x05);
	Lcd_WriteData(0x09);
	Lcd_WriteData(0x09);
	Lcd_WriteData(0x08);
	Lcd_WriteData(0x03);
	Lcd_WriteData(0x24);
	Lcd_WriteData(0x32);
	Lcd_WriteData(0x32);
	Lcd_WriteData(0x3B);
	Lcd_WriteData(0x14);
	Lcd_WriteData(0x13);
	Lcd_WriteData(0x28);
	Lcd_WriteData(0x2F);

	Lcd_WriteIndex(0x21); 		//反顯

	Lcd_WriteIndex(0x29);         //開啟顯示
}
/*
** ===================================================================
**     Funtion Name :void Lcd_SetRegion(uint16_t x_start,uint16_t y_start,uint16_t x_end,uint16_t y_end)
**     Description : 設置lcd顯示區域，在此區域寫點數據自動換行
**     Parameters  :xy起點和終點
**     Returns     :
** ===================================================================
*/
void Lcd_SetRegion(uint16_t x_start,uint16_t y_start,uint16_t x_end,uint16_t y_end)
{			
	Lcd_WriteIndex(0x2a);
	LCD_WriteData_16Bit(x_start);
	LCD_WriteData_16Bit(x_end);
	Lcd_WriteIndex(0x2B);
	LCD_WriteData_16Bit(y_start);
	LCD_WriteData_16Bit(y_end);
	Lcd_WriteIndex(0x2c);	
}
/*
** ===================================================================
**     Funtion Name :void Lcd_SetXY(uint16_t x,uint16_t y)
**     Description : 設置lcd顯示起始點
**     Parameters  :xy坐標
**     Returns     :Data
** ===================================================================
*/
void Lcd_SetXY(uint16_t x,uint16_t y)
{
  	//Lcd_SetRegion(x,y,x,y);
	Lcd_WriteIndex(0x2A);
	LCD_WriteData_16Bit(x);
	Lcd_WriteIndex(0x2B);
	LCD_WriteData_16Bit(y);
	Lcd_WriteIndex(0x2c);	
}
/*
** ===================================================================
**     Funtion Name :void Gui_DrawPoint(uint16_t x,uint16_t y,uint16_t Data)
**     Description : 畫一個點
**     Parameters  :無
**     Returns     :
** ===================================================================
*/
void Gui_DrawPoint(uint16_t x,uint16_t y,uint16_t Data)
{
	Lcd_SetXY(x,y);
	LCD_WriteData_16Bit(Data);
}    
/*
** ===================================================================
**     Funtion Name :unsigned int Lcd_ReadPoint(uint16_t x,uint16_t y)
**     Description : 讀TFT某一點的顏色
**     Parameters  :無
**     Returns     :Data
** ===================================================================
*/
unsigned int Lcd_ReadPoint(uint16_t x,uint16_t y)
{
  unsigned int Data = 0;
  Lcd_SetXY(x,y);
  //Lcd_ReadData();//丟掉無用字節
  //Data=Lcd_ReadData();
  Lcd_WriteData(Data);
  return Data;
}
/*
** ===================================================================
**     Funtion Name :  Lcd_Clear(uint16_t Color)
**     Description :   全屏清屏函數，填充顏色COLOR
**     Parameters  :無
**     Returns     :無
** ===================================================================
*/
void Lcd_Clear(uint16_t Color)               
{	
   unsigned int i;
   Lcd_SetRegion(0,0,X_MAX_PIXEL-1,Y_MAX_PIXEL-1);
   LCD_RS_SET;	
   for(i=0;i<X_MAX_PIXEL*Y_MAX_PIXEL;i++)
   {	
		SPI_WriteData(Color>>8);
		SPI_WriteData(Color);
   }   
}

/*
** ===================================================================
**     Funtion Name :CCMRAM void LCDshowDate(uint16_t x,uint16_t y,uint16_t fc,uint16_t bc,uint16_t add)
**     Description : 顯示數字0~9,
**     0~9數字的字碼在LCDdata數組中定義
**     Parameters  :xy顯示坐標，fc字體顏色，bc,背景顏色，add顯示數字在數組中的位置
**     Returns     :無
** ===================================================================
*/
void LCDshowDate(uint16_t x,uint16_t y,uint16_t fc,uint16_t bc,uint16_t add)
{
	uint16_t column;
  uint8_t tm=0,temp=0,xxx=0;
	
	Lcd_SetRegion(x,y,x+15,y+31);//定義顯示的x，y坐標，結束坐標為x+15，y+31
	
	for(column=0;column<64;column++)//每個數字的字碼由64個字節組成，見LCDdata數組中表示
	{
		temp=LCDdata[add][xxx];
		for(tm=0;tm<8;tm++)
		{
			if(temp&0x01)
			{
				Lcd_WriteData(fc>>8);
				Lcd_WriteData(fc);
			}
			else 
			{
				Lcd_WriteData(bc>>8);
				Lcd_WriteData(bc);
			}
			temp>>=1;
		}
		xxx++;
	}
}
/*
** ===================================================================
**     Funtion Name :CCMRAM CCMRAM void LCDshowChar(uint16_t x,uint16_t y,uint16_t fc,uint16_t bc,uint16_t add)
**     Description : 顯示字母A~Z,
**     A~Z字母的字碼在LCDcha數組中定義
**     Parameters  :xy顯示坐標，fc字體顏色，bc,背景顏色，add顯示字母在數組中的位置
**     Returns     :無
** ===================================================================
*/
void LCDshowChar(uint16_t x,uint16_t y,uint16_t fc,uint16_t bc,uint16_t add)
{
	uint16_t column;
	uint8_t tm=0,temp=0,xxx=0;
	Lcd_SetRegion(x,y,x+15,y+31);
	
	for(column=0;column<64;column++)
	{
		temp=LCDchar[add][xxx];
		for(tm=0;tm<8;tm++)
		{
			if(temp&0x01)
			{
				Lcd_WriteData(fc>>8);
				Lcd_WriteData(fc);
			}
			else 
			{
				Lcd_WriteData(bc>>8);
				Lcd_WriteData(bc);
			}
			temp>>=1;
		}
		xxx++;
	}
}
/*
** ===================================================================
**     Funtion Name :CCMRAM void LCDshowDot(uint16_t x,uint16_t y,uint16_t fc,uint16_t bc,uint16_t add)
**     Description : 顯示特殊符號,
**     特殊符號的字碼在LCDcha數組中定義
**     Parameters  :xy顯示坐標，fc字體顏色，bc,背景顏色，add顯示特殊符號在數組中的位置
**     Returns     :無
** ===================================================================
*/
void LCDshowDot(uint16_t x,uint16_t y,uint16_t fc,uint16_t bc,uint16_t add)
{
	uint16_t column;
  uint8_t tm=0,temp=0,xxx=0;
	Lcd_SetRegion(x,y,x+15,y+31);
	
	for(column=0;column<64;column++) 
	{
		temp=LCDdot[add][xxx];
		for(tm=0;tm<8;tm++)
		{
			if(temp&0x01)
			{
				Lcd_WriteData(fc>>8);
				Lcd_WriteData(fc);
			}
			else 
			{
				Lcd_WriteData(bc>>8);
				Lcd_WriteData(bc);
			}
			temp>>=1;
		}
		xxx++;
	}
}
/*
** ===================================================================
**     Funtion Name :CCMRAM void LCDshowDot(uint16_t x,uint16_t y,uint16_t fc,uint16_t bc,uint16_t add)
**     Description : 顯示特殊符號,
**     特殊符號的字碼在LCDcha數組中定義
**     Parameters  :xy顯示坐標，fc字體顏色，bc,背景顏色，num為顯示的數字
**     Returns     :無
** ===================================================================
*/
void LCDShowNum(uint16_t x,uint16_t y,uint16_t fc,uint16_t bc,uint16_t num)
{
	switch(num)//判斷為什麼數字
	{
		case  0: LCDshowDate(x,y,fc,bc,0);break;		
		case  1: LCDshowDate(x,y,fc,bc,1);break;		
		case  2: LCDshowDate(x,y,fc,bc,2);break;		
		case  3: LCDshowDate(x,y,fc,bc,3);break;	
		case  4: LCDshowDate(x,y,fc,bc,4);break;	
		case  5: LCDshowDate(x,y,fc,bc,5);break;
		case  6: LCDshowDate(x,y,fc,bc,6);break;
		case  7: LCDshowDate(x,y,fc,bc,7);break;		
		case  8: LCDshowDate(x,y,fc,bc,8);break;		
		case  9: LCDshowDate(x,y,fc,bc,9);break;			
	}	
}

/*
** ===================================================================
**     Funtion Name :  void LCDShowdData(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint16_t data)
**     Description :   顯示4位數字data
**     Parameters  :xy顯示坐標，fc字體顏色，bc背景顏色，data顯示數字
**     Returns     :無
** ===================================================================
*/
void LCDShowFnum(uint16_t x,uint16_t y,uint16_t fc,uint16_t bc,uint16_t data)
{
	uint8_t temp[4]={0,0,0,0};//暫存data的每一位數字
	
	temp[0] = (uint8_t)(data/1000);//第1位
	temp[1] = (uint8_t)((data-(uint8_t)temp[0]*1000)/100);//第2位
	temp[2] = (uint8_t)((data-(uint16_t)temp[0]*1000-(uint16_t)temp[1]*100)/10);//第3位
	temp[3] = (uint8_t)(data-(uint16_t)temp[0]*1000-(uint16_t)temp[1]*100-(uint16_t)temp[2]*10);//第4位
	
	LCDShowNum(x,y,fc,bc,temp[0]);//分別將每位數字顯示到屏幕上
	LCDShowNum(x+16,y,fc,bc,temp[1]);				
	LCDshowDot(x+32,y,fc,bc,2);//顯示小數點
	LCDShowNum(x+48,y,fc,bc,temp[2]);
	LCDShowNum(x+64,y,fc,bc,temp[3]);		
}
