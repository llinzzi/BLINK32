/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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

/* USER CODE BEGIN PV */
// 灯光状态枚举
LightStateTypeDef lightState = LIGHT_OFF;
uint32_t pressStartTime = 0;
uint32_t dimStartTime = 0;
uint8_t buttonPressed = 0;
// 添加时间打印相关变量
uint32_t lastPrintTime = 0;

// 添加串口接收相关变量
uint8_t rxBuffer[50];  // 接收缓冲区
uint8_t rxIndex = 0;   // 接收索引

typedef enum {
  WAKEUP_SOURCE_RESET = 0,
  WAKEUP_SOURCE_BUTTON,
  WAKEUP_SOURCE_ALARM
} WakeupSourceTypeDef;

WakeupSourceTypeDef wakeupSource = WAKEUP_SOURCE_RESET;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// 串口命令处理函数声明
void ProcessSerialCommand(char* command);
void SetSystemTime(char* timeStr);
void SetSystemDate(char* dateStr);
// 添加闹铃处理函数声明
void SetAlarmTime(char* alarmStr);
void CancelAlarm(void);
void PlayBeepSound(void); 
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

// 检查唤醒源
  if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET) {
    // 从Standby模式唤醒
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_WUF1) != RESET) {
      // 按键唤醒 (WKUP1 - PA0)
      wakeupSource = WAKEUP_SOURCE_BUTTON;
      __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF1);
    } else if (__HAL_PWR_GET_FLAG(PWR_FLAG_WUF4) != RESET) {
      // RTC闹铃唤醒 (WKUP4)
      wakeupSource = WAKEUP_SOURCE_ALARM;
      __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF4);
    } else {
      // 其他唤醒源
      wakeupSource = WAKEUP_SOURCE_RESET;
    }
    // 清除Standby标志
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
  } else {
    // 复位启动
    wakeupSource = WAKEUP_SOURCE_RESET;
  }


  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RTC_Init();
  MX_TIM14_Init();
  MX_USART1_UART_Init();
  MX_TIM16_Init();
  MX_TIM17_Init();
  /* USER CODE BEGIN 2 */

  // 启动TIM14 PWM输出 (LEDA)
  HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);
  
  // 启动TIM17 PWM输出 (LEDB)
  HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1);

  // 启动TIM16 PWM输出 (蜂鸣器)
  HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
  __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 0);

  // 初始化完成后直接进入微光模式
  SetLightDim();
  lightState = LIGHT_DIM;
  dimStartTime = HAL_GetTick();
  
  // 启动串口接收
  HAL_UART_Receive_IT(&huart1, &rxBuffer[rxIndex], 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */


    // 每隔1秒打印系统时间（仅在非待机模式下）
    if ((HAL_GetTick() - lastPrintTime) >= 1000) {
      // 获取RTC时间
      RTC_TimeTypeDef sTime;
      RTC_DateTypeDef sDate;
      
      HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
      HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
      
      // 获取闹铃状态
      char* alarmStatus = GetAlarmStatus();
     // 根据唤醒源生成相应的字符串
      char* wakeupSourceStr;
      switch (wakeupSource) {
        case WAKEUP_SOURCE_BUTTON:
          wakeupSourceStr = "BUTTON";
          break;
        case WAKEUP_SOURCE_ALARM:
          wakeupSourceStr = "ALARM";
          break;
        case WAKEUP_SOURCE_RESET:
        default:
          wakeupSourceStr = "RESET";
          break;
      }
      
      // 格式化时间字符串，包含日期和闹铃信息
     char timeStr[120];
     sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d ALARM:%s WAKEUP:%s\r\n", 
              2000 + sDate.Year, sDate.Month, sDate.Date,
              sTime.Hours, sTime.Minutes, sTime.Seconds,
              alarmStatus, wakeupSourceStr);
      
      // 通过串口打印时间、日期和闹铃信息
      HAL_UART_Transmit(&huart1, (uint8_t*)timeStr, strlen(timeStr), HAL_MAX_DELAY);
      



      // 更新上次打印时间
      lastPrintTime = HAL_GetTick();
    }

    // 检查微光模式超时 (1分钟)
    if (lightState == LIGHT_DIM) {
      if ((HAL_GetTick() - dimStartTime) >= DIM_TIMEOUT_MS) {
        // 微光模式超时，关闭灯光并进入待机模式
        TurnOffLight();
        EnterStandbyMode();
      }
    }
    
    // 检查按键状态（软件轮询方式检测按键释放）
    if (buttonPressed) {
      if (HAL_GPIO_ReadPin(BIG_BTN_GPIO_Port, BIG_BTN_Pin) == GPIO_PIN_RESET) {
        // 按键已释放，处理按键事件
        buttonPressed = 0;
        uint32_t pressDuration = HAL_GetTick() - pressStartTime;
        
        // 根据当前状态和按压时间处理按键事件
        if (lightState == LIGHT_DIM && pressDuration <= SHORT_PRESS_MAX_MS) {
          // 微光模式下按键 短按：关闭灯光并进入待机模式
          TurnOffLight();
          EnterStandbyMode();
        } else if (lightState == LIGHT_BRIGHT) {
          // 高亮模式下按键：关闭灯光并进入待机模式
          TurnOffLight();
          EnterStandbyMode();
        }
      } else {
        // 按键仍处于按下状态，检查是否为长按
        uint32_t pressDuration = HAL_GetTick() - pressStartTime;
        if (pressDuration > LONG_PRESS_MS && lightState == LIGHT_DIM) {
          // 长按：进入高亮模式（无需等待释放）
          SetLightBright();
          lightState = LIGHT_BRIGHT;
          buttonPressed = 0; // 防止重复触发
        }
      }
    }
    
    // 短暂延时以减少CPU占用
    HAL_Delay(10);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
  * @brief  USART1中断回调函数
  * @param  huart 串口句柄
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance == USART1) {
    // 如果接收到回车符或换行符，则处理命令
    if(rxBuffer[rxIndex] == '\r' || rxBuffer[rxIndex] == '\n') {
      // 添加字符串结束符
      rxBuffer[rxIndex] = '\0';
      
      // 处理命令（如果缓冲区不为空）
      if(rxIndex > 0) {
        ProcessSerialCommand((char*)rxBuffer);
      }
      
      // 重置接收索引
      rxIndex = 0;
    } else {
      // 继续接收下一个字符
      rxIndex++;
      
      // 防止缓冲区溢出
      if(rxIndex >= sizeof(rxBuffer)-1) {
        rxIndex = 0;
      }
    }
    
    // 继续接收下一个字节
    HAL_UART_Receive_IT(&huart1, &rxBuffer[rxIndex], 1);
  }
}

/**
  * @brief  处理串口命令
  * @param  command 命令字符串
  * @retval None
  */
void ProcessSerialCommand(char* command) {
  // 移除可能存在的换行符
  char* newline = strchr(command, '\r');
  if(newline) *newline = '\0';
  newline = strchr(command, '\n');
  if(newline) *newline = '\0';
  
  // 解析命令
  if(strncmp(command, "TIME=", 5) == 0) {
    // 设置时间命令，格式为 TIME=HH:MM:SS
    SetSystemTime(command+5);
  } else if(strncmp(command, "DATE=", 5) == 0) {
    // 设置日期命令，格式为 DATE=YYYY-MM-DD
    SetSystemDate(command+5);
  } else if(strncmp(command, "ALARM=", 6) == 0) {
    // 设置闹铃命令，格式为 ALARM=HH:MM:SS 或 ALARM=OFF
    SetAlarmTime(command+6);
  } else if(strcmp(command, "BEEP=1") == 0) {
    // 播放提示音命令
    PlayBeepSound();
    char successMsg[] = "Beep sound played\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)successMsg, strlen(successMsg), HAL_MAX_DELAY);
  } else {
    // 未知命令，返回错误信息
    char errorMsg[] = "ERROR: Unknown command\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)errorMsg, strlen(errorMsg), HAL_MAX_DELAY);
  }
}

/**
  * @brief  设置系统时间
  * @param  timeStr 时间字符串，格式为 HH:MM:SS
  * @retval None
  */
void SetSystemTime(char* timeStr) {
  int hours, minutes, seconds;
  
  // 解析时间字符串
  if(sscanf(timeStr, "%d:%d:%d", &hours, &minutes, &seconds) == 3) {
    // 验证时间有效性
    if(hours >= 0 && hours <= 23 && 
       minutes >= 0 && minutes <= 59 && 
       seconds >= 0 && seconds <= 59) {
       
      // 设置RTC时间
      RTC_TimeTypeDef sTime;
      sTime.Hours = hours;
      sTime.Minutes = minutes;
      sTime.Seconds = seconds;
      sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
      sTime.StoreOperation = RTC_STOREOPERATION_RESET;
      
      if(HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK) {
        char successMsg[] = "Time set successfully\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)successMsg, strlen(successMsg), HAL_MAX_DELAY);
      } else {
        char errorMsg[] = "ERROR: Failed to set time\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)errorMsg, strlen(errorMsg), HAL_MAX_DELAY);
      }
    } else {
      char errorMsg[] = "ERROR: Invalid time format\r\n";
      HAL_UART_Transmit(&huart1, (uint8_t*)errorMsg, strlen(errorMsg), HAL_MAX_DELAY);
    }
  } else {
    char errorMsg[] = "ERROR: Invalid time format\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)errorMsg, strlen(errorMsg), HAL_MAX_DELAY);
  }
}

/**
  * @brief  设置系统日期
  * @param  dateStr 日期字符串，格式为 YYYY-MM-DD
  * @retval None
  */
void SetSystemDate(char* dateStr) {
  int year, month, day;
  
  // 解析日期字符串
  if(sscanf(dateStr, "%d:%d:%d", &year, &month, &day) == 3) {
    // 验证日期有效性
    if(year >= 2000 && year <= 2099 && 
       month >= 1 && month <= 12 && 
       day >= 1 && day <= 31) {
       
      // 转换年份为RTC格式 (0-99)
      year = year - 2000;
       
      // 设置RTC日期
      RTC_DateTypeDef sDate;
      sDate.Year = year;
      sDate.Month = month;
      sDate.Date = day;
      sDate.WeekDay = RTC_WEEKDAY_MONDAY; // 简单设置为周一，实际应该根据日期计算
      
      if(HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK) {
        char successMsg[] = "Date set successfully\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)successMsg, strlen(successMsg), HAL_MAX_DELAY);
      } else {
        char errorMsg[] = "ERROR: Failed to set date\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)errorMsg, strlen(errorMsg), HAL_MAX_DELAY);
      }
    } else {
      char errorMsg[] = "ERROR: Invalid date format\r\n";
      HAL_UART_Transmit(&huart1, (uint8_t*)errorMsg, strlen(errorMsg), HAL_MAX_DELAY);
    }
  } else {
    char errorMsg[] = "ERROR: Invalid date format\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)errorMsg, strlen(errorMsg), HAL_MAX_DELAY);
  }
}

/**
  * @brief  设置闹铃时间
  * @param  alarmStr 闹铃字符串，格式为 HH:MM:SS 或 OFF
  * @retval None
  */
void SetAlarmTime(char* alarmStr) {
  // 检查是否为取消闹铃命令
  if(strcmp(alarmStr, "OFF") == 0) {
    // 取消闹铃
    CancelAlarm();
    return;
  }
  
  int hours, minutes, seconds;
  
  // 解析时间字符串
  if(sscanf(alarmStr, "%d:%d:%d", &hours, &minutes, &seconds) == 3) {
    // 验证时间有效性
    if(hours >= 0 && hours <= 23 && 
       minutes >= 0 && minutes <= 59 && 
       seconds >= 0 && seconds <= 59) {
       
      // 设置RTC闹铃 - 使用星期模式实现每天重复
      RTC_AlarmTypeDef sAlarm;
      sAlarm.AlarmTime.Hours = hours;
      sAlarm.AlarmTime.Minutes = minutes;
      sAlarm.AlarmTime.Seconds = seconds;
      sAlarm.AlarmTime.SubSeconds = 0;
      sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
      sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
      sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
      sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
      sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_WEEKDAY;  // 改为星期模式
      sAlarm.AlarmDateWeekDay = 0x1F;  // 周一到周五（工作日）：0x1F = 0b00011111
      sAlarm.Alarm = RTC_ALARM_A;
      
      if(HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) == HAL_OK) {
        char successMsg[] = "Alarm set successfully\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)successMsg, strlen(successMsg), HAL_MAX_DELAY);
      } else {
        char errorMsg[] = "ERROR: Failed to set alarm\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)errorMsg, strlen(errorMsg), HAL_MAX_DELAY);
      }
    } else {
      char errorMsg[] = "ERROR: Invalid alarm time format\r\n";
      HAL_UART_Transmit(&huart1, (uint8_t*)errorMsg, strlen(errorMsg), HAL_MAX_DELAY);
    }
  } else {
    char errorMsg[] = "ERROR: Invalid alarm format\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)errorMsg, strlen(errorMsg), HAL_MAX_DELAY);
  }
}

/**
  * @brief  取消闹铃
  * @retval None
  */
void CancelAlarm(void) {
  // 取消RTC闹铃
  if(HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A) == HAL_OK) {
    char successMsg[] = "Alarm cancelled successfully\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)successMsg, strlen(successMsg), HAL_MAX_DELAY);
  } else {
    char errorMsg[] = "ERROR: Failed to cancel alarm\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)errorMsg, strlen(errorMsg), HAL_MAX_DELAY);
  }
}

/**
  * @brief  进入Standby模式
  * @retval None
  */
void EnterStandbyMode(void) {
// 关闭所有外设
  HAL_TIM_PWM_Stop(&htim14, TIM_CHANNEL_1);
  HAL_TIM_PWM_Stop(&htim17, TIM_CHANNEL_1);

  // 清除所有挂起的中断
  __HAL_RCC_CLEAR_RESET_FLAGS();
  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF1 | PWR_FLAG_WUF2 | PWR_FLAG_WUF4 | PWR_FLAG_WUF6 | PWR_FLAG_SB);

  // 使能唤醒引脚 (PA0)
  HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1_HIGH);
  
  // 使能RTC闹铃唤醒
  HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN4_HIGH);
  
  // 发送进入待机模式的消息
  char standbyMsg[] = "Entering Standby Mode...\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t*)standbyMsg, strlen(standbyMsg), HAL_MAX_DELAY);
  
  // 重置时间打印变量
  lastPrintTime = 0;
  
  // // 进入Standby模式
  HAL_PWR_EnterSTANDBYMode();
}

/**
  * @brief  设置微光模式 (10% PWM)
  * @retval None
  */
void SetLightDim(void) {
  // 设置PWM占空比为10% (周期为1600，10%为160)
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0);  // LEDA 10%
  __HAL_TIM_SET_COMPARE(&htim17, TIM_CHANNEL_1, 100);// LEDB 10%
}

/**
  * @brief  设置高亮模式 (50% PWM)
  * @retval None
  */
void SetLightBright(void) {
  // 设置PWM占空比为50% (周期为1600，50%为800)
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 800);
  __HAL_TIM_SET_COMPARE(&htim17, TIM_CHANNEL_1, 160);
}

/**
  * @brief  关闭灯光
  * @retval None
  */
void TurnOffLight(void) {
  // 设置PWM占空比为0%
  __HAL_TIM_SET_COMPARE(&htim17, TIM_CHANNEL_1, 0);
}

/**
  * @brief  播放提示音
  * @retval None
  */
void PlayBeepSound(void) {

  
  // 播放一个非常柔和、缓慢的提示音
  // 音符1: 很低的音调
  htim16.Init.Prescaler = 9;     // 更高的预分频器
  htim16.Init.Period = 3999;     // 更大的周期值，产生约666Hz的频率
  if (HAL_TIM_PWM_Init(&htim16) != HAL_OK) {
    Error_Handler();
  }
  
  // 设置20%占空比 (非常柔和的声音)
  __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 800);
  
  // 持续800ms
  HAL_Delay(800);
  
  // 音符2: 稍高一点的音调
  htim16.Init.Period = 2999;     // 周期值，产生约890Hz的频率
  if (HAL_TIM_PWM_Init(&htim16) != HAL_OK) {
    Error_Handler();
  }
  
  // 设置25%占空比
  __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 750);
  
  // 持续600ms
  HAL_Delay(600);
  
  // 确保完全关闭蜂鸣器
  __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 0);
  

  if (HAL_TIM_PWM_Init(&htim16) != HAL_OK) {
    Error_Handler();
  }
  
  // 短暂延时确保声音完全停止
  HAL_Delay(100);
}

/**
  * @brief  获取闹铃状态
  * @retval 闹铃状态字符串
  */
char* GetAlarmStatus(void) {
  RTC_AlarmTypeDef sAlarm;
  static char alarmStr[20];
  
  // 尝试获取闹铃信息
  if (HAL_RTC_GetAlarm(&hrtc, &sAlarm, RTC_ALARM_A, RTC_FORMAT_BIN) == HAL_OK) {
    // 如果闹铃已设置，返回闹铃时间
    sprintf(alarmStr, "%02d:%02d:%02d", sAlarm.AlarmTime.Hours, sAlarm.AlarmTime.Minutes, sAlarm.AlarmTime.Seconds);
    return alarmStr;
  } else {
    // 如果闹铃未设置或获取失败，返回OFF
    return "OFF";
  }
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
