
#include "stm32f3xx_hal.h"

#define RED  	0xf800//紅色
#define GREEN	0x07e0//綠色
#define BLUE 	0x001f//藍色
#define WHITE	0xffff//白色
#define BLACK	0x0000//黑色
#define YELLOW  0xFFE0//黃色
#define GRAY0   0xEF7D//灰色0
#define GRAY1   0x8410//灰色1
#define GRAY2   0x4208//灰色2

#define LCD_CTRLB   	GPIOB//定義TFT數據端口B口

#define LCD_SCL       GPIO_PIN_3//B3-時鐘
#define LCD_SDA       GPIO_PIN_5//B5-數據
#define LCD_RST       GPIO_PIN_15//B15-復位
#define LCD_RS        GPIO_PIN_12//B12-數據命令選擇

#define	LCD_SDA_SET  	LCD_CTRLB->BSRR=LCD_SDA//數據高
#define	LCD_SDA_CLR  	LCD_CTRLB->BRR=LCD_SDA//數據低
#define	LCD_SCL_SET  	LCD_CTRLB->BSRR=LCD_SCL//時鐘高
#define	LCD_SCL_CLR  	LCD_CTRLB->BRR=LCD_SCL //時鐘低
#define	LCD_RST_SET  	LCD_CTRLB->BSRR=LCD_RST//復位高
#define	LCD_RST_CLR  	LCD_CTRLB->BRR=LCD_RST//復位低
#define	LCD_RS_SET  	LCD_CTRLB->BSRR=LCD_RS //數據命令位高
#define	LCD_RS_CLR  	LCD_CTRLB->BRR=LCD_RS //數據命令位低

#define X_MAX_PIXEL 240//屏幕為128*128點
#define Y_MAX_PIXEL	240

//函數聲明
void SPI_WriteData(uint8_t Data);
void Lcd_WriteIndex(uint8_t Index);
void Lcd_WriteData(uint8_t Data);
void LCD_WriteData_16Bit(uint16_t Data);
void Lcd_WriteReg(uint8_t Index,uint8_t Data);
void Lcd_Reset(void);
void Lcd_Init(void);
void Lcd_SetRegion(uint16_t x_start,uint16_t y_start,uint16_t x_end,uint16_t y_end);
void Lcd_SetXY(uint16_t x,uint16_t y);
void Gui_DrawPoint(uint16_t x,uint16_t y,uint16_t Data);
unsigned int Lcd_ReadPoint(uint16_t x,uint16_t y);
void Lcd_Clear(uint16_t Color);
void LCDshowDate(uint16_t x,uint16_t y,uint16_t fc,uint16_t bc,uint16_t add);
void LCDshowChar(uint16_t x,uint16_t y,uint16_t fc,uint16_t bc,uint16_t add);
void LCDshowDot(uint16_t x,uint16_t y,uint16_t fc,uint16_t bc,uint16_t add);
void LCDShowNum(uint16_t x,uint16_t y,uint16_t fc,uint16_t bc,uint16_t num);
void LCDShowFnum(uint16_t x,uint16_t y,uint16_t fc,uint16_t bc,uint16_t data);

