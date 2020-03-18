/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
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
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal.h"

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

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SW_USER_Pin GPIO_PIN_13
#define SW_USER_GPIO_Port GPIOG
#define LED_2_Pin GPIO_PIN_7
#define LED_2_GPIO_Port GPIOD
#define LED_1_Pin GPIO_PIN_6
#define LED_1_GPIO_Port GPIOD
#define USB_OTG_FS_ID_Pin GPIO_PIN_10
#define USB_OTG_FS_ID_GPIO_Port GPIOA
#define USB_OVCR_Pin GPIO_PIN_6
#define USB_OVCR_GPIO_Port GPIOC
#define USB_PPWR_EN_Pin GPIO_PIN_7
#define USB_PPWR_EN_GPIO_Port GPIOG
#define SDMMC1_DETECT_Pin GPIO_PIN_10
#define SDMMC1_DETECT_GPIO_Port GPIOF
#define SD_PWR_Pin GPIO_PIN_5
#define SD_PWR_GPIO_Port GPIOC
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
