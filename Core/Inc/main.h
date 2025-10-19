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
// 灯光状态枚举
typedef enum {
  LIGHT_OFF = 0,
  LIGHT_DIM,      // 微光模式
  LIGHT_BRIGHT    // 高亮模式
} LightStateTypeDef;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
#define SHORT_PRESS_MIN_MS    500
#define SHORT_PRESS_MAX_MS    2000
#define LONG_PRESS_MS         2000
#define DIM_TIMEOUT_MS        60000  // 1分钟 = 60,000毫秒
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void EnterStandbyMode(void);
void SetLightDim(void);
void SetLightBright(void);
void TurnOffLight(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BIG_BTN_Pin GPIO_PIN_0
#define BIG_BTN_GPIO_Port GPIOA
#define BIG_BTN_EXTI_IRQn EXTI0_1_IRQn
#define LEDA_Pin GPIO_PIN_4
#define LEDA_GPIO_Port GPIOA
#define BEEP_Pin GPIO_PIN_6
#define BEEP_GPIO_Port GPIOA
#define LEDB_Pin GPIO_PIN_7
#define LEDB_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */