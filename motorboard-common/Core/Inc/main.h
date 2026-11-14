/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32g0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

// address pins all tied to GND
#define I2C_ADDR_LM75 0x48

// The AS5047P claims 4000 ticks, but the timer HW effectively scales by 2
#define ENC_TICKS_PER_REV 2000U

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define FAULT_LED_Pin GPIO_PIN_13
#define FAULT_LED_GPIO_Port GPIOC
#define CHIPID_A_Pin GPIO_PIN_14
#define CHIPID_A_GPIO_Port GPIOC
#define CHIPID_B_Pin GPIO_PIN_15
#define CHIPID_B_GPIO_Port GPIOC
#define ENC_A_Pin GPIO_PIN_0
#define ENC_A_GPIO_Port GPIOA
#define ENC_B_Pin GPIO_PIN_1
#define ENC_B_GPIO_Port GPIOA
#define SCOPE_OUT_Pin GPIO_PIN_4
#define SCOPE_OUT_GPIO_Port GPIOA
#define I_SENSE_Pin GPIO_PIN_1
#define I_SENSE_GPIO_Port GPIOB
#define OC_COMP_INT_Pin GPIO_PIN_12
#define OC_COMP_INT_GPIO_Port GPIOB
#define OC_SEL_Pin GPIO_PIN_11
#define OC_SEL_GPIO_Port GPIOA
#define HALL_A_Pin GPIO_PIN_5
#define HALL_A_GPIO_Port GPIOB
#define HALL_A_EXTI_IRQn EXTI4_15_IRQn
#define HALL_B_Pin GPIO_PIN_8
#define HALL_B_GPIO_Port GPIOB
#define HALL_B_EXTI_IRQn EXTI4_15_IRQn
#define HALL_C_Pin GPIO_PIN_9
#define HALL_C_GPIO_Port GPIOB
#define HALL_C_EXTI_IRQn EXTI4_15_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
