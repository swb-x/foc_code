/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32g4xx_ll_gpio.h"
#include "mcpwm_foc.h"
#include "app_adc.h"
void StartCtrlLedTask(void *argument);
void StartMotorTask(void *argument);
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
osThreadId_t LedCtrlTaskHandle;
const osThreadAttr_t LedCtrlTask_attributes = {
  .name = "LedCtrlTask",
  .priority = (osPriority_t) osPriorityNormal,
  /* printf + RTOS mutex call inside fputc needs >512B stack */
  .stack_size = 256 * 4
};
osThreadId_t TempDetectTaskHandle;
const osThreadAttr_t TempDetectTask_attributes = {
  .name = "TempDetectTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 256 * 4
};
osThreadId_t MotorTaskHandle;
const osThreadAttr_t MotorTask_attributes = {
  .name = "MotorTask",
  .priority = (osPriority_t) osPriorityAboveNormal,
  .stack_size = 256 * 4
};
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  /* Print mutex used by fputc (see usart.c). Must be created after osKernelInitialize. */
  {
    extern osMutexId_t uart_print_mutex;
    uart_print_mutex = osMutexNew(NULL);
  }
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  LedCtrlTaskHandle = osThreadNew(StartCtrlLedTask, NULL, &LedCtrlTask_attributes);
  TempDetectTaskHandle = osThreadNew(StartTempDetectTask, NULL, &TempDetectTask_attributes);
  MotorTaskHandle = osThreadNew(StartMotorTask, NULL, &MotorTask_attributes);
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void StartCtrlLedTask(void *argument)
{
  uint32_t last_fault = 0;

  printf("LedCtrlTask started\n");

  for(;;)
  {
    /* nFAULT tripped: output stays off until Motor_PWM_Start(); print so it is visible */
    if (g_drv_fault_count != last_fault)
    {
      last_fault = g_drv_fault_count;
      printf("DRV8353 FAULT: nFAULT low, all PWM disabled (cnt=%lu)\r\n",
             (unsigned long)last_fault);
    }

    LL_GPIO_TogglePin(LED_CTRL_GPIO_Port, LED_CTRL_Pin);
    osDelay(500);
  }
}

/*==========================================================================
 * MotorTask：FOC 初始化 + 1kHz 速度环 + 遥测打印
 * 上电只做初始化和校准，不自动转动（mode 保持 IDLE）。
 *==========================================================================*/
void StartMotorTask(void *argument)
{
  uint32_t print_div = 0;

  printf("MotorTask started\r\n");

  /* 1. ADC1 校准 + DMA + TIM1 触发链启动 */
  FOC_Init();

  /* 2. DRV8353 电流失调校准（MOE=0，功率级无输出；要求电机零电流） */
  MC_CalibrateOffsets();

  for(;;)
  {
    /* 3. 速度环（IDLE / OPENLOOP 模式下内部直接返回） */
    FOC_SpeedLoop();

    /* 4. 每 500ms 打印一帧遥测，用来确认采样链路和角度收敛情况 */
    if (++print_div >= 500)
    {
      print_div = 0;
      printf("raw=%u,%u,%u,%u I=%.2f,%.2f,%.2f Vbus=%.1f id=%.2f iq=%.2f spd=%.0frpm\r\n",
             g_adc_raw[0], g_adc_raw[1], g_adc_raw[2], g_adc_raw[3],
             g_mc_state.iu, g_mc_state.iv, g_mc_state.iw,
             g_mc_state.vbus,
             g_mc_state.id, g_mc_state.iq,
             g_mc_state.pll_speed * 60.0f / 6.2831853f / (float)g_mc_cfg.pole_pairs);
    }

    osDelay(1);
  }
}
/* USER CODE END Application */

