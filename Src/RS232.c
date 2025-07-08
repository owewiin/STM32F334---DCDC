/*
 * RS232.c
 *
 *  Created on: Jan 7, 2025
 *      Author: mshome
 */

#include <RS232.h>
#include "function.h"

extern UART_HandleTypeDef huart3;


RS232_STATE_MACHINE_TYPE RS232State=RS232Init; 					//通訊狀態機聲明
RS232_RX_FRAME RxFrame    = {0,0,0,0}; 		  					//接收幀聲明
RS232_TX_FRAME TxFrame 	  = {0,0,0,0}; 		   					//發送幀聲明
RS232_REG      RS232_Reg  = {{0,0,0,0,0,0},0,0};  				//通訊變量聲明
RS232_DATA	   RS232data  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};	//通訊數據內容
uint8_t UARTBuf[MAX_BYTE_NUM]={0};								//數據緩存

/*
** ===================================================================
**     Funtion Name :void RS232StateMachine(void)
**     Description :
**     通信狀態機：初始化/等待接收/幀檢測完好/幀信息處理/上報信息/通信故障
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void RS232StateMachine(void)
{
	if(DF.CtlMode ==CTL_MODE_MONITOR)//只有在上位機運行的模式下才處理通信協議內容
	{
		switch(RS232State)//判斷通信狀態
		{
			case  RS232Init:RS232MInit();				//初始化
			break;
			case  RS232Idle:RS232MIdle();				//等待接收，接收後清零訊號等待下一波
			break;
			case  RS232ReqCheck:RS232MReqCheck();		//幀檢測完好
			break;
			case  RS232ReqProcess:RS232MReqProcess();	//幀信息處理
			break;
			case  RS232acK:RS232Mack();					//返回信息
			break;
			case  RS232Err:RS232MErr();					//通信故障處理
			break;
			default:
			break;
		}
		DataRecord();//數據上報
	}
}
/*
** ===================================================================
**     Funtion Name :void RS232MInit(void)
**     Description :通信初始化
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void RS232MInit(void)
{
	for(int16_t i=0;i<MAX_BYTE_NUM;i++)
		RS232_Reg.MsgBuff[i]=0;//初始化接收緩衝器

	RS232_Reg.errType=ERR_NONE;//初始化通信故障標誌位

	RS232data.OutEn=0;//初始化開關機
	RS232data.vRef=500;//初始化電壓參考
	RS232data.iRef=500;//初始化電流參考
	RS232data.RxReserve1=0;//設置保留-用戶自定義
	RS232data.RxReserve2=0;//設置保留-用戶自定義
	RS232data.RxReserve3=0;//設置保留-用戶自定義
	RS232data.RxReserve4=0;//設置保留-用戶自定義
	RS232data.RxReserve5=0;//設置保留-用戶自定義
	RS232data.Vout=0;//輸出電壓
	RS232data.Iout=0;//輸出電流
	RS232data.ModuleState=0;//電源狀態
	RS232data.ErrType=0;//電源故障類型
	RS232data.Mode=0;//BUCK/BOOST模式
	RS232data.ccvMode=0;//CC/CV模式
	RS232data.TxReserve1=0;//上報保留-用戶自定義
	RS232data.TxReserve2=0;//上報保留-用戶自定義

	RS232State = RS232Idle;	//跳轉等待狀態
}
/*
** ===================================================================
**     Funtion Name :void RS232MIdle(void)
**     Description :等待接收新幀
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void RS232MIdle(void)
{
	RS232_Reg.errType = ERR_NONE;//服務通信故障標誌位

	if(RS232_Reg.NewFlame == 1)//當接收一幀結束，則跳轉幀檢測完好狀態機
	{
		RS232State = RS232ReqCheck;//跳轉幀檢測完好狀態機
		RS232_Reg.NewFlame = 0;//復位標誌位，等待下一幀到來
	}
}
/*
** ===================================================================
**     Funtion Name :void RS232MReqCheck(void)
**     Description :幀檢測是否正常
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void RS232MReqCheck(void)
{
	int32_t RxCheckValueTemp = 0;
	//將MsgBuff緩衝器中的數據轉存到RxFrame接收幀結構體中
	RxFrame.RxFunctionCode=(RS232_Reg.MsgBuff[1]<<8)|RS232_Reg.MsgBuff[0];//保存功能碼
	RxFrame.RxValue=(RS232_Reg.MsgBuff[3]<<8)|RS232_Reg.MsgBuff[2];//保存幀數據
	RxFrame.RxCheckValue=(RS232_Reg.MsgBuff[5]<<8)|RS232_Reg.MsgBuff[4];//保存幀校驗和

	for(int16_t i=0;i<4;i++)//計算一幀數據中前四個字節的和
		RxCheckValueTemp = RxCheckValueTemp + RS232_Reg.MsgBuff[i];
	RxCheckValueTemp = RxCheckValueTemp&0xFFFF;//取低四位

	if(RxFrame.RxCheckValue == RxCheckValueTemp)//驗證接收數據的效驗值和計算值是否相同，相同則數據正確，不相同著數據有問題
	{
		if(RxFrame.RxFunctionCode< 17)//判斷功能碼是否在定義範圍內，定義的最大功能碼為16
		{
			RS232State = RS232ReqProcess;//數據正常跳轉幀信息處理
		}
		else
		{
			RS232_Reg.errType = ERR_FUN;//功能碼錯誤
			RS232State = RS232Err;//通信跳轉故障狀態
		}
	}
	else//數據接收錯誤
	{
		RS232_Reg.errType = ERR_CCR;//校驗和錯誤
		RS232State = RS232Err;//通信跳轉故障狀態
	}
}
/*
** ===================================================================
**     Funtion Name :void RS232MReqProcess(void)
**     Description :幀信息處理
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void RS232MReqProcess(void)
{
	switch(RxFrame.RxFunctionCode)//根據接收到的功能碼不同處理不同
	{
		case SET_OUTPUT_EN://控制輸出開關機
		{
			if(RxFrame.RxValue<2)//判斷數據合理性，開關機只有0和1兩種
				RS232data.OutEn = RxFrame.RxValue;//將接收幀數據轉存對應變量
			else//數據不合理
				RS232_Reg.errType = ERR_DATA;//錯誤內容數據不合理
			break;
		}
		case SET_OUTPUT_VREF://設置輸出目標電壓
		{
			if(RxFrame.RxValue<5001)//判斷數據合理性，最大目標電壓50V
				RS232data.vRef = RxFrame.RxValue;//將接收幀數據轉存對應變量
			else//數據不合理
				RS232_Reg.errType = ERR_DATA;//錯誤內容數據不合理
			break;
		}
		case SET_OUTPUT_IREF://設置輸出目標電流
		{
			if(RxFrame.RxValue<1001)//判斷數據合理性，最大目標電流10A
				RS232data.iRef = RxFrame.RxValue;//將接收幀數據轉存對應變量
			else//數據不合理
				RS232_Reg.errType = ERR_DATA;//錯誤內容數據不合理
			break;
		}
		case SET_RESERVE1://設置保留-用戶自定義
		{
			RS232data.RxReserve1=RxFrame.RxValue;//預留調試數據1，用戶自定義用
			break;
		}
		case SET_RESERVE2://設置保留-用戶自定義
		{
			RS232data.RxReserve2=RxFrame.RxValue;//預留調試數據2，用戶自定義用
			break;
		}
		case SET_RESERVE3://設置保留-用戶自定義
		break;
		case SET_RESERVE4://設置保留-用戶自定義
		break;
		case SET_RESERVE5://設置保留-用戶自定義
		break;
		default:
		break;
	}

	if(RS232_Reg.errType==ERR_NONE)//信息處理沒有任何問題
		RS232State = RS232Idle;//跳轉待機狀態等待下一幀數據接收
	else
		RS232State = RS232Err;//通信跳轉故障狀態
}
/*
** ===================================================================
**     Funtion Name :void RS232Mack(void)
**     Description :返回信息
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void RS232Mack(void)
{

}
/*
** ===================================================================
**     Funtion Name :void RS232MErr(void)
**     Description :通信故障（隔一陣時間後清除，跳轉等待轉態重新接收）
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void RS232MErr(void)
{
	int32_t ErrCnt = 0;//計時器

	ErrCnt++;//計時器累加
	if(ErrCnt>4)
	{
		ErrCnt=0;
		RS232_Reg.errType=ERR_NONE;//清故障類型標誌位
		RS232State = RS232Idle;//跳轉待機狀態等待下一幀數據接收
	}
}

/*
** ===================================================================
**     Funtion Name :void DataRecord(void)
**     Description :模塊信息上報函數，固定間隔一段時間上報模塊相關的信息
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void DataRecord(void)
{
	static int32_t RS2323RecordTimeCnt = 0;//計時器
	static int32_t RecordFlag=READ_VOUT;//發送幀的類型
	int32_t temp=0;//數據中間變量
	uint8_t txdata[6]={0};//發送輸出-一幀數據6個字節
	int32_t TxCheckValueTemp=0;//校驗和

	RS2323RecordTimeCnt++;//計時器累加

	if(RS2323RecordTimeCnt>40)//隔固定時間發送一幀數據
	{
		RS2323RecordTimeCnt = 0;//復位計數器，判斷發送什麼數據

		switch(RecordFlag)//判斷當前上報的數據類型
		{
			case READ_VOUT://上報輸出電壓
			{
				TxFrame.TxFunctionCode = READ_VOUT;//賦值功能碼
				if(DF.BBFlag == BUCK)//BUCK模式控制輸出為Y端，上報Y端電壓
					temp = SADC.VyAvg*6800>>12;//將y端電壓數字量轉換成實際值，並擴大100倍
				else//BOOST模式控制輸出為Y端，上報X端電壓
					temp = SADC.VxAvg*6800>>12;//將x端電壓數字量轉換成實際值，並擴大100倍

				TxFrame.TxValue = temp&0xFFFF;//賦值發送幀中的TxValue

				RecordFlag = READ_IOUT;//跳轉下一個上報狀態
				break;
			}
			case READ_IOUT://上報輸出電流
			{
				TxFrame.TxFunctionCode = READ_IOUT;//賦值功能碼

				if(DF.BBFlag == BUCK)//BUCK模式控制輸出為Y端，上報Y端電流
					temp = SADC.IyAvg*3300>>12;//將y端電流數字量轉換成實際值，並擴大100倍
				else//BOOST模式控制輸出為Y端，上報X端電流
					temp = SADC.IxAvg*3300>>12;//將x端電流數字量轉換成實際值，並擴大100倍

				TxFrame.TxValue = temp&0xFFFF;//賦值發送幀中的TxValue

				RecordFlag = READ_MSTATE;//跳轉下一個上報狀態
				break;
			}
			case READ_MSTATE://上報模塊狀態
			{
				TxFrame.TxFunctionCode = READ_MSTATE;//賦值功能碼
				TxFrame.TxValue = DF.SMFlag;//賦值發送幀中的TxValue
				RecordFlag = READ_ERR_TYPE;//跳轉下一個上報狀態
				break;
			}
			case READ_ERR_TYPE://上報故障代碼
			{
				TxFrame.TxFunctionCode = READ_ERR_TYPE;//賦值功能碼
				TxFrame.TxValue = DF.ErrFlag;//賦值發送幀中的TxValue
				RecordFlag = READ_MODE;//跳轉下一個上報狀態
				break;
			}
			case READ_MODE://上報運行模式，BUCK模式還是BOOST模式
			{
				TxFrame.TxFunctionCode = READ_MODE;//賦值功能碼
				TxFrame.TxValue = DF.BBFlag;//賦值發送幀中的TxValue
				RecordFlag = READ_CCVMODE;//跳轉下一個上報狀態
				break;
			}
			case READ_CCVMODE://上報控制模式，輸出恆壓狀態還是恆流狀態
			{
				TxFrame.TxFunctionCode = READ_CCVMODE;//賦值功能碼
				TxFrame.TxValue = DF.cvMode;//賦值發送幀中的TxValue
				RecordFlag = READ_RESERVE1;//跳轉下一個上報狀態
				break;
			}
			case READ_RESERVE1://上報保留-用戶自定義
			{
				TxFrame.TxFunctionCode=READ_RESERVE1;//賦值功能碼
				TxFrame.TxValue = 0;//預留數據，用戶自定義
				RecordFlag = READ_RESERVE2;//跳轉下一個上報狀態
				break;
			}
			case READ_RESERVE2://上報保留-用戶自定義
			{
				TxFrame.TxFunctionCode=READ_RESERVE2;//賦值功能碼
				TxFrame.TxValue = 0;//預留數據，用戶自定義
				RecordFlag = READ_VOUT;//跳轉下一個上報狀態
				break;
			}
		}

		//賦值發送的數組
		txdata[0] = TxFrame.TxFunctionCode&0x00FF;//賦值發送的數組
		txdata[1] = (TxFrame.TxFunctionCode>>8)&0x00FF;//賦值發送的數組
		txdata[2] = TxFrame.TxValue&0x00FF;//賦值發送的數組
		txdata[3] = (TxFrame.TxValue>>8)&0x00FF;//賦值發送的數組

		for(int16_t i=0;i<4;i++)//計算一幀數據中前四個字節的和
			TxCheckValueTemp = TxCheckValueTemp + txdata[i];
		TxFrame.TxCheckValue = TxCheckValueTemp&0xFFFF;//取低四位 賦值校驗和

		txdata[4] = TxFrame.TxCheckValue&0x00FF;//賦值發送的數組
		txdata[5] = (TxFrame.TxCheckValue>>8)&0x00FF;//賦值發送的數組

		/*
		txdata[0]=0x0A;
		txdata[1]=0x00;
		txdata[2]=0xa4;
		txdata[3]=0x0d;
		txdata[4]=0xBB;
		txdata[5]=0x00;
		*/

		HAL_UART_Transmit(&huart3, txdata, MAX_BYTE_NUM, 0x00ff);//調用函數發送數據
	}
}

/*
** ===================================================================
**     Funtion Name :   void HAL_UART_RxCpltCallback(void)
**     Description :    UART中斷接收回調函數
**     Parameters  :
**     Returns     :
** ===================================================================
*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	RS232_Reg.NewFlame = 1;//接收完一幀數據標誌位置位

	for(uint8_t i=0;i<MAX_BYTE_NUM;i++)//將數據從接收寄存器中轉存至RS232_Reg.MsgBuff中
		RS232_Reg.MsgBuff[i] = UARTBuf[i];
}

