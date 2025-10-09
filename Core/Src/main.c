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
LED_StateTypeDef led_state = LED_OFF;
System_StateTypeDef system_state = SYSTEM_NORMAL;
uint32_t pwm_value = 0;
uint32_t last_button_press_time = 0;
uint32_t last_button_check = 0;
uint8_t button_prev_state = 1;  // Assume button is not pressed initially (pull-up)
uint8_t button_press_detected = 0;
uint8_t beep_active = 0;
uint32_t beep_start_time = 0;
uint8_t alarm_active = 0;
uint32_t alarm_start_time = 0;
uint32_t alarm_flash_time = 0;
uint8_t alarm_led_state = 0;
uint32_t alarm_flash_period = ALARM_INITIAL_PERIOD;
uint8_t test_mode = 0;  // 测试模式标志
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void LED_Control(void);
void Button_Check(void);
void Beep_Control(void);
void Alarm_Control(void);
void Enter_Low_Power_Mode(void);
void Exit_Low_Power_Mode(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/**
  * @brief  Alarm callback in non-blocking mode
  * @param  hrtc: RTC handle
  * @retval None
  */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  /* Activate alarm mode */
  alarm_active = 1;
  alarm_start_time = HAL_GetTick();
  alarm_flash_time = HAL_GetTick();
  alarm_led_state = 1;
  alarm_flash_period = ALARM_INITIAL_PERIOD;
  
  /* Set LED to full brightness initially */
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, PWM_BRIGHT_VALUE);
  
  /* Activate beep */
  beep_active = 1;
  beep_start_time = HAL_GetTick();
  
  /* Set PWM for beep sound (50% duty cycle) */
  __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, BEEP_PWM_PERIOD/2);  // 50%占空比
  
  /* Exit low power mode if in it */
  if (system_state == SYSTEM_LOW_POWER) {
    Exit_Low_Power_Mode();
  }
}

/**
  * @brief  Check button state and handle press/release
  * @retval None
  */
void Button_Check(void)
{
  uint32_t current_time = HAL_GetTick();
  
  // Check button state every 10ms
  if ((current_time - last_button_check) >= 10)
  {
    last_button_check = current_time;
    
    uint8_t button_current_state = HAL_GPIO_ReadPin(BIG_BTN_GPIO_Port, BIG_BTN_Pin);
    
    // Check for button press (falling edge)
    if ((button_prev_state == 1) && (button_current_state == 0))
    {
      // Record press time
      last_button_press_time = current_time;
      button_press_detected = 1;
    }
    // Check for button release (rising edge)
    else if ((button_prev_state == 0) && (button_current_state == 1) && button_press_detected)
    {
      // Debounce delay
      HAL_Delay(DEBOUNCE_DELAY);
      
      // Check if button is still released
      if (HAL_GPIO_ReadPin(BIG_BTN_GPIO_Port, BIG_BTN_Pin) == 1)
      {
        uint32_t press_duration = current_time - last_button_press_time;
        
        // If in alarm mode, disable alarm mode
        if (alarm_active)
        {
          alarm_active = 0;
          beep_active = 0;
          __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 0);
          
          // 停止LED闪烁，恢复到熄灭状态并进入低功耗模式
          led_state = LED_OFF;
          pwm_value = 0;
          __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0);
          system_state = SYSTEM_LOW_POWER;
          Enter_Low_Power_Mode();
        }
        else
        {
          // Short press (< 2 seconds)
          if (press_duration < LONG_PRESS_TIME)
          {
            // Toggle LED state: OFF -> DIM -> BRIGHT -> OFF
            switch(led_state)
            {
              case LED_OFF:
                led_state = LED_DIM;
                pwm_value = PWM_DIM_VALUE;
                system_state = SYSTEM_NORMAL;
                Exit_Low_Power_Mode();
                break;
              case LED_DIM:
                led_state = LED_BRIGHT;
                pwm_value = PWM_BRIGHT_VALUE;
                break;
              case LED_BRIGHT:
                led_state = LED_OFF;
                pwm_value = 0;
                system_state = SYSTEM_LOW_POWER;
                Enter_Low_Power_Mode();
                break;
              default:
                break;
            }
          }
          // Long press (>= 2 seconds)
          else
          {
            if (led_state == LED_OFF)
            {
              led_state = LED_BRIGHT;
              pwm_value = PWM_BRIGHT_VALUE;
              system_state = SYSTEM_NORMAL;
              Exit_Low_Power_Mode();
            }
            else
            {
              led_state = LED_OFF;
              pwm_value = 0;
              system_state = SYSTEM_LOW_POWER;
              Enter_Low_Power_Mode();
            }
          }
        }
        
        button_press_detected = 0;
      }
    }
    
    button_prev_state = button_current_state;
  }
}

/**
  * @brief  Control beep duration
  * @retval None
  */
void Beep_Control(void)
{
  if (beep_active)
  {
    if ((HAL_GetTick() - beep_start_time) >= BEEP_DURATION)
    {
      beep_active = 0;
      /* Stop PWM for beep */
      __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 0);
      
      // 闹铃结束后进入低功耗模式
      alarm_active = 0;
      led_state = LED_OFF;
      pwm_value = 0;
      __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0);
      system_state = SYSTEM_LOW_POWER;
      Enter_Low_Power_Mode();
    }
  }
}

/**
  * @brief  Control alarm LED flashing and frequency increase
  * @retval None
  */
void Alarm_Control(void)
{
  if (alarm_active)
  {
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_time = current_time - alarm_start_time;
    
    // 每5秒递增一次闪烁频率，直到达到最高速度
    if (elapsed_time > 0 && (elapsed_time % 5000) < 100) {
      if (alarm_flash_period > ALARM_MIN_PERIOD) {
        alarm_flash_period -= ALARM_PERIOD_STEP;
        if (alarm_flash_period < ALARM_MIN_PERIOD) {
          alarm_flash_period = ALARM_MIN_PERIOD;
        }
      }
    }
    
    // 30分钟后进入常亮状态
    if (elapsed_time >= 1800000) { // 30分钟 = 1800000毫秒
      led_state = LED_ALARM_ON;
      __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, PWM_BRIGHT_VALUE);
    } else {
      // 闪烁状态
      if ((current_time - alarm_flash_time) >= alarm_flash_period)
      {
        alarm_flash_time = current_time;
        alarm_led_state = !alarm_led_state;  // Toggle LED state
        
        if (alarm_led_state)
        {
          __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, PWM_BRIGHT_VALUE);  // Full brightness
        }
        else
        {
          __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0);  // Off
        }
      }
    }
  }
}

/**
  * @brief  Control LED states
  * @retval None
  */
void LED_Control(void)
{
  // 如果在闹钟模式下，使用闹钟控制逻辑
  if (alarm_active)
  {
    Alarm_Control();
    return;
  }
  
  // 正常模式下的LED控制
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, pwm_value);
}

/**
  * @brief  Enter low power mode
  * @retval None
  */
void Enter_Low_Power_Mode(void)
{
  // 停止不必要的外设以节省功耗
  HAL_TIM_PWM_Stop(&htim14, TIM_CHANNEL_1);
  HAL_TIM_PWM_Stop(&htim16, TIM_CHANNEL_1);
  
  // 进入睡眠模式
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

/**
  * @brief  Exit low power mode
  * @retval None
  */
void Exit_Low_Power_Mode(void)
{
  // 重新启动必要的外设
  HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
  
  // 设置当前PWM值
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, pwm_value);
  if (beep_active) {
    __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, BEEP_PWM_PERIOD/2);
  } else {
    __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 0);
  }
}
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
  /* USER CODE BEGIN 2 */
  /* Start PWM signal generation for LED */
  HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);
  /* Set initial PWM value to 0 */
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0);
  
  /* Initialize TIM16 for beep (but don't start it yet) */
  HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
  __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 0);
  
  
  /* Enter low power mode initially */
  system_state = SYSTEM_LOW_POWER;
  Enter_Low_Power_Mode();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Button_Check();
    LED_Control();
    Beep_Control();
    HAL_Delay(10);  /* 10ms delay for smooth transition */
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