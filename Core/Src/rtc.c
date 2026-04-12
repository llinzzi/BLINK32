/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
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
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  
  // 根据备份标志决定是否设置时间和日期
  uint32_t rtcConfigured = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0);

  /** Initialize RTC and set the Time and Date
  */
  // 只有首次配置或备份域丢失时才设置时间和日期
  if (rtcConfigured != 0x32F1) {
    sTime.Hours = 0x7;
    sTime.Minutes = 0x00;
    sTime.Seconds = 0x30;
    sTime.SubSeconds = 0x0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
    {
      Error_Handler();
    }
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;
    sDate.Month = RTC_MONTH_JANUARY;
    sDate.Date = 0x1;
    sDate.Year = 0x25;

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
    {
      Error_Handler();
    }
    // 标记RTC已配置
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x32F1);
  } else {
    // 从STANDBY模式唤醒，RTC配置仍然有效，不需要重新初始化时间和日期
    // 但需要继续执行后续代码，确保RTC中断等配置正常
    // return; // 注释掉这行，避免跳过闹钟中断配置
  }


  /* USER CODE BEGIN RTC_Init 2 */
  // 不再默认设置闹铃，闹铃将通过串口命令设置
  
  // 确保使能RTC闹铃中断
  HAL_NVIC_SetPriority(RTC_TAMP_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(RTC_TAMP_IRQn);
  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */

  /** Initializes the peripherals clocks
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
    __HAL_RCC_RTCAPB_CLK_ENABLE();

    /* RTC interrupt Init */
    HAL_NVIC_SetPriority(RTC_TAMP_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_TAMP_IRQn);
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
    __HAL_RCC_RTCAPB_CLK_DISABLE();

    /* RTC interrupt Deinit */
    HAL_NVIC_DisableIRQ(RTC_TAMP_IRQn);
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
// 声明蜂鸣器标志位（定义在main.c中）
extern volatile uint8_t playBeepFlag;

/**
  * @brief  Alarm A callback.
  * @param  hrtc RTC handle
  * @retval None
  */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  // 闹铃触发，清除RTC闹铃标志位
  __HAL_RTC_ALARM_CLEAR_FLAG(hrtc, RTC_FLAG_ALRAF);

  // 设置蜂鸣器播放标志，由主循环处理
  playBeepFlag = 1;
}

/* USER CODE END 1 */