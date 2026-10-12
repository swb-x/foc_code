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
#include "stm32g4xx_hal.h"

#include "stm32g4xx_ll_adc.h"
#include "stm32g4xx_ll_dma.h"
#include "stm32g4xx_ll_i2c.h"
#include "stm32g4xx_ll_rcc.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_crs.h"
#include "stm32g4xx_ll_system.h"
#include "stm32g4xx_ll_exti.h"
#include "stm32g4xx_ll_cortex.h"
#include "stm32g4xx_ll_utils.h"
#include "stm32g4xx_ll_pwr.h"
#include "stm32g4xx_ll_tim.h"
#include "stm32g4xx_ll_usart.h"
#include "stm32g4xx_ll_gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdint.h"
#include "app_adc.h"
#include "cmsis_os2.h"
#include "math.h"
#include "datetypes.h"
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
#define SOA_Pin LL_GPIO_PIN_0
#define SOA_GPIO_Port GPIOA
#define SOB_Pin LL_GPIO_PIN_1
#define SOB_GPIO_Port GPIOA
#define SOC_Pin LL_GPIO_PIN_2
#define SOC_GPIO_Port GPIOA
#define VCC_ADIN_Pin LL_GPIO_PIN_3
#define VCC_ADIN_GPIO_Port GPIOA
#define TEMP_Pin LL_GPIO_PIN_4
#define TEMP_GPIO_Port GPIOA
#define nFault_Pin LL_GPIO_PIN_5
#define nFault_GPIO_Port GPIOA
#define nFault_EXTI_IRQn EXTI9_5_IRQn
#define ENABLE_Pin LL_GPIO_PIN_6
#define ENABLE_GPIO_Port GPIOA
#define LED_CTRL_Pin LL_GPIO_PIN_10
#define LED_CTRL_GPIO_Port GPIOB
#define CAL_Pin LL_GPIO_PIN_11
#define CAL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
