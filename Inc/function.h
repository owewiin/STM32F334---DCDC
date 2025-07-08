/*
 * function.h
 *
 *  Created on: Dec 13, 2024
 *      Author: mshome
 */

#ifndef INC_FUNCTION_H_
#define INC_FUNCTION_H_

#include "RS232.h"
#include "stm32f3xx_hal.h"
#include "function.h"
#include "CtlLoop.h"
#include "stdio.h"

extern uint16_t ADC1_RESULT[5];
extern uint16_t ADC2_RESULT[3];
extern uint8_t CtlModeCFlag;
extern uint32_t TxCnt;
extern struct _ADI SADC;
extern struct _FLAG DF;
extern struct _Ctr_value CtrValue;


//採樣變量-平均值-偏置值結構體 (BUCK = 輸入端:X,輸出端:Y BOOST = 相反)
struct _ADI
{
	 int32_t  Vx;//X側電壓
	 int32_t  VxAvg;//X側電壓平均值

	 int32_t  Ix;//X側電流
	 int32_t  IxAvg;//X側電流平均值
	 int32_t  IxOffset;//X側電流偏置

	 int32_t  Vy;//Y側電壓
	 int32_t  VyAvg;//Y側電壓平均值

	 int32_t  Iy;//Y側電流
	 int32_t  IyAvg;//Y側電流平均值
	 int32_t  IyOffset;//Y側電流偏置
	 int32_t  IL;//電感電流
	 int32_t  ILAvg;//電感電流平均值
	 int32_t  ILOffset;//電感電流偏置

	 int32_t  Vadj;//滑動變阻器電壓值-電壓調節
	 int32_t  VadjAvg;//滑動變阻器電壓平均值

	 int32_t  Iadj;//滑動變阻器電壓值-電流調節
	 int32_t  IadjAvg;//滑動變阻器電壓平均值

	 int32_t  Temp;//溫度NTC採樣值
	 int32_t  TempAvg;//溫度NTC採樣平均值
	 int32_t  Deg;//實際溫度值
};

//主狀態機列舉變量
typedef enum
{
	Init,	//初始化
	Wait,	//閒置等待
	Rise,	//軟啟動
	Run,	//正常運作
	Err		//故障
}STATE_M;

//軟啟動列舉子變量 (軟啟動子狀態)
typedef enum
{
	SSInit,	//軟啟初始化
	SSRun	//開始軟啟
}SState_M;

//標誌位(Flag)結構體
struct  _FLAG
{
	uint16_t SMFlag; //狀態機Flag
	uint16_t ErrFlag; //故障模式Flag
	uint8_t	BBFlag; //運行模式Flag，BUCK模式 Mode=1,Boost模式Mode=2
	uint8_t PWMENFlag; //PWM狀態Flag
	uint8_t KeyFlag1;//按鍵標誌位
	uint8_t KeyFlag2;//按鍵標誌位
	uint8_t OutEn;//輸出標誌位，0關機，1開機
	uint8_t CtlMode;//電源控制方式 1:LCD板按鍵控制 2:上位機控制
	uint8_t cvMode;//電壓電流模式 ,恆壓模式 = 1,恆流模式 = 2
};

//PWM控制變量結構體
struct  _Ctr_value
{
		int32_t		Voref;		//目標電壓
		int32_t		Ioref;		//目標電流
	 	int32_t 	Q1MaxDuty;	//Buck最大占空比
		int32_t		Q2MaxDuty;	//Boost最大占空比
		int32_t		Q1Duty;		//上管MOS的占空比
		int32_t		Q2Duty;		//下管MOS的占空比
};

#define BUCK 1 //定義電路BUCK模式
#define BOOST 2 //定義電路BOOST模式

#define OUT_DIS 0 //關機
#define OUT_EN  1 //開機

#define CTL_MODE_LCDB 1 //電源由LCD控制板按鍵控制
#define CTL_MODE_MONITOR 2 //電源由電腦上位機通訊控制

#define CV_MODE 1 //恆壓模式
#define CC_MODE 2 //恆流模式

#define MIN_Q1_DUTY	100//Q1最小占空比 0.2%占空比-Q15表示
#define MIN_Q2_DUTY	100//Q2最小占空比 0.2%占空比-Q15表示
#define MAX_Q1_DUTY	30767//Q1最大占空比 90%占空比-Q15表示
#define MAX_Q2_DUTY	30767//Q2最大占空比 90%占空比-Q15表示

/***********************故障類型************/
#define F_NOERR		  	0x0000 	//無故障
#define F_SW_VIN_UVP 	0x0001 	//輸入欠壓
#define F_SW_VIN_OVP  	0x0002 	//輸入過壓
#define F_SW_VOUT_UVP	0x0004 	//輸出欠壓
#define F_SW_VOUT_OVP 	0x0008 	//輸出過壓
#define F_SW_IOUT_OCP 	0x0010 	//輸出過流
#define F_SW_SHORT  	0x0020 	//輸出短路
#define F_SW_OTP	 	0x0040 	//過溫保護

/*故障判據宣告*/
#define MAX_BUCK_OCP_VAL		1365//輸出過電流判據-110%的最大輸出電流11A 11*2^12/33
#define MAX_BUCK_VOUT_OVP_VAL	3162//輸出過電壓判據-105%的最大輸出電壓52.5V 52.5*2^12/68
#define MIN_BUCK_VIN_UVP_VAL    663//11V發生輸入欠壓保護 11*2^12/68
#define MAX_BUCK_VIN_OVP_VAL    3072//51V發生輸入過壓保護 51*2^12/68

#define MAX_BOOST_OCP_VAL		745//輸出過電流判據-110%的最大輸出電流6A 6*2^12/33
#define MAX_BOOST_VOUT_OVP_VAL	3162//輸出過電壓判據-105%的最大輸出電壓52.5V 52.5*2^12/68
#define MIN_BOOST_VIN_UVP_VAL   663//11V發生輸入欠壓保護 11*2^12/68
#define MAX_BOOST_VIN_OVP_VAL   3072//51V發生輸入過壓保護 51*2^12/68

#define MAX_SHORT_I				1862//短路電流判據-150%的最大輸出電流15A 15*2^12/33
#define MIN_SHORT_V				301//短路電壓判據-10%的最大輸出電壓5V 5*2^12/68
#define MAX_OTP_VALUE			50//散熱器保護值

#define CAL_VY_K 4081 //電壓矯正參數
#define CAL_VY_B -12 	//電壓矯正參數

/*ADC量測模組及數值打印模組*/
void ADCSample(void);
void ADC1dataPrintf(void);
void ADC2dataPrintf(void);
void ADC1valuePrintf(void);
void ADC2valuePrintf(void);
void RealValuePrintf(void);
void ErrFlagPrintf(void);
void DataPrint(void);

/*溫度感測數位類比轉換模組*/
int32_t CalTempDeg(int32_t val);

/*電路保護模組*/
void VoutSwOVP(uint8_t flag,int32_t ovpValue); //輸出過壓
void VinSwOVP(uint8_t flag,int32_t ovpValue); //輸入過壓
void VinSwUVP(uint8_t flag,int32_t uvpValue); //輸入欠壓
void SwOCP(uint8_t flag,int32_t ocpValue); //過流保護
void ShortOff(uint8_t flag,int32_t vValue,int32_t iValue); //短路保護
void DeviceOTP(int32_t tValue); //過溫保護

/* UART printf重新定向*/
int PUTCHAR_PROTOTYPE(void);

/*按鍵控制及判斷模組*/
void KEYFlag(void);
void OutEnCtl(void);

/*狀態機控制及判斷模組*/
void StateM(void);
void StateMInit(void);
void StateMWait(void);
void StateMRise(void);
void StateMRun(void);
void StateMErr(void);
void LEDShow(void);


/*電源控制方式轉換模組*/
void CtlModeC(void);

/*目標電壓模組*/
void GetVItarget(void);

#define setRegBits(reg, mask) (reg |= (unsigned int)(mask)) //設置故障Flag
#define clrRegBits(reg, mask) (reg &= (unsigned int)(~(unsigned int)(mask))) //清故障Flag
#define getRegBits(reg, mask) (reg &  (unsigned int)(mask)) //取Flag位0/1


#endif /* INC_FUNCTION_H_ */
