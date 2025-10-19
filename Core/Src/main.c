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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  
  // 初始化完成后直接进入微光模式
  SetLightDim();
  lightState = LIGHT_DIM;
  dimStartTime = HAL_GetTick();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
        if (lightState == LIGHT_DIM) {
          // 微光模式下按键
          if (pressDuration >= SHORT_PRESS_MIN_MS && pressDuration <= SHORT_PRESS_MAX_MS) {
            // 短按：关闭灯光并进入待机模式
            TurnOffLight();
            EnterStandbyMode();
          } else if (pressDuration > LONG_PRESS_MS) {
            // 长按：进入高亮模式
            SetLightBright();
            lightState = LIGHT_BRIGHT;
          }
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
  * @brief  进入Standby模式
  * @retval None
  */
void EnterStandbyMode(void) {
  // 关闭所有外设
  HAL_TIM_PWM_Stop(&htim14, TIM_CHANNEL_1);
  
  // 清除所有挂起的中断
  __HAL_RCC_CLEAR_RESET_FLAGS();
  
  // 使能唤醒引脚 (PA0)
  HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1_HIGH);
  
  // // 进入Standby模式
  HAL_PWR_EnterSTANDBYMode();
}

/**
  * @brief  设置微光模式 (10% PWM)
  * @retval None
  */
void SetLightDim(void) {
  // 设置PWM占空比为10% (周期为1600，10%为160)
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 160);
}

/**
  * @brief  设置高亮模式 (50% PWM)
  * @retval None
  */
void SetLightBright(void) {
  // 设置PWM占空比为50% (周期为1600，50%为800)
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 800);
}

/**
  * @brief  关闭灯光
  * @retval None
  */
void TurnOffLight(void) {
  // 设置PWM占空比为0%
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0);
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