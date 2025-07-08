/****************************************************************************************
	* 
	* @author  文七電源
	* @淘寶店舖鏈接：https://shop598739109.taobao.com
	* @file           : LCDshow.c
  * @brief          : LCD屏幕顯示函數，顯示LCD的內容
  * @version V1.0
  * @date    07-13-2022
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

#include "LCDDriver.h"
#include "LCDshow.h"
#include "function.h"
extern struct _Ctr_value CtrValue;

uint32_t ShowCnt=0;//計時器，
void LCDShow(void)
{
	static int32_t preMode=0;//預存模式變換

	if(ShowCnt>100)//*秒更新屏幕，佔用資源太多，不能更新太快
	{
		ShowCnt=0;

		if(CtlModeCFlag==1)//控制模式發生變化
			Lcd_Clear(BLACK);//清屏幕

		if(DF.CtlMode == CTL_MODE_LCDB)//電源由LCD板控制，LCD上顯示對應的參數內容
		{
			if((preMode!=DF.BBFlag)||(CtlModeCFlag==1))//運行模式變換,該判斷執行一次，避免顯示屏一直刷新占空資源，該段顯示屏幕上固定的字
			{
				CtlModeCFlag=0;
				LCDshowMode();//顯示屏幕標題-控制模式
				LCDshowConstant();//顯示屏幕上一些固定的字母，符號
				preMode=DF.BBFlag;//保存當前模式，只有運行模式發生變化後，該段函數才會刷新
			}
			LCDshowVIout();//顯示輸出電壓電流
			LCDshowVIref();//顯示目標電壓電流
			LCDshowState();//顯示電源狀態和故障信息
			LCDshowCCV();//顯示恆壓恆流模式
		}
		else//電源由上位機控制，顯示屏上顯示其他
		{
			if(CtlModeCFlag==1)//運行模式變換,該判斷執行一次，避免顯示屏一直刷新占空資源，該段顯示屏幕上固定的字
			{
				CtlModeCFlag=0;
				LCDshowMode();//顯示屏幕標題-控制模式
				LCDshowMonitor();//顯示 CONTROLLED BY MONITOR
			}
		}
	}
}


void LCDshowMode(void)
{
	if(DF.BBFlag == BUCK) //BUCK模式顯示
	{
		LCDshowChar(40,10,BLUE,BLACK,12); 	//M
		LCDshowChar(56,10,BLUE,BLACK,14); 	//O
		LCDshowChar(72,10,BLUE,BLACK,3); 	//D
		LCDshowChar(88,10,BLUE,BLACK,4); 	//E
		LCDshowDot(96,10,BLUE,BLACK,1); 	//:
		LCDshowChar(104,10,BLUE,BLACK,1); 	//B
		LCDshowChar(120,10,BLUE,BLACK,20); 	//U
		LCDshowChar(136,10,BLUE,BLACK,2); 	//C
		LCDshowChar(152,10,BLUE,BLACK,10); 	//K
	}
	else //BOOST模式顯示
	{
		LCDshowChar(40,10,BLUE,BLACK,12); 	//M
		LCDshowChar(56,10,BLUE,BLACK,14); 	//O
		LCDshowChar(72,10,BLUE,BLACK,3); 	//D
		LCDshowChar(88,10,BLUE,BLACK,4); 	//E
		LCDshowDot(96,10,BLUE,BLACK,1); 	//:
		LCDshowChar(104,10,BLUE,BLACK,1); 	//B
		LCDshowChar(120,10,BLUE,BLACK,14); 	//O
		LCDshowChar(136,10,BLUE,BLACK,14); 	//O
		LCDshowChar(152,10,BLUE,BLACK,18); 	//S
		LCDshowChar(168,10,BLUE,BLACK,19); 	//T
	}
}

void LCDshowConstant(void)
{
	if(DF.BBFlag == BUCK)//BUCK模式顯示，BUCK模式輸出為Y端，輸入為X端
	{
		//V-Y:表示輸出電壓
		LCDshowChar(16,50,YELLOW,BLACK,21);	//V
		LCDshowChar(32,50,YELLOW,BLACK,24);	//Y
		LCDshowDot(48,50,YELLOW,BLACK,1);	//:
		//I-Y:表示輸出電流
		LCDshowChar(16,90,YELLOW,BLACK,8);	//I
		LCDshowChar(32,90,YELLOW,BLACK,24);	//Y
		LCDshowDot(48,90,YELLOW,BLACK,1);	//:
	}
	else//BOOST模式顯示，輸出為X端，輸入為Y端
	{
		//V-X:表示輸出電壓
		LCDshowChar(16,50,YELLOW,BLACK,21);	//V
		LCDshowChar(32,50,YELLOW,BLACK,23);	//X
		LCDshowDot(48,50,YELLOW,BLACK,1);	//:
		//I-X:表示輸出電流
		LCDshowChar(16,90,YELLOW,BLACK,8);	//I
		LCDshowChar(32,90,YELLOW,BLACK,23);	//X
		LCDshowDot(48,90,YELLOW,BLACK,1);	//:
	}

	//顯示設定目標電壓
	LCDshowChar(16,130,YELLOW,BLACK,18);	//S
	LCDshowChar(32,130,YELLOW,BLACK,4);		//E
	LCDshowChar(48,130,YELLOW,BLACK,19);	//T
	LCDshowDot(64,130,YELLOW,BLACK,0);		//""
	LCDshowChar(80,130,YELLOW,BLACK,21);	//V
	LCDshowDot(96,130,YELLOW,BLACK,1);		//:
	//顯示設定目標電流
	LCDshowChar(16,170,YELLOW,BLACK,18);	//S
	LCDshowChar(32,170,YELLOW,BLACK,4);		//E
	LCDshowChar(48,170,YELLOW,BLACK,19);	//T
	LCDshowDot(64,170,YELLOW,BLACK,0);		//""
	LCDshowChar(80,170,YELLOW,BLACK,8);		//I
	LCDshowDot(96,170,YELLOW,BLACK,1);		//:

	//顯示單位
	LCDshowChar(144,50,WHITE,BLACK,21);		//V
	LCDshowChar(144,90,WHITE,BLACK,0);		//A
	LCDshowChar(192,130,WHITE,BLACK,21);	//V
	LCDshowChar(192,170,WHITE,BLACK,0);		//A
}

void LCDshowVIout(void)
{
	static int32_t vtemp=0,itemp=0;//預存電壓電流

	if(DF.BBFlag == BUCK)//BUCK模式顯示-輸出為Y端，該段程序顯示實際運行中的輸出電壓電流
	{
		vtemp =SADC.VyAvg*6800>>12;//將數字量轉換成實際值，並擴大100倍(顯示屏幕帶小數點)
		LCDShowFnum(64,50,WHITE,BLACK,vtemp);//將數字顯示
		itemp =SADC.IyAvg*3300>>12;//將數字量轉換成實際值，並擴大100倍(顯示屏幕帶小數點)
		LCDShowFnum(64,90,WHITE,BLACK,itemp);//將數字顯示;//將數字顯示

	}
	else//BOOST模式顯示-輸出為X端，該段程序顯示實際運行中的輸出電壓電流
	{
		vtemp =SADC.VxAvg*6800>>12;//將數字量轉換成實際值，並擴大100倍(顯示屏幕帶小數點)
		LCDShowFnum(64,50,WHITE,BLACK,vtemp);//將數字顯示
		itemp =SADC.IxAvg*3300>>12;//將數字量轉換成實際值，並擴大100倍(顯示屏幕帶小數點)
		LCDShowFnum(64,90,WHITE,BLACK,itemp);//將數字顯示;//將數字顯示
	}
}

void LCDshowVIref(void)
{
	static int32_t vtemp=0,itemp=0;//預存電壓電流

	vtemp =CtrValue.Voref*6800>>12;//將數字量轉換成實際值，並擴大100倍(顯示屏幕帶小數點)
	LCDShowFnum(112,130,WHITE,BLACK,vtemp);//將數字顯示;
	itemp =CtrValue.Ioref*3300>>12;//將數字量轉換成實際值，並擴大100倍(顯示屏幕帶小數點)
	LCDShowFnum(112,170,WHITE,BLACK,itemp);//將數字顯示;
}

void LCDshowState(void)
{
	if(DF.ErrFlag ==F_NOERR)//無故障時顯示模塊運行狀態
	{
		switch(DF.SMFlag)//判斷狀態機類型-不同狀態機顯示不同內容
		{
			case  Init ://初始化狀態
			{
				LCDshowChar(16,205,BLUE,BLACK,8);		//I
				LCDshowChar(32,205,BLUE,BLACK,13);		//N
				LCDshowChar(48,205,BLUE,BLACK,8);		//I
				LCDshowChar(64,205,BLUE,BLACK,19);		//T
				break;
			}
			case  Wait ://等待狀態
			{
				LCDshowChar(16,205,BLUE,BLACK,22);		//W
				LCDshowChar(32,205,BLUE,BLACK,0);		//A
				LCDshowChar(48,205,BLUE,BLACK,8);		//I
				LCDshowChar(64,205,BLUE,BLACK,19);		//T
				break;
			}
			case  Rise ://軟啟動狀態
			{
				LCDshowChar(16,205,BLUE,BLACK,17);		//R
				LCDshowChar(32,205,BLUE,BLACK,8);		//I
				LCDshowChar(48,205,BLUE,BLACK,18);		//S
				LCDshowChar(64,205,BLUE,BLACK,4);		//E
				break;
			}
			case  Run ://運行狀態
			{
				LCDshowChar(16,205,BLUE,BLACK,17);		//R
				LCDshowChar(32,205,BLUE,BLACK,20);		//U
				LCDshowChar(48,205,BLUE,BLACK,13);		//N
				LCDshowDot(64,205,BLUE,BLACK,0);		//
				break;
			}
			case  Err ://故障狀態
			{
				LCDshowChar(16,205,RED,BLACK,4);		//E
				LCDshowChar(32,205,RED,BLACK,17);		//R
				LCDshowChar(48,205,RED,BLACK,17);		//R
				LCDshowDot(64,205,RED,BLACK,0);			//
				break;
			}
		}
	}
	else//發生故障時顯示故障類型
		LCDshowErr();
}

void LCDshowErr(void)
{
	if(getRegBits(DF.ErrFlag,F_SW_VIN_UVP))
	{
			LCDshowChar(16,205,BLACK,RED,8);		//I
			LCDshowChar(32,205,BLACK,RED,20);		//U
			LCDshowChar(48,205,BLACK,RED,21);		//V
			LCDshowChar(64,205,BLACK,RED,15);		//P
	}
	else if(getRegBits(DF.ErrFlag,F_SW_VIN_OVP))
	{
			LCDshowChar(16,205,BLACK,RED,8);		//I
			LCDshowChar(32,205,BLACK,RED,14);		//O
			LCDshowChar(48,205,BLACK,RED,21);		//V
			LCDshowChar(64,205,BLACK,RED,15);		//P
	}
	else if(getRegBits(DF.ErrFlag,F_SW_VOUT_UVP))
	{
			LCDshowChar(16,205,BLACK,RED,14);		//O
			LCDshowChar(32,205,BLACK,RED,20);		//U
			LCDshowChar(48,205,BLACK,RED,21);		//V
			LCDshowChar(64,205,BLACK,RED,15);		//P
	}
	else if(getRegBits(DF.ErrFlag,F_SW_VOUT_OVP))
	{
			LCDshowChar(16,205,BLACK,RED,14);		//O
			LCDshowChar(32,205,BLACK,RED,14);		//O
			LCDshowChar(48,205,BLACK,RED,21);		//V
			LCDshowChar(64,205,BLACK,RED,15);		//P
	}
	else if(getRegBits(DF.ErrFlag,F_SW_IOUT_OCP))
	{
			LCDshowChar(16,205,BLACK,RED,14);		//O
			LCDshowChar(32,205,BLACK,RED,14);		//O
			LCDshowChar(48,205,BLACK,RED,2);		//C
			LCDshowChar(64,205,BLACK,RED,15);		//P
	}
	else if(getRegBits(DF.ErrFlag,F_SW_SHORT))
	{
			LCDshowChar(16,205,BLACK,RED,18);		//S
			LCDshowChar(32,205,BLACK,RED,7);		//H
			LCDshowChar(48,205,BLACK,RED,14);		//O
			LCDshowChar(64,205,BLACK,RED,19);		//T
	}
}

void LCDshowCCV(void)
{
	if(DF.SMFlag == Run)
	{
		if(DF.cvMode == CV_MODE)//恆壓模式
		{
			LCDshowDot(134,205,BLACK,RED,0);	//
			LCDshowChar(150,205,BLACK,RED,2);	//C
			LCDshowChar(166,205,BLACK,RED,21);	//V
			LCDshowDot(182,205,BLACK,RED,0);	//
		}
		else//恆流模式
		{
			LCDshowDot(134,205,BLACK,RED,0);	//
			LCDshowChar(150,205,BLACK,RED,2);	//C
			LCDshowChar(166,205,BLACK,RED,2);	//C
			LCDshowDot(182,205,BLACK,RED,0);	//
		}
	}
	else
	{
		LCDshowDot(134,205,BLACK,RED,0);		//
		LCDshowDot(150,205,BLACK,RED,0);		//
		LCDshowDot(166,205,BLACK,RED,0);		//
		LCDshowDot(182,205,BLACK,RED,0);		//
	}
}

void LCDshowMonitor(void)
{
	/* CONTROLLED BY MONITOR*/
	LCDshowChar(30,100,YELLOW,BLACK,2);		//C
	LCDshowChar(46,100,YELLOW,BLACK,14);	//O
	LCDshowChar(62,100,YELLOW,BLACK,13);	//N
	LCDshowChar(78,100,YELLOW,BLACK,19);	//T
	LCDshowChar(94,100,YELLOW,BLACK,17);	//R
	LCDshowChar(110,100,YELLOW,BLACK,11);	//L
	LCDshowChar(126,100,YELLOW,BLACK,11);	//L
	LCDshowChar(142,100,YELLOW,BLACK,4);	//E
	LCDshowChar(158,100,YELLOW,BLACK,3);	//D
	LCDshowChar(190,100,YELLOW,BLACK,1);	//B
	LCDshowChar(206,100,YELLOW,BLACK,24);	//Y

	LCDshowChar(70,132,YELLOW,BLACK,12);	//M
	LCDshowChar(86,132,YELLOW,BLACK,14);	//O
	LCDshowChar(102,132,YELLOW,BLACK,13);	//N
	LCDshowChar(118,132,YELLOW,BLACK,8);	//I
	LCDshowChar(134,132,YELLOW,BLACK,19);	//T
	LCDshowChar(150,132,YELLOW,BLACK,14);	//O
	LCDshowChar(166,132,YELLOW,BLACK,17);	//R
}
