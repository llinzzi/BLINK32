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
#include "stm32g0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef enum {
  LED_OFF = 0,      // 熄灭状态
  LED_DIM,          // 微亮状态
  LED_BRIGHT,       // 高亮状态
  LED_ALARM_FLASH,  // 闹铃闪烁状态
  LED_ALARM_ON      // 闹铃常亮状态
} LED_StateTypeDef;

typedef enum {
  SYSTEM_NORMAL = 0,  // 正常模式
  SYSTEM_LOW_POWER    // 低功耗模式
} System_StateTypeDef;
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
#define BIG_BTN_Pin GPIO_PIN_0
#define BIG_BTN_GPIO_Port GPIOA
#define BIG_BTN_EXTI_IRQn EXTI0_1_IRQn
#define LEDA_Pin GPIO_PIN_4
#define LEDA_GPIO_Port GPIOA
#define BEEP_Pin GPIO_PIN_6
#define BEEP_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */
#define PWM_MAX_VALUE         1600
#define PWM_DIM_VALUE         50    // 微亮PWM值
#define PWM_BRIGHT_VALUE      1000   // 高亮PWM值
#define PWM_STEP_SIZE         16
#define DEBOUNCE_DELAY        50
#define LONG_PRESS_TIME       1000   // 长按1秒
#define BEEP_DURATION         30000  // 闹铃持续30秒
#define ALARM_INITIAL_PERIOD  2000   // 初始闪烁周期2秒
#define ALARM_MIN_PERIOD      200    // 最小闪烁周期200ms
#define ALARM_PERIOD_STEP     10     // 周期递减步长
#define BEEP_PWM_PERIOD       987    // TIM16周期值
#define ALARM_DELAY_TIME      1800000  // 30分钟延迟时间 (30*60*1000)
#define ALARM_BEEP_DURATION   30000    // 闹钟蜂鸣持续时间30秒
#define PWM_TRANSITION_STEP   10       // PWM渐变步长
#define PWM_TRANSITION_DELAY  10       // PWM渐变延迟(ms)
#define BEEP_ENABLE           0        // 蜂鸣器开关量 1-开启 0-关闭
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */