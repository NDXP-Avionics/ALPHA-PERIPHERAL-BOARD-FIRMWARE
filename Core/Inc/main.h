/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED1_Pin GPIO_PIN_13
#define LED1_GPIO_Port GPIOC
#define LED2_Pin GPIO_PIN_14
#define LED2_GPIO_Port GPIOC
#define LED3_Pin GPIO_PIN_15
#define LED3_GPIO_Port GPIOC
#define K1_Pin GPIO_PIN_0
#define K1_GPIO_Port GPIOD
#define TC4_CS_Pin GPIO_PIN_0
#define TC4_CS_GPIO_Port GPIOA
#define TC3_CS_Pin GPIO_PIN_1
#define TC3_CS_GPIO_Port GPIOA
#define DIR_RS485_Pin GPIO_PIN_4
#define DIR_RS485_GPIO_Port GPIOA
#define TC1_CS_Pin GPIO_PIN_0
#define TC1_CS_GPIO_Port GPIOB
#define TC2_CS_Pin GPIO_PIN_1
#define TC2_CS_GPIO_Port GPIOB
#define BW1_Pin GPIO_PIN_2
#define BW1_GPIO_Port GPIOB
#define S1_Pin GPIO_PIN_12
#define S1_GPIO_Port GPIOB
#define S2_Pin GPIO_PIN_13
#define S2_GPIO_Port GPIOB
#define S3_Pin GPIO_PIN_14
#define S3_GPIO_Port GPIOB
#define S4_Pin GPIO_PIN_15
#define S4_GPIO_Port GPIOB
#define PYRO1_Pin GPIO_PIN_8
#define PYRO1_GPIO_Port GPIOA
#define LC_DRDY_DOUT_Pin GPIO_PIN_11
#define LC_DRDY_DOUT_GPIO_Port GPIOA
#define LC_SCLK_Pin GPIO_PIN_12
#define LC_SCLK_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
