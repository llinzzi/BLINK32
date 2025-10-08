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
typedef enum {
  LED_OFF = 0,
  LED_ON,
  LED_ALARM  // 新增闹钟状态
} LED_StateTypeDef;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PWM_MAX_VALUE     1600
#define PWM_STEP_SIZE     16
#define DEBOUNCE_DELAY    50
#define BEEP_DURATION     1000  // 1秒蜂鸣时间
#define ALARM_FLASH_PERIOD 200  // 200ms闪烁周期
#define BEEP_PWM_PERIOD   987   // TIM16周期值
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
LED_StateTypeDef led_state = LED_OFF;
uint32_t pwm_value = 0;
uint32_t last_button_check = 0;
uint8_t button_prev_state = 1;  // Assume button is not pressed initially (pull-up)
uint8_t beep_active = 0;
uint32_t beep_start_time = 0;
uint8_t alarm_active = 0;
uint32_t alarm_flash_time = 0;
uint8_t alarm_led_state = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void LED_Breathing_Control(void);
void Button_Check(void);
void Beep_Control(void);
void Alarm_LED_Control(void);
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
  alarm_flash_time = HAL_GetTick();
  alarm_led_state = 1;
  
  /* Set LED to full brightness initially */
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, PWM_MAX_VALUE);
  
  /* Activate beep */
  beep_active = 1;
  beep_start_time = HAL_GetTick();
  
  /* Set PWM for beep sound (50% duty cycle) */
  __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, BEEP_PWM_PERIOD/2);  // 50%占空比
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
      // Debounce delay
      HAL_Delay(DEBOUNCE_DELAY);
      
      // Check if button is still pressed
      if (HAL_GPIO_ReadPin(BIG_BTN_GPIO_Port, BIG_BTN_Pin) == 0)
      {
        // If in alarm mode, disable alarm mode
        if (alarm_active)
        {
          alarm_active = 0;
          beep_active = 0;
          __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 0);
          
          // Return to previous LED state
          if(led_state == LED_ON)
          {
            __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, pwm_value);
          }
          else
          {
            __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0);
          }
        }
        else
        {
          // Toggle LED state
          if(led_state == LED_OFF)
          {
            led_state = LED_ON;
          }
          else
          {
            led_state = LED_OFF;
          }
        }
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
    }
  }
}

/**
  * @brief  Control alarm LED flashing
  * @retval None
  */
void Alarm_LED_Control(void)
{
  if (alarm_active)
  {
    uint32_t current_time = HAL_GetTick();
    if ((current_time - alarm_flash_time) >= ALARM_FLASH_PERIOD)
    {
      alarm_flash_time = current_time;
      alarm_led_state = !alarm_led_state;  // Toggle LED state
      
      if (alarm_led_state)
      {
        __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, PWM_MAX_VALUE);  // Full brightness
      }
      else
      {
        __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0);  // Off
      }
    }
  }
}

/**
  * @brief  Control LED breathing effect
  * @retval None
  */
void LED_Breathing_Control(void)
{
  // 如果在闹钟模式下，不执行正常的LED渐变控制
  if (alarm_active)
  {
    Alarm_LED_Control();
    return;
  }
  
  if(led_state == LED_ON)
  {
    /* Gradually increase brightness */
    if(pwm_value < PWM_MAX_VALUE)
    {
      pwm_value += PWM_STEP_SIZE;
      if(pwm_value > PWM_MAX_VALUE)
      {
        pwm_value = PWM_MAX_VALUE;
      }
      __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, pwm_value);
    }
  }
  else
  {
    /* Gradually decrease brightness */
    if(pwm_value > 0)
    {
      if(pwm_value > PWM_STEP_SIZE)
      {
        pwm_value -= PWM_STEP_SIZE;
      }
      else
      {
        pwm_value = 0;
      }
      __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, pwm_value);
    }
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
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Button_Check();
    LED_Breathing_Control();
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
