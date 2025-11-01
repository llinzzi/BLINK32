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

  /* USER CODE BEGIN Check_RTC_BKUP */
  // 检查RTC备份域是否已被配置
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == 0x32F1) {
    // 从STANDBY模式唤醒，RTC配置仍然有效，不需要重新初始化时间和日期
    // 但需要继续执行后续代码，确保RTC中断等配置正常
    // return; // 注释掉这行，避免跳过闹钟中断配置
  } else {
    // 首次配置RTC或备份数据丢失
    // 标记RTC已配置
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x32F1);
  }
  
  // 根据备份标志决定是否设置时间和日期
  uint32_t rtcConfigured = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0);
  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  // 只有首次配置或备份域丢失时才设置时间和日期
  if (rtcConfigured != 0x32F1) {
    sTime.Hours = 0x7;
    sTime.Minutes = 0x59;
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
  }

  /** Enable the Alarm A
  */
  // 注释掉默认闹钟设置，避免覆盖用户通过串口设置的闹钟
  // 只在首次配置时设置默认闹钟（可选）
  // sAlarm.AlarmTime.Hours = 0x8;
  // sAlarm.AlarmTime.Minutes = 0x0;
  // sAlarm.AlarmTime.Seconds = 0x0;
  // sAlarm.AlarmTime.SubSeconds = 0x0;
  // sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  // sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
  // sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;  // 忽略日期匹配,实现每日重复
  // sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
  // sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;  // 使用日期模式
  // sAlarm.AlarmDateWeekDay = 0x1;  // 设置为1日(由于掩码忽略此字段,实现每天触发)
  // sAlarm.Alarm = RTC_ALARM_A;
  // if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BCD) != HAL_OK)
  // {
  //   Error_Handler();
  // }
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
/**
  * @brief  Alarm A callback.
  * @param  hrtc RTC handle
  * @retval None
  */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  // 闹铃触发，清除RTC闹铃标志位
  __HAL_RTC_ALARM_CLEAR_FLAG(hrtc, RTC_FLAG_ALRAF);
  
  // 注意：在中断回调中不能直接调用PlayBeepSound，需要通过标志位在主循环中处理
  // 这里我们只清除标志位，确保下次闹铃能够正常触发
}

/* USER CODE END 1 */