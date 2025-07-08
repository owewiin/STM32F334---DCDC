/*
* function.c
 *
 *  Created on: Dec 13, 2024
 *      Author: mshome
 */

#include "RS232.h"
#include "function.h"
#include "main.h"
#include "stm32f3xx_it.h"
#include "stdio.h"
#include "stm32f3xx_hal.h"

/*重定向printf到uart輸出*/
#ifdef __GNUC__
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

extern UART_HandleTypeDef huart3;
extern HRTIM_HandleTypeDef hhrtim1;
extern RS232_DATA	   RS232data;	//通訊數據內容 **Jason補充段**

uint16_t ADC1_RESULT[5]={0,0,0,0,0};//ADC1採樣外設到內存的DMA數據保存寄存器(輸入端:X,輸出端:Y)，電感電流,Y電壓，Y電流,X電壓，X電流
uint16_t ADC2_RESULT[3]={0,0,0};//ADC2採樣外設到內存的DMA數據保存寄存器,電壓電位器，電流電位器，溫度

struct  _ADI SADC={0,0,0,0,2048,0,0,0,0,2048,0,0,2048,0,0,0,0,0,0,0};//採樣變量-平均值-偏置變量結構體
struct  _FLAG  DF={Init,F_NOERR,BUCK,0,0,0,0,0,0};//控制Flag
struct  _Ctr_value CtrValue={0,0,0,0,0,0}; //控制參數

SState_M STState = SSInit ; //軟啟動狀態 Flag初始設定

#define READ_KEY1() HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) //按鍵1電位讀取
#define READ_KEY2() HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14) //按鍵2電位讀取

/** ===================================================================
**     Funtion Name :void StateM(void)
**     Description :   狀態機函數，在5ms中斷中運行，5ms運行一次
**     0-初始化狀態
**     1-等外啟動狀態
**     2-啟動狀態
**     3-運行狀態
**     4-故障狀態
**     Parameters  :
**     Returns     :
** ===================================================================*/
void StateM(void)
{
	switch(DF.SMFlag)	//判斷狀態類型
	{
		case  Init :StateMInit();		break;//初始化狀態
		case  Wait :StateMWait();		break;//等待狀態
		case  Rise :StateMRise();		break;//軟啟動狀態
		case  Run  :StateMRun();		break;//運行狀態
		case  Err  :StateMErr();		break;//故障狀態
	}
}

/** ===================================================================
**     Funtion Name :void KEYFlag(void)
**     Description :兩個按鍵的狀態
**		 默認狀態KEYFlag為0.按下時Flag變1，再次按下Flag變0，依次循環
			 該函數只用以讀取按鍵狀態，並未做按鍵功能處理
**     Parameters  :
**     Returns     :
** ===================================================================*/
void KEYFlag(void)
{
	static int32_t	KeyDownCnt1=0,KeyDownCnt2=0;//按鍵按下的計時器，用以消抖用

	if(READ_KEY1()==0)//按鍵按下時IO電平為0
	{
		KeyDownCnt1++;//按鍵保持計時器計時，按鍵按下*秒有效
		if(KeyDownCnt1 > 30)
		{
			KeyDownCnt1 = 0;//按鍵保持計時器計時復位清0
			if(DF.KeyFlag1==0)//按鍵狀態標誌位翻轉
				DF.KeyFlag1 = 1;//原來為0則更改為1
			else
				DF.KeyFlag1 = 0;//原來為1則更改為0
		}
	}
	else//按鍵無動作
		KeyDownCnt1 = 0;//按鍵保持計時器計時復位清0

	if(READ_KEY2()==0)//按鍵按下時IO電平為0
	{
		KeyDownCnt2++;//按鍵保持計時器計時，按鍵按下*秒有效
		if(KeyDownCnt2 > 30)
		{
			KeyDownCnt2 = 0;//按鍵保持計時器計時復位清0
			if(DF.KeyFlag2==0)//按鍵狀態標誌位翻轉
				DF.KeyFlag2 = 1;//原來為0則更改為1
			else
				DF.KeyFlag2 = 0;//原來為1則更改為0
		}
	}
	else//按鍵無動作
		KeyDownCnt2 = 0;//按鍵保持計時器計時復位清0
}

/** ===================================================================
**     Funtion Name :void CtlModeC(void)
**     Description : 電源控制由LCD還是上位機的方式變換，
			 當方式發生變化時，必須先關閉電源，轉換方式，並初始化控制相關所有變量
**     Parameters  :
**     Returns     :
** ===================================================================*/

#define READ_MODE_PIN() HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4);
uint8_t CtlModeCFlag=0;//當控制模式放生變化時，該位置1
void CtlModeC(void)
{
	static int8_t Flag=0;//暫存變量
	static int8_t PreMode=0;//暫存變量

	Flag = READ_MODE_PIN();//讀取IO口的值，
	if(Flag == 0)//當IO口的值為低時，電源由LCD板控制
		DF.CtlMode = CTL_MODE_LCDB;//由LCD板控制
	else//當IO口的值為高時，電源由上位機控制
		DF.CtlMode = CTL_MODE_MONITOR;//上位機控制

	if(PreMode!=DF.CtlMode)//當方式發生變化時，必須先關閉電源，轉換方式，並初始化控制相關所有變量
	{
		CtlModeCFlag=1;
		HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2);//禁止PWM輸出
		if(DF.SMFlag!=Init)//若當前不處於初始化狀態，則跳轉至等待狀態
			DF.SMFlag = Wait;//跳轉等待狀態

		DF.ErrFlag=0;	//清除故障標誌位
		DF.PWMENFlag=0;//持續關PWM-模塊暫未工作
		DF.KeyFlag1=0;//復位按鍵狀態
		DF.KeyFlag2=0;//復位按鍵狀態
		DF.OutEn=0;//關機狀態
		//RS232data.OutEn=0;

		RS232MInit();
		ResetLoopValue();//環路計算變量參數初始化

		CtrValue.Q1MaxDuty = MIN_Q1_DUTY;//最大上管占空比限制從0開始，啟動後該變量逐漸遞增
		CtrValue.Q2MaxDuty = MIN_Q2_DUTY;	//最大下管占空比限制從0開始，啟動後該變量逐漸遞增
		CtrValue.Q1Duty = MIN_Q1_DUTY;//上管占空比從0開始
		CtrValue.Q2Duty = MIN_Q2_DUTY;//上管占空比從0開始

	}
	PreMode = DF.CtlMode;//保存當前的值
}

/** ===================================================================
**     Funtion Name :void OutEnCtl(void)
**     Description : 判斷模組的輸出是否開啟還是關閉
**     根據電源是有LCD板子控制還是電腦上位機通信控制，不同的方式控制模塊的輸出關閉不同
**     Parameters  :
**     Returns     :
** ===================================================================*/
void OutEnCtl(void)
{
	if(DF.CtlMode == CTL_MODE_LCDB)//電源開關機由LCD板子的按鍵控制
	{
		if((DF.KeyFlag1==1)&&(DF.OutEn==OUT_DIS))//在關機的狀態下當有按鍵1按下時，開機
			DF.OutEn=OUT_EN;//開機標誌位置位
		if((DF.KeyFlag1==0)&&(DF.OutEn==OUT_EN)&&((DF.SMFlag==Rise)||(DF.SMFlag==Run)))//在開機的狀態下，當按鍵1再次按下時關機（機器正在啟動或者運行）
		{
			DF.OutEn = OUT_DIS;//清開機標誌位
			DF.SMFlag = Wait;//跳轉等待狀態
			DF.PWMENFlag=0;//關閉PWM
			HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2);//禁止PWM輸出
		}
	}
	else//電源開關機由電腦上位機通信控制
	{
		if((RS232data.OutEn==1)&&(DF.OutEn==OUT_DIS))//在關機的狀態下上位機發送開機命令，RS232data.OutEn為上位機發送的開關機命令
			DF.OutEn = OUT_EN;
		if((RS232data.OutEn==0)&&(DF.OutEn==OUT_EN)&&((DF.SMFlag==Rise)||(DF.SMFlag==Run)))//在開機的狀態下，收到關機命令時關閉
		{
			DF.OutEn = OUT_DIS;//清開機標誌位
			DF.SMFlag = Wait;//跳轉等待狀態
			DF.PWMENFlag=0;//關閉PWM
			HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2);//禁止PWM輸出
		}
	}
}

/** ===================================================================
**     Funtion Name :void LEDShow(void)
**     Description :  LED顯示函數
**     1-初始化與等待啟動狀態，紅黃綠全亮
**     2-啟動狀態，黃綠亮
**     3-運行狀態，綠燈亮
**     4-故障狀態，紅燈亮
**     Parameters  :
**     Returns     :
** ===================================================================*/
 #define SET_LED_G()	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13,GPIO_PIN_SET)//綠燈亮
 #define SET_LED_Y()	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14,GPIO_PIN_SET)//黃燈亮
 #define SET_LED_R()	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15,GPIO_PIN_SET)//紅燈亮
 #define CLR_LED_G()	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13,GPIO_PIN_RESET)//綠燈滅
 #define CLR_LED_Y()	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14,GPIO_PIN_RESET)//黃燈滅
 #define CLR_LED_R()	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15,GPIO_PIN_RESET)//紅燈滅
void LEDShow(void)
{
	switch(DF.SMFlag)//根據電源工作狀態不同，紅綠黃LED燈顯示不同
	{
		case  Init ://電源初始化狀態
		{
			SET_LED_G();SET_LED_Y();SET_LED_R();break;//紅黃綠全亮
		}
		case  Wait ://電源等待狀態
		{
			SET_LED_G();SET_LED_Y();SET_LED_R();break;//紅黃綠全亮
		}
		case  Rise ://電源軟啟動狀態
		{
			SET_LED_G();SET_LED_Y();CLR_LED_R();break;//黃綠亮，紅滅
		}
		case  Run :	//電源運行狀態
		{
			SET_LED_G();CLR_LED_Y();CLR_LED_R();break;//綠燈亮，黃紅滅
		}
		case  Err :	//電源故障狀態
		{
			CLR_LED_G();CLR_LED_Y();SET_LED_R();break;//紅燈亮，綠黃滅
		}
	}
}

/** ===================================================================
**     Funtion Name :void StateMInit(void)
**     Description : 初始化狀態函數，參數初始化
**     Parameters  :
**     Returns     :
** ===================================================================*/
void StateMInit(void)
{
	static uint16_t Cnt = 0; //定義Counter , 用來計算偏置電壓AVG
	static uint32_t IxSum = 0, IySum = 0, ILSum = 0; //偏置電壓求和變量-求平均值用
	HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2);//禁止PWM輸出
	DF.ErrFlag=0;	//清除故障標誌位
	DF.BBFlag = BUCK; //BUCK工作模式 (Buck / Boost 工作模式要設定)
	DF.PWMENFlag=0;//持續關PWM-模塊暫未工作
	DF.KeyFlag1=0;//復位按鍵狀態
	DF.KeyFlag2=0;//復位按鍵狀態
	DF.cvMode=0;//復位恆壓恆流模式
	DF.OutEn=0;//關機狀態

	CtrValue.Voref=0; //復位目標電壓
	CtrValue.Ioref=0; //復位目標電流

	CtrValue.Q1MaxDuty = MIN_Q1_DUTY;//最大上管占空比限制從最小開始，啟動後該變量逐漸遞增 0占空比不得為0 硬體因素
	CtrValue.Q2MaxDuty = MIN_Q2_DUTY;//最大下管占空比限制從最小開始，啟動後該變量逐漸遞增 0占空比不得為0 硬體因素
	CtrValue.Q1Duty = MIN_Q1_DUTY;//上管占空比從最小開始
	CtrValue.Q2Duty = MIN_Q2_DUTY;//上管占空比從最小開始

	RS232MInit(); 	//通訊協議相關參數初始化
	ResetLoopValue();//環路計算變量參數初始化

	Cnt ++;
	if(Cnt==256)	//累計求和256次 求各個變量的偏置電壓平均值
	{
		SADC.IxOffset = IxSum >>8;	//(BUCK角度)輸入端信號調變電路偏置電壓平均值
		SADC.IyOffset = IySum >>8;	//(BUCK角度)輸出端信號調變電路偏置電壓平均值
		SADC.ILOffset = ILSum >>8;	//(BUCK角度)電感電流信號調變電路偏置電壓平均值

		Cnt =0;
		IxSum=0;	//變量清0, 防止下次啟動該值繼續疊加
		IySum=0;	//變量清0, 防止下次啟動該值繼續累加
		ILSum=0;	//變量清0, 防止下次啟動該值繼續累加
		DF.SMFlag = Wait; //主狀態機跳轉至等待狀態
	}
	else
	{
		IxSum+= ADC1_RESULT[4]; 	//(BUCK角度)輸入端信號調變電路偏置電壓求和
		IySum+= ADC1_RESULT[2];	//(BUCK角度)輸出端信號調變電路偏置電壓求和
		ILSum+= ADC1_RESULT[0];	//(BUCK角度)電桿電流信號調變電路偏置電壓求和
	}
}


/** ===================================================================
**     Funtion Name :void StateMWait(void)
**     Description :   等待狀態，
**     完成偏置電壓計算後等待*S後無故障
**     且開機標誌位置位時，跳轉軟起狀態
**     Parameters  :
**     Returns     :
** ===================================================================*/
void StateMWait(void)
{
	static uint16_t Cnt = 0;//計數器定義,用以計算偏置平均值

	HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2);//禁止PWM輸出
	DF.PWMENFlag=0;	//持續關PWM-模塊暫未工作
	ResetLoopValue();//環路計算變量參數初始化
	Cnt ++;	//計數器累加
	if(Cnt>100)	//等待*S，當小於*S時等待，大於*秒時
	{
		Cnt=100;//計數器保持，等待開機指令
		if((DF.ErrFlag==F_NOERR)&&(DF.OutEn==OUT_EN))//檢測電源並無故障，收到開機指令
		{
			Cnt=0;//計數器復位
			DF.SMFlag  = Rise;//主狀態機跳轉至軟啟動狀態
			STState = SSInit;	//軟啟動子狀態跳轉至初始化狀態
		}
		else if((DF.ErrFlag==F_NOERR)&&(RS232data.OutEn==OUT_EN))//**Jason補充段**
		{
			Cnt=0;//計數器復位
			DF.SMFlag  = Rise;//主狀態機跳轉至軟啟動狀態
			STState = SSInit;	//軟啟動子狀態跳轉至初始化狀態
		}
	}
}
/*
** ===================================================================
**     Funtion Name : void StateMRise(void)
**     Description :軟啟階段
**     0-軟啟初始化
**     1-軟啟等待
**     2-開始軟啟
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
void StateMRise(void)
{
	static  uint16_t	Q1Cnt=0,Q2Cnt=0;//5mS計數器，PWM軟起的計時器

	switch(STState)	//判斷軟啟狀態
	{
		case    SSInit://初始化狀態,一些啟動參數初始化
		{
			DF.PWMENFlag=0;//持續關PWM-模塊暫不發PWM
			HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2);//禁止PWM輸出
			ResetLoopValue();//環路計算變量參數初始化
			CtrValue.Q1MaxDuty = MIN_Q1_DUTY;//最大上管占空比限制從最小開始，啟動後該變量逐漸遞增
			CtrValue.Q2MaxDuty = MIN_Q2_DUTY;	//最大下管占空比限制從最小開始，啟動後該變量逐漸遞增
			CtrValue.Q1Duty = MIN_Q1_DUTY;//上管占空比從最小開始
			CtrValue.Q2Duty = MIN_Q2_DUTY;//上管占空比從最小開始

			if(DF.BBFlag == BUCK)//當工作於BUCK模式時，目標電壓從0開始
				CtrValue.Voref  = 0;//目標電壓從零逐漸開始遞增
			else//否則工作於BOOST模式時，Y端為輸入端，
				CtrValue.Voref  = SADC.Vy;//目標電壓從輸入逐漸開始遞增--boost待機時輸出等於輸出

			CtrValue.Ioref  = 0;//目標電流從零逐漸開始遞增

			Q1Cnt =0;//PWM軟起計時器計數
			Q2Cnt =0;//PWM軟起計時器計數
			STState = SSRun;//跳轉至軟啟等待狀態
			break;
		}
		case    SSRun://正式開始軟啟動
		{
			if(DF.BBFlag == BUCK)//工作於BUCK模式，上管Q1先軟起，Q1軟起後同步管Q2再軟起
			{
				if(DF.PWMENFlag==0)//在正式發波前先復位環路參量
				{
					ResetLoopValue();//環路計算變量參數初始化
					HAL_HRTIM_WaveformOutputStart(&hhrtim1,HRTIM_OUTPUT_TA1);//正式允許Q1 PWM輸出
				}
				DF.PWMENFlag=1;//發波標誌位置位，正式開始發PWM波
				if(CtrValue.Q1MaxDuty < MAX_Q1_DUTY)
				{
					Q1Cnt++;//計數器正式開始計數，5mS加1
					CtrValue.Q1MaxDuty= Q1Cnt*300;//Q1管占空比幅值增加
					if(CtrValue.Q1MaxDuty > MAX_Q1_DUTY)//累加到最大值
					{
						CtrValue.Q1MaxDuty  = MAX_Q1_DUTY ;//管Q1管軟起結束後，同步管Q2正式發波
						HAL_HRTIM_WaveformOutputStart(&hhrtim1,HRTIM_OUTPUT_TA2);//正式允許Q2 PWM輸出-啟動完成後互補管開通
					}
					//maxInte = CtrValue.Q1MaxDuty<<8;//環路計算的積份量
				}
				else
				{
					Q2Cnt++;//計數器正式開始計數，5mS加1
					CtrValue.Q2MaxDuty= Q2Cnt*300;//Q2占空比限制從最小開始
					if(CtrValue.Q2MaxDuty > MAX_Q2_DUTY)//累加到最大值
						CtrValue.Q2MaxDuty  = MAX_Q2_DUTY ;
				}
			}


			else//工作BOOST模式，下管Q2先軟起，Q2軟起後同步管Q1再軟起
			{
				if(DF.PWMENFlag==0)//在正式發波前先復位環路參量
				{
					ResetLoopValue();//環路計算變量參數初始化
					HAL_HRTIM_WaveformOutputStart(&hhrtim1,HRTIM_OUTPUT_TA2);//正式允許Q2 PWM輸出
				}
				DF.PWMENFlag=1;//發波標誌位置位，正式開始發PWM波
				if(CtrValue.Q2MaxDuty < MAX_Q2_DUTY)
				{
					Q2Cnt++;//計數器正式開始計數，5mS加1
					CtrValue.Q2MaxDuty= Q2Cnt*300;//Q2管占空比幅值增加
					if(CtrValue.Q2MaxDuty > MAX_Q2_DUTY)//累加到最大值
					{
						CtrValue.Q2MaxDuty  = MAX_Q2_DUTY ;//下管Q2軟起結束後，同步管Q1正式發波
						HAL_HRTIM_WaveformOutputStart(&hhrtim1,HRTIM_OUTPUT_TA1);//正式允許Q1 PWM輸出-啟動完成後互補管開通
					}
				}
				else
				{
					Q1Cnt++;//計數器正式開始計數，5mS加1
					CtrValue.Q1MaxDuty= Q1Cnt*300;//Q1占空比限制從0開始
					if(CtrValue.Q1MaxDuty > MAX_Q1_DUTY)//累加到最大值
						CtrValue.Q1MaxDuty  = MAX_Q1_DUTY ;
				}
			}

			if((CtrValue.Q1MaxDuty==MAX_Q1_DUTY)&&(CtrValue.Q2MaxDuty==MAX_Q2_DUTY))//當Q1和Q2的最大占空比限制達到最大，則認為軟啟結束
			{
				DF.SMFlag  = Run;	//主狀態機跳轉至正常運行運行狀態
				STState = SSInit;	//軟啟動子狀態跳轉至初始化狀態
			}
			break;
		}

		default:
		break;
	}
}
/*
** ===================================================================
**     Funtion Name :void StateMRun(void)
**     Description :正常運行，主處理函數在中斷中運行
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
void StateMRun(void)
{

}
/*
** ===================================================================
**     Funtion Name :void StateMErr(void)
**     Description :故障狀態
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
void StateMErr(void)
{
	HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2);//禁止PWM輸出
	DF.PWMENFlag=0;//關閉PWM
	if(DF.ErrFlag==F_NOERR)	//若故障消除跳轉至等待重新軟啟
		DF.SMFlag  = Wait;
}

/** ===================================================================
**     Funtion Name :void GetVItarget(void)
**     Description : 獲取電源的輸出目標電壓電流值
**		 判定電源由LCD板控制還是由電腦上位機控制，由LCD板控制，目標電壓電流由旋鈕調節
			 由上位機控制，目標電壓電流來至上位機的指令值。
**     Parameters  :
**     Returns     :
** ===================================================================*/
#define MAX_VREF    3012//最大目標電壓50V  50*2^12/68
#define MIN_VREF    0//最小目標電壓0V
#define VREF_K      6//目標電壓數字量每*ms遞增或遞減步長*V
#define MAX_IREF    621//最大目標電流5A  5*2^12/33 --BOOST模式
#define MAX_IREF1   1242//最大目標電流10A  10*2^12/33  --BUCK模式
#define MIN_IREF    0//最小目標電壓0A
#define IREF_K      25//目標電壓數字量每*ms遞增或遞減步長0.2A 0.2*2^12/33
void GetVItarget(void)
{
	static int32_t vTemp=0;//目標電壓值中間變量
	static int32_t iTemp=0;//目標電流值中間變量

	if(DF.CtlMode == CTL_MODE_LCDB)//電源電壓電流目標值由LCD板子的旋鈕控制
	{
		//電位器電壓範圍0~1.65V，數字量0~2048，目標電壓0~50V，對應數字為0~3012；
		//目標電壓數字量=電位器電壓數字量*1.7（Q12）-200;減去200,避免電位器在0電壓位置不准
		vTemp = (SADC.VadjAvg*6963>>12)-200;	//SADC.VadjAvg為電壓旋鈕滑動電位器電壓，
		if(vTemp<0)//最小目標電壓0，需要注意的是boost的輸出電壓時刻都大於輸入電壓
			vTemp=0;

		if(DF.BBFlag == BUCK)//工作於BUCK模式，輸出目標電流最大10A
		{
			//電位器電壓範圍0~1.65V，數字量0~2048，目標電壓0~10A，對應數字為0~1241；
			//目標電流數字量=電位器電壓數字量*0.75（Q12）-200;減去200,避免電位器在0電壓位置不准
			iTemp = (SADC.IadjAvg*3072>>12)-200;	//SADC.IadjAvg為電流旋鈕滑動電位器電壓，

		}
		else//工作於BOOST模式，輸出目標電流最大5A
		{
			//電位器電壓範圍0~1.65V，數字量0~2048，目標電壓0~5A，對應數字為0~621；
			//目標電流數字量=電位器電壓數字量*0.45（Q12）-200;減去200,避免電位器在0電壓位置不准
			iTemp = (SADC.IadjAvg*1843>>12)-200;	//SADC.IadjAvg為電流旋鈕滑動電位器電壓，
		}

	}
	else//電源電壓電流目標值由上位機指令控制
	{
		vTemp = ((int32_t)RS232data.vRef)*2467>>12;//將上位機發送的指令轉換成實際的電壓數字量
		iTemp = ((int32_t)RS232data.iRef)*5083>>12;//將上位機發送的指令轉換成實際的電流數字量
	}

	if( vTemp> ( CtrValue.Voref + VREF_K))//設定的目標電壓大於現目標電壓+步長
			CtrValue.Voref = CtrValue.Voref + VREF_K;//當前目標電壓=原目標電壓+步長，實現目標電壓緩慢上升
	else if( vTemp < ( CtrValue.Voref - VREF_K ))
			CtrValue.Voref =CtrValue.Voref - VREF_K;//實現目標電壓緩慢下降
	else
			CtrValue.Voref = vTemp;//最終達到設定值

	if( iTemp> ( CtrValue.Ioref + IREF_K))//設定的目標電流大於現目標電流+步長
			CtrValue.Ioref = CtrValue.Ioref + IREF_K;//當前目標電流=原目標電流+步長，實現目標電流緩慢上升
	else if( iTemp < ( CtrValue.Ioref - IREF_K ))
			CtrValue.Ioref =CtrValue.Ioref - IREF_K;//實現目標電流緩慢下降
	else
			CtrValue.Ioref = iTemp;//最終達到滑動電位器設定值

	if(CtrValue.Voref>MAX_VREF)//限制目標電壓最大值
		CtrValue.Voref=MAX_VREF; //如超過目標最大值 則強制等於最大值
	if(CtrValue.Voref<MIN_VREF)//限制目標電壓最小值
		CtrValue.Voref=MIN_VREF; //如降低目標最小值 則強制等於最小值

	if(DF.BBFlag == BUCK)//工作於BUCK模式，最大限制10A
	{
		if(CtrValue.Ioref>MAX_IREF1)//限制目標電流最大值
			CtrValue.Ioref=MAX_IREF1;
	}
	else//工作於BOOST模式，最大限制5A
	{
		if(CtrValue.Ioref>MAX_IREF)//限制目標電流最大值
			CtrValue.Ioref=MAX_IREF;
	}

	if(CtrValue.Ioref<MIN_IREF)//限制目標電流最小值
		CtrValue.Ioref=MIN_IREF;
}

/*
** ===================================================================
**     Funtion Name :   void ADCSample(void)
**     Description :    採樣電源X側電壓/電流、Y側電壓/電流、電感電流等參數,並滑動方式求平均值
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void ADCSample(void)
{
	//輸入輸出採樣參數求和，用以計算平均值
	static uint32_t VxSum=0,IxSum=0,VySum=0,IySum=0,VaSum=0,IaSum=0,TSum=0;

	//從DMA緩衝器中獲取數據 Q12,並對其進行線性矯正-採用線性矯正
	SADC.Vx  = ADC1_RESULT[3];//X側端電壓
	SADC.Vy  = ((int32_t)ADC1_RESULT[1]*CAL_VY_K>>12) + CAL_VY_B;//右側端電壓

	if(DF.BBFlag == BUCK)//當工作於BUCK模式時，X端為輸入端
	{
		SADC.Ix  = (((int32_t)ADC1_RESULT[4]) - SADC.IxOffset );//左側X端電流，硬件信號調理電路有1.65V偏置
		SADC.Iy  = (((int32_t)ADC1_RESULT[2]) - SADC.IyOffset );//右側Y端電流，硬件信號調理電路有1.65V偏置
		SADC.IL  = (((int32_t)ADC1_RESULT[0]) - SADC.ILOffset);//電感電流，硬件信號調理電路有1.65V偏置
	}
	else//否則工作於BOOST模式時，Y端為輸入端，
	{
		SADC.Ix  = (SADC.IxOffset - ((int32_t)ADC1_RESULT[4]));//左側X端電流，硬件信號調理電路有1.65V偏置
		SADC.Iy  = (SADC.IyOffset - ((int32_t)ADC1_RESULT[2]));//右側Y端電流，硬件信號調理電路有1.65V偏置
		SADC.IL  = (SADC.ILOffset - ((int32_t)ADC1_RESULT[0]));//電感電流，硬件信號調理電路有1.65V偏置
	}

	SADC.Vadj = ADC2_RESULT[0];//滑動電位器1電壓-用來做電壓目標值
	SADC.Iadj = ADC2_RESULT[1];//滑動電位器2電壓-用來做電流目標值
	SADC.Temp = ADC2_RESULT[2];//溫度NTC電壓-底板溫度

	if(SADC.Vx<0)SADC.Vx=0;//最小限制-避免矯正後出現負值
	if(SADC.Vy<0)SADC.Vy=0;//最小限制-避免矯正後出現負值
	if(SADC.Ix<0)SADC.Ix=0;//最小限制-避免取樣擾動減去偏置後出現負值
	if(SADC.Iy<0)SADC.Iy=0;//最小限制-避免取樣擾動減去偏置後出現負值
	if(SADC.IL<0)SADC.IL=0;//最小限制-避免取樣擾動減去偏置後出現負值

	//計算各個採樣值的平均值-滑動平均方式
	//求平均值的求和數據量個數取決於需要多大的平均量-經驗得到
	VxSum = VxSum + SADC.Vx -(VxSum>>6);//求和，新增入一個新的採樣值，同時減去之前的平均值。
	SADC.VxAvg = VxSum>>6;//求平均-左側端電壓
	IxSum = IxSum + SADC.Ix -(IxSum>>6);//求和，新增入一個新的採樣值，同時減去之前的平均值。
	SADC.IxAvg = IxSum>>6;//求平均-左側端電流
	VySum = VySum + SADC.Vy -(VySum>>6);//求和，新增入一個新的採樣值，同時減去之前的平均值。
	SADC.VyAvg = VySum>>6;//求平均-右側端電壓
	IySum = IySum + SADC.Iy -(IySum>>6);//求和，新增入一個新的採樣值，同時減去之前的平均值。
	SADC.IyAvg = IySum>>6;//求平均-右側端電流
	VaSum = VaSum + SADC.Vadj -(VaSum>>8);//求和，新增入一個新的採樣值，同時減去之前的平均值。
	SADC.VadjAvg = VaSum>>8;//滑動電位器1平均電壓
	IaSum = IaSum + SADC.Iadj -(IaSum>>8);//求和，新增入一個新的採樣值，同時減去之前的平均值。
	SADC.IadjAvg = IaSum>>8;//滑動電位器1平均電壓
	TSum = TSum + SADC.Temp -(TSum>>8);//求和，新增入一個新的採樣值，同時減去之前的平均值。
	SADC.TempAvg = TSum>>8;//滑動電位器1平均電壓
}

/*
** ===================================================================
**     Funtion Name :   void ADC1dataPrintf(void)
**     Description :    打印ADC1 所有採樣數據
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void ADC1dataPrintf(void)
{
	printf("ADC1_RESULT[0-4]= %d, %d, %d, %d, %d\n",ADC1_RESULT[0],ADC1_RESULT[1],ADC1_RESULT[2],ADC1_RESULT[3],ADC1_RESULT[4]);
}

/*
** ===================================================================
**     Funtion Name :   void ADC2dataPrintf(void)
**     Description :    打印ADC2 所有採樣數據
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void ADC2dataPrintf(void)
{
	printf("ADC2_RESULT[0-2]= %d, %d, %d\n",ADC2_RESULT[0],ADC2_RESULT[1],ADC2_RESULT[2]);
}
/*
** ===================================================================
**     Funtion Name :   void ADC1valuePrintf(void)
**     Description :    打印ADC1 所有口電壓
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void ADC1valuePrintf(void)
{
	/*
	double ADC1val[5]={0,0,0,0,0};//控制器ADC1端口對應的電壓值0~3.3V

	for(uint8_t i=0;i<5;i++)//控制器ADC1端口對應的電壓值 = 採樣數字量*3.3/Q12
		ADC1val[i]=((double)ADC1_RESULT[i]*3.3/4096.0);

	printf("ADC1val[0-4]= %2.2fV, %2.2fV, %2.2fV, %2.2fV, %2.2fV\n",ADC1val[0],ADC1val[1],ADC1val[2],ADC1val[3],ADC1val[4]);
	*/

	double ADC1val[5]={0,0,0,0,0};//控制器ADC1端口對應的電壓值0~3.3V
	double temp=0;//中間量

	for(uint8_t i=0;i<5;i++)//控制器ADC1端口對應的電壓值 = 採樣數字量*3.3/Q12
	{
		temp = ADC1_RESULT[i];
		ADC1val[i]=(temp*3.3/4096.0);
	}

	printf("ADC1val[0-4]= %2.2fV, %2.2fV, %2.2fV, %2.2fV, %2.2fV\n",ADC1val[0],ADC1val[1],ADC1val[2],ADC1val[3],ADC1val[4]);
}
/*
** ===================================================================
**     Funtion Name :   void ADC2valuePrintf(void)
**     Description :    打印ADC2 所有口電壓
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void ADC2valuePrintf(void)
{
	double ADC2val[3]={0,0,0};//控制器ADC2端口對應的電壓值0~3.3V
	double temp=0;//中間量

	for(uint8_t i=0;i<3;i++)//控制器ADC2端口對應的電壓值 = 採樣數字量*3.3/Q12
	{
		temp = ADC2_RESULT[i];
		ADC2val[i]=(temp*3.3/4096.0);
	}

	printf("ADC2val[0-2]= %2.2fV, %2.2fV, %2.2fV \n",ADC2val[0],ADC2val[1],ADC2val[2]);
	/*
	double ADC2val[3]={0,0,0};//控制器ADC2端口對應的電壓值0~3.3V

	for(uint8_t i=0;i<3;i++)//控制器ADC2端口對應的電壓值 = 採樣數字量*3.3/Q12
		ADC2val[i]=((double)ADC2_RESULT[i]*3.3/4096.0);

	printf("ADC2val[0-2]= %2.2fV, %2.2fV, %2.2fV \n",ADC2val[0],ADC2val[1],ADC2val[2]);
	*/
}

/*
** ===================================================================
**     Funtion Name :   void RealValuePrintf(void)
**     Description :    打印實際電源X，Y端電壓電流
**     Parameters  :
**     Returns     :
** ===================================================================
*/
#define VX_MAX 68.0//X電壓最大值
#define IX_MAX 33.0//X電流最大值
#define VY_MAX 68.0//Y電壓最大值
#define IY_MAX 33.0//Y電流最大值
#define IL_MAX 33.0//電感電流最大值
void RealValuePrintf(void)
{
	double temp=0;//中間變量
	double RealVal1[5]={0,0,0,0,0};//ADC1端口對應實際電壓電流

	temp = SADC.IL;
	RealVal1[0]= (double)temp*IL_MAX/4096.0;//計算實際電感電流

	temp = SADC.Vy;
	RealVal1[1]= (double)temp*VY_MAX/4096.0;//計算實際Y電壓

	temp = SADC.Iy;
	RealVal1[2]= (double)temp*IY_MAX/4096.0;//計算實際Y電流

	temp = SADC.Vx;
	RealVal1[3]= (double)temp*VX_MAX/4096.0;//計算實際X電壓

	temp = SADC.Ix;
	RealVal1[4]= (double)temp*IX_MAX/4096.0;//計算實際X電流

	printf("IL=%2.2fA, 輸出電壓=%2.2fV, 輸出電流=%2.2fA, 輸入電壓=%2.2fV, 輸入電流=%2.2fA \n",RealVal1[0],RealVal1[1],RealVal1[2],RealVal1[3],RealVal1[4]);
}
/** ===================================================================
**     Funtion Name :void DataPrint(void)
**     Description :數據打印函數，將特定數據發送至電腦的串口助手
打印函數占時間資源較多，不能連續打印，只能特定時間間隔打印輸出
**     Parameters  :
**     Returns     :
** ===================================================================*/
uint32_t TxCnt=0;//計時器，
void DataPrint(void)
{
	if(TxCnt>400)//TxCnt在5ms中斷中計算一次，2S發送一次數據
	{
		TxCnt=0;
		//ADC1dataPrintf();//打印ADC1 所有採樣值數據
		//ADC1valuePrintf();//打印ADC1 所有X電壓
		RealValuePrintf();//打印實際電源X，Y端電壓電流，電感電流
		//ADC2dataPrintf();//打印ADC2 所有採樣數據
		//ADC2valuePrintf();//打印ADC2 所有Y電壓
		ErrFlagPrintf();
		printf("\r\n");//換行
	}
}

/*
** ===================================================================
**     Funtion Name :void VoutSwOVP(uint8_t flag,int32_t ovpValue)
**     Description :
**     輸出過壓保護，預先判定BUCK模式運行還是BOOST模式來決定用X端還是Y端的電壓作為輸出過壓保護的變量
**     當電源檢測輸出電壓大於一定閾值，且在一定時間內保持該條件，則判定輸出為過壓；
**     過壓容易發生器件超壓擊穿風險，不建議電源自動恢復重啟
**     Parameters  :
**     flag：運行模式 BUCK模式或者BOOST模式；
**     ovpValu：過壓保護值數字量
**     Returns     : none
** ===================================================================
*/

void VoutSwOVP(uint8_t flag,int32_t ovpValue)
{
	static  int32_t  Cnt=0;//過壓判據保持時間計數器
	static 	int32_t	 vo=0;//輸出電壓變量

	if(flag == BUCK)//當工作於BUCK模式時，Y端為輸出端
		vo=SADC.Vy;
	else//否則工作於BOOST模式時，X端為輸出端，
		vo=SADC.Vx;

	if (vo > ovpValue)//當輸出電壓大於*V；
	{
		Cnt++;//過壓條件保持計時
		if(Cnt > 10) //過壓條件保持*毫秒，則認為過壓發生
		{
			Cnt = 0;//過壓判據保持時間計數器清零
			HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2);//禁止PWM輸出
			DF.PWMENFlag=0; //關閉PWM
			setRegBits(DF.ErrFlag,F_SW_VOUT_OVP);//DF.ErrFlag中對應故障類型的Flag位置1 進入0x0008 故障碼
			DF.SMFlag  =Err;//主狀態機跳轉至故障狀態
		}
	}
	else//過壓條件不滿足，
		Cnt  = 0;//計數器一直復位清0
}

/*
** ===================================================================
**     Funtion Name :CCMRAM void VinSwOVP(uint8_t flag,int32_t ovpValue)
**     Description :
**     輸入過壓保護
**     預先判定BUCK模式運行還是BOOST模式來決定用X端還是Y端的電壓作為輸入欠壓保護的變量
**     當電源檢測輸入電壓大於一定閾值，且在一定時間內保持該條件，則判定輸入過壓；
**     當發生輸入過壓後，電源關閉輸出，輸入過壓標誌位置位；
**     當發生輸入過壓後，電源檢測輸入電壓小於一定閾值，且在一定時間內保持該條件，則電源恢復正常運行；
**     這裡過壓保護的閾值，和恢復的電壓閾值有一定電壓差，避免供電電源的負載效應電壓下降的特性
**     Parameters  : none
**     flag：運行模式 BUCK模式或者BOOST模式；
**     ovpValue：過壓保護值數字量
**     Returns     : none
** ===================================================================
*/

void VinSwOVP(uint8_t flag,int32_t ovpValue)
{
	static  int32_t  Cnt=0; //輸入過壓判據保持時間計數器
	static  int32_t	 RSCnt=0; //輸入過壓恢復判據保持時間計數器
	static 	int32_t	 vi=0; //輸入電壓變量

	if(flag == BUCK) //當工作於BUCK模式時，X端為輸入端
		vi=SADC.Vx;
	else //否則工作於BOOST模式時，Y端為輸入端，
		vi=SADC.Vy;

	if ((vi > ovpValue) && (DF.SMFlag != Init )) //當輸入電壓大於*V；排除電源初始化時還沒啟動採樣
	{
		Cnt++; //輸入過壓條件保持計時
		if(Cnt > 200) //輸入過壓保持*毫秒，則認為過輸入過壓 計時時間較長是因為輸入能量會有波動
		{
			Cnt = 0; //輸入過壓判據保持時間計數器清零
			DF.PWMENFlag=0; //關閉PWM
			HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //禁止PWM輸出
			setRegBits(DF.ErrFlag,F_SW_VIN_OVP); //DF.ErrFlag中對應故障類型的標誌位置1
			DF.SMFlag  =Err;//主狀態機跳轉至故障狀態-等待重新啟動
		}
	}
	else//輸入過壓條件不滿足，
		Cnt  = 0;//計數器一直復位清0

	if(getRegBits(DF.ErrFlag,F_SW_VIN_OVP))//當輸入過壓發生，電源關機
	{
		//保護閾值和恢復閾值一定要相差一定值，避免來回保護和恢復
		if(vi < (ovpValue-60)) //若輸入電壓恢復至正常狀態，則手動清除故障，進入等待狀態等待重啟
		{
			RSCnt++;//輸入過壓恢復條件保持計時
			if(RSCnt>200)//等待*秒
			{
				RSCnt=0;//時間計數器清零；一定要清零。
				clrRegBits(DF.ErrFlag,F_SW_VIN_OVP);//清除DF.ErrFlag中對應故障類型的標誌位
			}
		}
		else	//輸入過壓並未在正常範圍
			RSCnt=0;	//計數器一直復位清0
	}
	else//並未發生輸入過壓故障
		RSCnt=0;//計數器一直復位清0
}
/*
** ===================================================================
**     Funtion Name :VinSwUVP(uint8_t flag,int32_t uvpValue)
**     Description :
**     輸入欠壓保護
**     預先判定BUCK模式運行還是BOOST模式來決定用X端還是Y端的電壓作為輸入欠壓保護的變量
**     當電源檢測輸入電壓小於一定閾值，且在一定時間內保持該條件，則判定輸入欠壓；
**     當發生輸入欠壓後，電源關閉輸出，輸入欠壓標誌位置位；
**     當發生輸入欠壓後，電源檢測輸入電壓大於一定閾值，且在一定時間內保持該條件，則電源恢復正常運行；
**     這裡欠壓保護的閾值，和恢復的電壓閾值有一定電壓差，避免供電電源的負載效應電壓下降的特性
**     Parameters  : none
**     flag：運行模式 BUCK模式或者BOOST模式；
**     uvpValue：欠壓保護值數字量
**     Returns     : none
** ===================================================================
*/

void VinSwUVP(uint8_t flag,int32_t uvpValue)
{
	static  int32_t  Cnt=0;//輸入欠壓判據保持時間計數器
	static  int32_t	 RSCnt=0;//輸入欠壓恢復判據保持時間計數器
	static 	int32_t	 vi=0;//輸入電壓變量

	if(flag == BUCK)//當工作於BUCK模式時，X端為輸入端
		vi=SADC.Vx;
	else//否則工作於BOOST模式時，Y端為輸入端，
		vi=SADC.Vy;

	if ((vi < uvpValue) && (DF.SMFlag != Init ))//當輸入電壓小於*V；排除電源初始化時還沒啟動採樣
	{
		Cnt++;//輸入欠壓條件保持計時
		if(Cnt > 200)//輸入欠壓保持*秒，則認為過輸入欠壓
		{
			Cnt = 0;//輸入欠壓判據保持時間計數器清零
			HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2);//禁止PWM輸出
			DF.PWMENFlag=0;//關閉PWM
			setRegBits(DF.ErrFlag,F_SW_VIN_UVP);//DF.ErrFlag中對應故障類型的標誌位置1
			DF.SMFlag  = Err;//主狀態機跳轉至故障狀態-等待重新啟動
		}
	}
	else//輸入欠壓條件不滿足，
		Cnt  = 0;//計數器一直復位清0

	if(getRegBits(DF.ErrFlag,F_SW_VIN_UVP))//當輸入欠壓發生，電源關機
	{
		//保護閾值和恢復閾值一定要相差一定值，避免來回保護和恢復
		if(vi> (uvpValue+60)) //若輸入電壓恢復至正常狀態，則手動清除故障，進入等待狀態等待重啟,恢復點=保護點+1V
		{
			RSCnt++;//輸入欠壓恢復條件保持計時
			if(RSCnt>200)//等待*秒
			{
				RSCnt=0;//時間計數器清零；一定要清零。
				clrRegBits(DF.ErrFlag,F_SW_VIN_UVP);//清除DF.ErrFlag中對應故障類型的標誌位
			}
		}
		else	//輸入欠壓並未在正常範圍
			RSCnt=0;	//計數器一直復位清0
	}
	else//並未發生輸入欠壓故障
		RSCnt=0;//計數器一直復位清0
}

/*
** ===================================================================
**     Funtion Name :CCMRAM void SwOCP(uint8_t flag,int32_t ocpValue)
**     Description :
**     輸出過流保護
**     預先判定BUCK模式運行還是BOOST模式來決定用X端還是Y端的電流作為輸出過流保護的電流變量
**     當電源檢測輸出電流大於一定閾值，且在一定時間內保持該條件，則判定輸出為過流；
**     當發生輸出過流後，電源關閉輸出，過流標誌位置位；
**     電源關機後等待*秒後重新啟動；
**     Parameters  :
**     flag：運行模式 BUCK模式或者BOOST模式；
**     ocpValue：過流保護值數字量
**     Returns     : none
** ===================================================================
*/
void SwOCP(uint8_t flag,int32_t ocpValue)
{
	static  int32_t  Cnt=0;//過流判據保持時間計數器
	static  int32_t  RSCnt=0;//過流故障恢復等待的時間計數器
	static 	int32_t	 io=0;//輸出電流變量

	if(flag == BUCK)//當工作於BUCK模式時，Y端為輸出端，輸出電流為	SADC.Iy
		io=SADC.Iy;
	else//否則工作於BOOST模式時，X端為輸入端，輸出電流為SADC.Ix
		io=SADC.Ix;

	if((io> ocpValue)&&(DF.SMFlag  == Run))//當輸出電流大於*A；只有在電源運行過程中才檢測輸出是否過流
	{
		Cnt++;//過流條件保持計時
		if(Cnt>50)//過流條件保持*秒，則認為過流發生
		{
			Cnt = 0;//過流判據保持時間計數器清零
			HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2);//禁止PWM輸出
			DF.PWMENFlag=0;//關閉PWM
			setRegBits(DF.ErrFlag,F_SW_IOUT_OCP);//DF.ErrFlag中對應故障類型的標誌位置1
			DF.SMFlag  =Err;//主狀態機跳轉至故障狀態-等待重新啟動
		}
	}
	else//過流條件不滿足，
		Cnt  = 0;//計數器一直復位清0

	if(getRegBits(DF.ErrFlag,F_SW_IOUT_OCP))//當過流故障發生，電源關機，等待*秒，手動清除故障，進入等待狀態等待重啟
	{
		RSCnt++;//等待故障清楚計數器累加
		if(RSCnt>3000)//等待*秒
		{
			RSCnt=0;//時間計數器清零；一定要清零。
			clrRegBits(DF.ErrFlag,F_SW_IOUT_OCP);//清除DF.ErrFlag中對應故障類型的標誌位
		}
	}
}

/*
** ===================================================================
**     Funtion Name :void ShortOff(uint8_t flag,int32_t vValue,int32_t iValue)
**     Description :
**     輸出短路保護
**     預先判定BUCK模式運行還是BOOST模式來決定用X端還是Y端的電流作為輸出過流保護的電流變量
**     當電源檢測輸出電壓小於一定閾值，電流大於一定閾值，則判定輸出為短路；
**     當發生輸出短路後，電源關閉輸出，短路標誌位置位；
**     電源關機後等待*秒後重新啟動；
**     Parameters  : none
**     flag：運行模式 BUCK模式或者BOOST模式；
**     vValue：短路電壓判據 ***短路電壓急遽下降***
**     iValue：短路電流判據 ***短路電流急遽上升***
**     Returns     : none
** ===================================================================
*/

//需要注意的是，boost電路由於上管MOS Q1體二極管的存在，boost不具備短路保護功能
void ShortOff(uint8_t flag,int32_t vValue,int32_t iValue)
{
	static int32_t RSCnt = 0;//短路故障恢復等待的時間計數器
	static int32_t io=0,vo=0;//輸出電壓電流變量

	if(flag == BUCK)//當工作於BUCK模式時，Y端為輸出端，輸出電流為	SADC.Iy
	{
		io=SADC.Iy;
		vo=SADC.Vy;
	}
	else//否則工作於BOOST模式時，X端為輸入端，輸出電流為SADC.Ix
	{
		io=SADC.Ix;
		vo=SADC.Vx;
	}
/***需要注意的是，boost電路由於上管MOS Q1內部二極管的存在，boost不具備短路保護功能***/
/***需要注意的是，boost電路由於上管MOS Q1內部二極管的存在，boost不具備短路保護功能***/
/***需要注意的是，boost電路由於上管MOS Q1內部二極管的存在，boost不具備短路保護功能***/
	if((io> iValue)&&(vo <vValue))//當輸出電壓過小，電流過大時，則判定為輸出短路 同時誤判機會很小 不用加計時器
	{
		HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2);//禁止PWM輸出
		DF.PWMENFlag=0;//關閉PWM
		setRegBits(DF.ErrFlag,F_SW_SHORT);//DF.ErrFlag中對應故障類型的標誌位置1
		DF.SMFlag  =Err;//主狀態機跳轉至故障狀態-等待重新啟動
	}

	if(getRegBits(DF.ErrFlag,F_SW_SHORT))//當短路故障發生，電源關機，等待*秒，手動清除故障，進入等待狀態等待重啟
	{
		RSCnt++;//等待故障清楚計數器累加
		if(RSCnt>3000)//等待*秒
		{
			RSCnt=0;//時間計數器清零；一定要清零。
			clrRegBits(DF.ErrFlag,F_SW_SHORT);//清除DF.ErrFlag中對應故障類型的標誌位
		}
	}
}

/*
** ===================================================================
**     Funtion Name :int32_t CalTempDeg(int32_t val)
**     Description : 根據溫度採樣值(數字量)擬合出實際的溫度值
**     Parameters  :val 溫度數據量
**     Returns     : 實際溫度
** ===================================================================
*/
int32_t CalTempDeg(int32_t val)
{
	int32_t temp;//中間轉換變量
	double Te,a1,a2,a3,a4;//中間轉換量

	Te = (double) val ;//將採樣的數字強制轉換成浮點

	//按照函數擬合表擬合計算函數的各個量
	a1= 117.301974407480;
	a2= -0.093044462493*Te;
	a3= 0.000033457203*Te*Te;
	a4= -0.000000004974*Te*Te*Te;
	temp=(int32_t)(a1+a2+a3+a4);

	if(temp>100)//最大計算不超過100
		temp=100;
	if(temp<-40)//最大計算不超過-40
		temp=-40;
	return temp;
}

/*
** ===================================================================
**     Funtion Name :void DeviceOTP(void)
**     Description : 底板過溫保護
**     模塊採樣電源散熱器溫度，當環境溫度大於*攝氏度，則判定輸出為過溫；
**     當發生過溫，電源關閉輸出，過溫標誌位置位；
**     當發生過溫保護後，若底板溫度降至*攝氏度以下，自恢復重啟。
**     Parameters  : tValue 過溫保護點
**     Returns     : none
** ===================================================================
*/

void DeviceOTP(int32_t tValue)
{
	static int32_t DEGCnt;

	if(SADC.Deg > tValue ) //環境溫度過高
	{
		DEGCnt ++; //環溫異常計時器啟動
		if(DEGCnt > 200) //還溫異常計時器大於 * 毫秒，則進入保護模式
		{
			HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2);//禁止PWM輸出
			DF.PWMENFlag=0;//關閉PWM
			setRegBits(DF.ErrFlag,F_SW_OTP);//DF.ErrFlag中對應故障類型的標誌位置1
			DF.SMFlag  =Err;//主狀態機跳轉至故障狀態-等待重新啟動
		}
	}
	else DEGCnt = 0 ; //還境溫度回復，計時器清零

	if(getRegBits(DF.ErrFlag,F_SW_OTP))//當過溫保護發生，電源關機，等待散熱器溫度冷卻至*度，恢復啟動
	{
		if(SADC.Deg < (tValue-5)) //溫度恢復至過溫點5度一下
				clrRegBits(DF.ErrFlag,F_SW_OTP);//清除DF.ErrFlag中對應故障類型的標誌位
	}
}

/*
** ===================================================================
**     Funtion Name :   void ErrFlagPrintf(void)
**     Description :    打印故障信息,分別檢測DF.ErrFlag中的故障為，若有故障，則打印故障或保護信息
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void ErrFlagPrintf(void)
{

	if(getRegBits(DF.ErrFlag,F_SW_VIN_UVP))//打印輸入欠壓
		printf("errFlag = F_SW_VIN_UVP \n");
	if(getRegBits(DF.ErrFlag,F_SW_VIN_OVP))//打印輸入過壓
		printf("errFlag = F_SW_VIN_OVP \n");
	if(getRegBits(DF.ErrFlag,F_SW_VOUT_UVP))//打印輸出欠壓
		printf("errFlag = F_SW_VOUT_UVP \n");
	if(getRegBits(DF.ErrFlag,F_SW_VOUT_OVP))//打印輸出過壓
		printf("errFlag = F_SW_VOUT_OVP \n");
	if(getRegBits(DF.ErrFlag,F_SW_IOUT_OCP))//打印輸出過流
		printf("errFlag = F_SW_IOUT_OCP \n");
	if(getRegBits(DF.ErrFlag,F_SW_SHORT))//打印輸出短路
		printf("errFlag = F_SW_SHORT \n");
	if(getRegBits(DF.ErrFlag,F_SW_OTP))//打印散熱器過溫
		printf("errFlag = F_SW_OTP \n");
}



PUTCHAR_PROTOTYPE
{
	HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1 , 0xFFFF);
	return ch;
}
