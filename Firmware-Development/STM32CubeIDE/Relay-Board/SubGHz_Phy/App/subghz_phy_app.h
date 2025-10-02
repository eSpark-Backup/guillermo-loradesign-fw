/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    subghz_phy_app.h
  * @author  MCD Application Team
  * @brief   Header of application of the SubGHz_Phy Middleware
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __SUBGHZ_PHY_APP_H__
#define __SUBGHZ_PHY_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include "platform.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* MODEM type: one shall be 1 the other shall be 0 */
#define USE_MODEM_LORA  1
#define USE_MODEM_FSK   0

#ifndef TX_OUTPUT_POWER   /* please, to change this value, redefine it in USER CODE SECTION */
#define TX_OUTPUT_POWER  20        /* dBm */
#endif /* TX_OUTPUT_POWER */

/* ---------------- LoRa Config per MCU ---------------- */
#if (RELAYBOARD == MCU_1)
/* MCU 1: 915 MHz, SF12, BW = 125 kHz */
#define RF_FREQUENCY                915000000 /* Hz */
#define LORA_BANDWIDTH              0         /* [0: 125 kHz, 1: 250 kHz, 2: 500 kHz] */
#define LORA_SPREADING_FACTOR       12        /* SF12 */
#define LORA_CODINGRATE             1         /* 4/5 */
#elif (RELAYBOARD == MCU_2)
/* MCU 2: 918 MHz, SF10, BW = 250 kHz */
#define RF_FREQUENCY                918000000 /* Hz */
#define LORA_BANDWIDTH              1         /* 250 kHz */
#define LORA_SPREADING_FACTOR       10        /* SF10 */
#define LORA_CODINGRATE             1         /* 4/5 */
#else
#error "RELAYBOARD not defined as MCU_1 or MCU_2"
#endif

#define LORA_PREAMBLE_LENGTH        8         /* Same for Tx and Rx */
#define LORA_SYMBOL_TIMEOUT         5         /* Symbols */
#define LORA_FIX_LENGTH_PAYLOAD_ON  false
#define LORA_IQ_INVERSION_ON        false

#define PAYLOAD_LEN                 64

/* USER CODE BEGIN EC */

/* Definitions */
#define RX_CONTINUOUS_ON            1
#define RADIO_TX                    0 /* do not change*/
#define RADIO_RX                    1 /* do not change*/
#define PRBS9_INIT                  ( ( uint16_t) 2 )

/* Test Configurations */
/* if TEST_MODE is RADIO_TX, board will send packet indefinitely */
/* if TEST_MODE is RADIO_RX, board will receive packet indefinitely */
#define TEST_MODE                   RADIO_TX

/* USER CODE END EC */

/* Exported functions prototypes ---------------------------------------------*/
void SubghzApp_Init(void);

/* USER CODE BEGIN EFP */
void SwitchToTxMode(uint8_t *payload, uint16_t size);
void SwitchToRxMode(void);
uint32_t isTXDone(void);
/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /*__SUBGHZ_PHY_APP_H__*/
