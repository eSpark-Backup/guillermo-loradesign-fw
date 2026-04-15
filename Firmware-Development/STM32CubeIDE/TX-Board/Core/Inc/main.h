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
#include "stm32wlxx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define ENABLE_GNSS_STREAM_DEBUG
#define DEBUG_PORT       		0
#define GNSS_STREAM_PORT		1
#define TRACE_PORT          2

// #define LOG_D printf
// #define LOG_D(fmt, ...)
 #define LOG_D(fmt, ...)  itm_printf(DEBUG_PORT, fmt, ##__VA_ARGS__)

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
extern void itm_printf(uint32_t port, const char* format, ...);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define RTC_PREDIV_A ((1<<(15-RTC_N_PREDIV_S))-1)
#define RTC_N_PREDIV_S 10
#define RTC_PREDIV_S ((1<<RTC_N_PREDIV_S)-1)
#define GNSS_PWR_Pin GPIO_PIN_5
#define GNSS_PWR_GPIO_Port GPIOB
#define GPS_VBCKUP_EN_Pin GPIO_PIN_8
#define GPS_VBCKUP_EN_GPIO_Port GPIOB
#define RTC_IRQ_Pin GPIO_PIN_0
#define RTC_IRQ_GPIO_Port GPIOA
#define GNSS_RESET_Pin GPIO_PIN_4
#define GNSS_RESET_GPIO_Port GPIOA
#define LED2_Pin GPIO_PIN_11
#define LED2_GPIO_Port GPIOA
#define LED1_Pin GPIO_PIN_12
#define LED1_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */
#define __ITM_PORT_MASK_1(p)           (1U << (p))
#define __ITM_PORT_MASK_2(p, ...)      ((1U << (p)) | __ITM_PORT_MASK_1(__VA_ARGS__))
#define __ITM_PORT_MASK_3(p, ...)      ((1U << (p)) | __ITM_PORT_MASK_2(__VA_ARGS__))
#define __ITM_PORT_MASK_4(p, ...)      ((1U << (p)) | __ITM_PORT_MASK_3(__VA_ARGS__))
#define __ITM_PORT_MASK_5(p, ...)      ((1U << (p)) | __ITM_PORT_MASK_4(__VA_ARGS__))
#define __ITM_PORT_MASK_6(p, ...)      ((1U << (p)) | __ITM_PORT_MASK_5(__VA_ARGS__))
#define __ITM_PORT_MASK_7(p, ...)      ((1U << (p)) | __ITM_PORT_MASK_6(__VA_ARGS__))
#define __ITM_PORT_MASK_8(p, ...)      ((1U << (p)) | __ITM_PORT_MASK_7(__VA_ARGS__))

#define GET_MACRO(_1,_2,_3,_4,_5,_6,_7,_8,NAME,...) NAME
#define __ITM_PORT_MASK(...) GET_MACRO(__VA_ARGS__, \
      __ITM_PORT_MASK_8, __ITM_PORT_MASK_7, __ITM_PORT_MASK_6, __ITM_PORT_MASK_5, \
      __ITM_PORT_MASK_4, __ITM_PORT_MASK_3, __ITM_PORT_MASK_2, __ITM_PORT_MASK_1)(__VA_ARGS__)


#define INIT_ITM_DEBUG(...)                                       \
  do {                                                            \
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;              \
    ITM->LAR = 0xC5ACCE55;                                       \
    ITM->TCR = ITM_TCR_ITMENA_Msk |                              \
               ITM_TCR_SYNCENA_Msk |                             \
               ITM_TCR_TSENA_Msk   |                             \
               (1 << ITM_TCR_TraceBusID_Pos);                    \
    ITM->TER = __ITM_PORT_MASK(__VA_ARGS__);                     \
  } while (0)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
