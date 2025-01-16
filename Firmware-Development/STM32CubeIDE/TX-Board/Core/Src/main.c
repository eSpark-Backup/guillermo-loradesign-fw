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
#include "dma.h"
#include "app_lorawan.h"
#include "rtc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include  <stdio.h>
#include <stdarg.h>
//#include "TinyGPS++.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
int _write(int file, char *ptr, int len) {
    for (int i = 0; i < len; i++) {
        ITM_SendChar(ptr[i]);
    }
    return len;
}

void itm_printf(const char* format, ...) {
    char buffer[256]; // Temporary buffer for formatted string
    va_list args;
    va_start(args, format);
    int length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    for (int i = 0; i < length; ++i) {
        ITM_SendChar((uint8_t)buffer[i]);
    }
}
#define printf itm_printf
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
unsigned long lastGnssDisplay = 0;
//TinyGPSPlus gps;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void displayInfo()
{
//    printf("Location: ");
//    if (gps.location.isValid())
//    {
//        printf("%.6f,%.6f", gps.location.lat(), gps.location.lng());
//    }
//    else
//    {
//        printf("INVALID");
//    }
//
//    printf("  Date/Time: ");
//    if (gps.date.isValid())
//    {
//        uint8_t month, day;
//        uint16_t year;
//        month = gps.date.month();
//        day = gps.date.day();
//        year = gps.date.year();
//        year = gps.date.year();
//        printf("%02d/%02d/%04d ", gps.date.month(), gps.date.day(), gps.date.year());
//
//    }
//    else
//    {
//        printf("INVALID ");
//    }
//
//    if (gps.time.isValid())
//    {
//        printf("%02d:%02d:%02d.%02d\r\n",
//               gps.time.hour(),
//               gps.time.minute(),
//               gps.time.second(),
//               gps.time.centisecond());
//    }
//    else
//    {
//        printf("INVALID\r\n");
//    }
}
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
//  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_LoRaWAN_Init();
//  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_RESET);
  HAL_Delay(400);
  HAL_GPIO_WritePin(GNSS_RESET_GPIO_Port, GNSS_RESET_Pin, GPIO_PIN_SET);
//  HAL_Delay(1000);
//  while(1)
//  {
//	  itm_printf("No GPS detected: che");
//
//	  HAL_Delay(1000);
//  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  uint32_t lastGnssDisplay = 0;
  while (1)
  {
//	  uint8_t data;  // Buffer to store GNSS data
//	  if (HAL_UART_Receive(&huart1, &data, 1, 10) == HAL_OK)
//	  {
//		  gps.encode(data);
//	  }
//	  else
//	  {
//		  if ((HAL_GetTick() - lastGnssDisplay) > 500)
//		  {
//		  	 lastGnssDisplay = HAL_GetTick();
//		  	 displayInfo();
//		  }
//
//	  }
//
//	  // Check for GNSS module connectivity
//	  if (HAL_GetTick() > 5000 && gps.charsProcessed() < 10)
//	  {
//		  printf("No GPS detected: check wiring.\r\n");
//		  while (1)
//		  {
//			  HAL_Delay(10); // Stay here if no GPS is detected
//		  }
//	  }
//	  HAL_GPIO_TogglePin(GPIOA, LED1_Pin|LED2_Pin);
//	  HAL_Delay(100);
    /* USER CODE END WHILE */
    MX_LoRaWAN_Process();

    /* USER CODE BEGIN 3 */
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
  RCC_OscInitStruct.LSIDiv = RCC_LSI_DIV1;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK3|RCC_CLOCKTYPE_HCLK
                              |RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK3Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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

#ifdef  USE_FULL_ASSERT
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
