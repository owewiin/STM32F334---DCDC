/*
 * RS232.h
 *
 *  Created on: Jan 7, 2025
 *      Author: mshome
 */

#ifndef INC_RS232_H_
#define INC_RS232_H_

#include "stm32f3xx_hal.h"
#include "stdio.h"

#define MAX_BYTE_NUM 6  //緩衝最大字節數
extern uint8_t UARTBuf[MAX_BYTE_NUM]; //數據緩存 必須定義在 #define MAX_.....下面
#define NEW_FLAME 1 	//新的數據幀
#define OLD_FLAME 0		//舊的數據幀

//通訊故障類型 errType
#define ERR_NONE 0		//無故障
#define ERR_FUN  1		//功能碼錯誤
#define ERR_CCR  2		//校驗錯誤
#define ERR_DATA 3		//數據超出範圍錯誤

//狀態機枚舉
typedef enum
{
	RS232Init, //初始化
	RS232Idle, //等待接收
	RS232ReqCheck, //幀檢測校驗
	RS232ReqProcess, //幀內容處理
	RS232acK, //回覆
	RS232Err //訊息錯誤S
} RS232_STATE_MACHINE_TYPE ;

//接收一幀數據的相關結構體
typedef struct _rs232_rx_frame
{
	int8_t 	RxNum;			//接收字節個數
	int8_t 	RxFunctionCode; //功能碼
	int16_t RxValue;		//接收數值
	int16_t RxCheckValue;	//校驗值
} RS232_RX_FRAME;

//發送一幀數據的相關結構體
typedef struct _rs232_tx_frame
{
	int8_t 	TxNum;			//發送字節個數
	int8_t 	TxFunctionCode;	//功能碼
	int16_t TxValue;		//接收數值
	int16_t TxCheckValue;	//校驗值
} RS232_TX_FRAME;

//通訊相關結構體
typedef struct _modbus_par
{
	uint8_t MsgBuff[MAX_BYTE_NUM];	//數據緩存
	int8_t 	errType;				//通訊錯誤內容
	int8_t	NewFlame;				//新的幀標誌表
} RS232_REG;

//數據命令功能碼
typedef struct _msy_type
{
	//數據命令功能碼
    __IO int16_t OutEn;//輸出使能
    __IO int16_t vRef;//電壓參考值0.01精度
    __IO int16_t iRef;//電流參考值0.01精度
	__IO int16_t RxReserve1;//設置保留-用戶自定義
	__IO int16_t RxReserve2;//設置保留-用戶自定義
	__IO int16_t RxReserve3;//設置保留-用戶自定義
	__IO int16_t RxReserve4;//設置保留-用戶自定義
	__IO int16_t RxReserve5;//設置保留-用戶自定義

	//對應變量
    __IO int16_t Vout;//輸出電壓
    __IO int16_t Iout;//輸出電流
    __IO int16_t ModuleState;//電源狀態
    __IO int16_t ErrType;//電源故障類型
	__IO int16_t Mode;//BUCK模式還是BOOST模式
	__IO int16_t ccvMode;//CC模式還是CV模式
	__IO int16_t TxReserve1;//上報保留-用戶自定義
	__IO int16_t TxReserve2;//上報保留-用戶自定義
}RS232_DATA;

//寫命令功能碼定義 (對應數據命令功能碼)
#define  SET_OUTPUT_EN      1
#define  SET_OUTPUT_VREF    2
#define  SET_OUTPUT_IREF  	3
#define  SET_RESERVE1       4
#define  SET_RESERVE2       5
#define  SET_RESERVE3       6
#define  SET_RESERVE4       7
#define  SET_RESERVE5       8

//上報功能碼定義 (對應變量)
#define  READ_VOUT      	9
#define  READ_IOUT			10
#define  READ_MSTATE		11
#define  READ_ERR_TYPE		12
#define  READ_MODE			13
#define  READ_CCVMODE		14
#define  READ_RESERVE1		15
#define  READ_RESERVE2		16

void RS232StateMachine(void);
void RS232MInit(void);
void RS232MIdle(void);
void RS232MReqCheck(void);
void RS232MReqProcess(void);
void RS232Mack(void);
void RS232MErr(void);
void DataRecord(void);

#endif /* INC_RS232_H_ */
