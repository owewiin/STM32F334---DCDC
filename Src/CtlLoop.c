/*
 * CtlLoop.c
 *
 *  Created on: Jan 2, 2025
 *      Author: mshome
 */

#include "CtlLoop.h"

/*
** ===================================================================
**     Funtion Name :void BUCK_Voltage_Increse_PID(int32_t Ref,int32_t Real)
**     Description :  BUCK 輸出恆壓控制，增量式離散PID算法
** 		 int32_t Ref：目標值
** 		 int32_t Real：當前值
**     Returns     :占空比Q15
** ===================================================================
*/

 int32_t err0=0;//當前誤差
 int32_t err1=0;//上一時刻誤差
 int32_t err2=0;//上一時刻誤差
 int32_t prop=0;//比例量
 int32_t inte=0;//積份量
 int32_t der=0;//微份量
 int32_t u0=0;//環路輸出
 int32_t u1=0;//上一時刻環路輸出
 void BUCK_Voltage_Increse_PID(int32_t Ref, int32_t Real)
 {
      static int32_t kp = 20543, ki = 32, kd = 500; // 調整後的 PID 參數
      static int32_t deltaU = 0;                    // PID 算法的增量值
      static int32_t inte_max = 5000;               // 積分項最大限制

     err0 = Ref - Real;                             // 計算誤差
     prop = (err0 - err1) * kp;                     // 比例項
     inte += err0 * ki;                             // 積分項累積

     if (inte > inte_max)
    	 inte = inte_max;              				// 限制積分項最大值
     if (inte < -inte_max)
    	 inte = -inte_max;

     der = (err0 - 2 * err1 + err2) * kd;           // 微分項

     deltaU = (prop + inte + der) >> 8;             // 計算增量
     u0 = u1 + deltaU;                              // 輸出計算

     if (u0 > CtrValue.Q1MaxDuty)                   // 輸出限制
         u0 = CtrValue.Q1MaxDuty;

     err2 = err1;                                   // 更新誤差
     err1 = err0;
     u1 = u0;                                       // 更新輸出
 }

/*
** ===================================================================
**     Funtion Name :void BuckModePWMReflash(void)
**     Description : PWM寄存器更新，根據環路計算得到的占空比，更新到PWM寄存器中（同時對占空比進行最大最小值限制）
**     Parameters  :
**     Returns     :無
** ===================================================================
*/
int32_t PERIOD=23039;	//一個開關週期數字量
static int32_t comp1=0,comp2=0,comp3=0,comp4=0;//PWM的比較器值,comp1為Q1PWM關斷點，comp2和comp3為Q2 PWM的開通與管斷點，comp4觸發ADC採樣點
void BuckModePWMReflash(void)
{
	if(CtrValue.Q1Duty > CtrValue.Q1MaxDuty)//環路輸出最大限制
		CtrValue.Q1Duty = CtrValue.Q1MaxDuty;	//CtrValue.Q1MaxDuty該變量在軟啟的過程中由0逐漸增加
	else if(CtrValue.Q1Duty  < MIN_Q1_DUTY)//環路輸出最小限制
		CtrValue.Q1Duty  =MIN_Q1_DUTY;

	CtrValue.Q2Duty = 32767-CtrValue.Q1Duty;//Q2Duty = 1 - Q1Duty；
	if(CtrValue.Q2Duty > CtrValue.Q2MaxDuty)//Q2的占空比也逐漸軟啟
		CtrValue.Q2Duty = CtrValue.Q2MaxDuty;	//CtrValue.Q2MaxDuty該變量在軟啟的過程中由0逐漸增加

	if(DF.PWMENFlag==0)//PWMENFlag是PWM開啟標誌位，當該位為0時,電源未啟動，或者保護
	{
		CtrValue.Q1Duty = MIN_Q1_DUTY;//Q1占空比為0，即關斷
		CtrValue.Q2Duty = MIN_Q2_DUTY;//Q2占空比為0，即關斷
	}

	comp1 = CtrValue.Q1Duty*PERIOD>>15;//第一個比較器值:Q1 的下降沿，CtrValue.Q1Duty變量Q15(2^15)代表100%
	comp2 = comp1+921;//第二個比較器值： Q2 PWM的上升沿位置， Q2 PWM上升沿與上管下降沿保證有921個時鐘週期的死區-4%死區
	comp3 = comp2 + (CtrValue.Q2Duty*PERIOD>>15);//第三個比較器：Q2 PWM的下降沿
	comp4 = comp1>>1;//ADC觸發點為上管PWM的中心點位置
	if(comp2>22120)		comp2=22120;//為了保證下管PWM的下降沿與上管PWM的上升沿有一定的死區為PERIOD-460
	if(comp3>22120)		comp3=22120;//為了保證下管PWM的下降沿與上管PWM的上升沿有一定的死區為PERIOD-460
	if(comp4>7000)		comp4=7000;

	HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP1xR = comp1; //Q2 PWM寄存器更新
	HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP2xR = comp2; //Q1PWM上升沿寄存器更新
	HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP3xR = comp3; //Q1PWM下降沿寄存器更新
	HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP4xR = comp4; //ADC觸發點更新
	HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].TIMxICR = HRTIM_TIM_IT_REP; //HRTIM中斷清零
}

/*
** ===================================================================
**     Funtion Name :  void LoopCtl(void)
**     Description :   環路控制函數，控制對應變量跟隨目標值
**     Parameters  :無
**     Returns     :無
** ===================================================================
*/
int32_t votemp=0; //矯正後的電壓

void LoopCtl(void)
{
	votemp = ADC1_RESULT[1]; //Y端電壓
	votemp = (votemp*CAL_VY_K>>12) + CAL_VY_B; //Y端電壓 矯正
	BUCK_Voltage_Increse_PID(CtrValue.Voref,votemp);//調用PI控制函數
	CtrValue.Q1Duty = u0;//環路輸出等於控制占空比

	BuckModePWMReflash();//占空比更新
}

/*
** ===================================================================
**     Funtion Name :  void ResetLoopValue(void)
**     Description :   復位環路計算中所有變量
**     Parameters  :無
**     Returns     :無
** ===================================================================
*/
void ResetLoopValue(void)
{
	err0=0;//當前誤差
	err1=0;//上一時刻誤差
	err2=0;//上一時刻誤差
	prop=0;//比例量
	inte=0;//積份量
	der=0;//微份量
	u0=0;//環路輸出
	u1=0;//上一時刻環路輸出
}


