/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_HRTIM_MspPostInit(HRTIM_HandleTypeDef *hhrtim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_G_Pin GPIO_PIN_13
#define LED_G_GPIO_Port GPIOC
#define LED_Y_Pin GPIO_PIN_14
#define LED_Y_GPIO_Port GPIOC
#define LED_R_Pin GPIO_PIN_15
#define LED_R_GPIO_Port GPIOC
#define ADC1_VX_Pin GPIO_PIN_0
#define ADC1_VX_GPIO_Port GPIOA
#define ADC1_IX_Pin GPIO_PIN_1
#define ADC1_IX_GPIO_Port GPIOA
#define ADC1_VY_Pin GPIO_PIN_2
#define ADC1_VY_GPIO_Port GPIOA
#define ADC1_IY_Pin GPIO_PIN_3
#define ADC1_IY_GPIO_Port GPIOA
#define ADC2_ADJV_Pin GPIO_PIN_6
#define ADC2_ADJV_GPIO_Port GPIOA
#define ADC2_ADJI_Pin GPIO_PIN_7
#define ADC2_ADJI_GPIO_Port GPIOA
#define ADC1_IL_Pin GPIO_PIN_0
#define ADC1_IL_GPIO_Port GPIOB
#define ADC2_TEMP_Pin GPIO_PIN_2
#define ADC2_TEMP_GPIO_Port GPIOB
#define TFT_DC_Pin GPIO_PIN_12
#define TFT_DC_GPIO_Port GPIOB
#define KEY1_Pin GPIO_PIN_13
#define KEY1_GPIO_Port GPIOB
#define KEY2_Pin GPIO_PIN_14
#define KEY2_GPIO_Port GPIOB
#define TFT_RES_Pin GPIO_PIN_15
#define TFT_RES_GPIO_Port GPIOB
#define TEST_IO_Pin GPIO_PIN_11
#define TEST_IO_GPIO_Port GPIOA
#define TFT_CLK_Pin GPIO_PIN_3
#define TFT_CLK_GPIO_Port GPIOB
#define KEY_SW_Pin GPIO_PIN_4
#define KEY_SW_GPIO_Port GPIOB
#define TFT_SDA_Pin GPIO_PIN_5
#define TFT_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
