/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f0xx_hal.h"

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
#define LED0_Pin GPIO_PIN_13
#define LED0_GPIO_Port GPIOC
#define LED1_Pin GPIO_PIN_14
#define LED1_GPIO_Port GPIOC
#define B0_Pin GPIO_PIN_0
#define B0_GPIO_Port GPIOA
#define B1_Pin GPIO_PIN_1
#define B1_GPIO_Port GPIOA
#define B2_Pin GPIO_PIN_2
#define B2_GPIO_Port GPIOA
#define B3_Pin GPIO_PIN_3
#define B3_GPIO_Port GPIOA
#define B4_Pin GPIO_PIN_4
#define B4_GPIO_Port GPIOA
#define B5_Pin GPIO_PIN_5
#define B5_GPIO_Port GPIOA
#define B6_Pin GPIO_PIN_6
#define B6_GPIO_Port GPIOA
#define B7_Pin GPIO_PIN_7
#define B7_GPIO_Port GPIOA
#define B8_Pin GPIO_PIN_0
#define B8_GPIO_Port GPIOB
#define B9_Pin GPIO_PIN_1
#define B9_GPIO_Port GPIOB
#define B10_Pin GPIO_PIN_2
#define B10_GPIO_Port GPIOB
#define B11_Pin GPIO_PIN_10
#define B11_GPIO_Port GPIOB
#define L0_Pin GPIO_PIN_12
#define L0_GPIO_Port GPIOB
#define L1_Pin GPIO_PIN_13
#define L1_GPIO_Port GPIOB
#define L2_Pin GPIO_PIN_14
#define L2_GPIO_Port GPIOB
#define L3_Pin GPIO_PIN_15
#define L3_GPIO_Port GPIOB
#define L4_Pin GPIO_PIN_8
#define L4_GPIO_Port GPIOA
#define L5_Pin GPIO_PIN_9
#define L5_GPIO_Port GPIOA
#define L6_Pin GPIO_PIN_10
#define L6_GPIO_Port GPIOA
#define L7_Pin GPIO_PIN_15
#define L7_GPIO_Port GPIOA
#define L8_Pin GPIO_PIN_3
#define L8_GPIO_Port GPIOB
#define L9_Pin GPIO_PIN_4
#define L9_GPIO_Port GPIOB
#define L10_Pin GPIO_PIN_5
#define L10_GPIO_Port GPIOB
#define L11_Pin GPIO_PIN_6
#define L11_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
