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
#define HIN_U_Pin GPIO_PIN_0
#define HIN_U_GPIO_Port GPIOA
#define HIN_V_Pin GPIO_PIN_1
#define HIN_V_GPIO_Port GPIOA
#define HIN_W_Pin GPIO_PIN_2
#define HIN_W_GPIO_Port GPIOA
#define LIN_U_Pin GPIO_PIN_3
#define LIN_U_GPIO_Port GPIOA
#define LIN_V_Pin GPIO_PIN_4
#define LIN_V_GPIO_Port GPIOA
#define LIN_W_Pin GPIO_PIN_5
#define LIN_W_GPIO_Port GPIOA
#define UOUT_Pin GPIO_PIN_10
#define UOUT_GPIO_Port GPIOB
#define UOUT_EXTI_IRQn EXTI15_10_IRQn
#define VOUT_Pin GPIO_PIN_11
#define VOUT_GPIO_Port GPIOB
#define VOUT_EXTI_IRQn EXTI15_10_IRQn
#define WOUT_Pin GPIO_PIN_12
#define WOUT_GPIO_Port GPIOB
#define WOUT_EXTI_IRQn EXTI15_10_IRQn
#define SCL_Pin GPIO_PIN_8
#define SCL_GPIO_Port GPIOB
#define SDA_Pin GPIO_PIN_9
#define SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
