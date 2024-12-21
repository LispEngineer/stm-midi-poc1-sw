/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "stm32f7xx_hal.h"
#include "stm32f7xx_ll_usart.h"
#include "stm32f7xx_ll_rcc.h"
#include "stm32f7xx_ll_bus.h"
#include "stm32f7xx_ll_cortex.h"
#include "stm32f7xx_ll_system.h"
#include "stm32f7xx_ll_utils.h"
#include "stm32f7xx_ll_pwr.h"
#include "stm32f7xx_ll_gpio.h"
#include "stm32f7xx_ll_dma.h"

#include "stm32f7xx_ll_exti.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

void reinit_uarts(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define GPIO_PC0_Pin GPIO_PIN_0
#define GPIO_PC0_GPIO_Port GPIOC
#define GPIO_PC3_Pin GPIO_PIN_3
#define GPIO_PC3_GPIO_Port GPIOC
#define GPIO_PA6_Pin GPIO_PIN_6
#define GPIO_PA6_GPIO_Port GPIOA
#define GPIO_PC4_Pin GPIO_PIN_4
#define GPIO_PC4_GPIO_Port GPIOC
#define AUDIO_MUTE_Pin GPIO_PIN_0
#define AUDIO_MUTE_GPIO_Port GPIOB
#define HP_GAIN1_Pin GPIO_PIN_1
#define HP_GAIN1_GPIO_Port GPIOB
#define HP_GAIN0_Pin GPIO_PIN_2
#define HP_GAIN0_GPIO_Port GPIOB
#define LED_BLUE_Pin GPIO_PIN_12
#define LED_BLUE_GPIO_Port GPIOB
#define LED_GREEN_Pin GPIO_PIN_14
#define LED_GREEN_GPIO_Port GPIOB
#define LED_RED_Pin GPIO_PIN_15
#define LED_RED_GPIO_Port GPIOB
#define GPIO_PC8_Pin GPIO_PIN_8
#define GPIO_PC8_GPIO_Port GPIOC
#define GPIO_PC9_Pin GPIO_PIN_9
#define GPIO_PC9_GPIO_Port GPIOC
#define SPI2_CS_Pin GPIO_PIN_15
#define SPI2_CS_GPIO_Port GPIOA
#define BTN2_Pin GPIO_PIN_3
#define BTN2_GPIO_Port GPIOB
#define BTN1_Pin GPIO_PIN_4
#define BTN1_GPIO_Port GPIOB
#define SPI2_RESET_Pin GPIO_PIN_5
#define SPI2_RESET_GPIO_Port GPIOB
#define SPI2_DC_Pin GPIO_PIN_8
#define SPI2_DC_GPIO_Port GPIOB
#define GPIO_PB9_Pin GPIO_PIN_9
#define GPIO_PB9_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
