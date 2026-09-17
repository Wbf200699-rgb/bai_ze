/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define UWB_FRAME_HEADER    0xAA55A55A  //ANCHOR
#define UWB_FRAME_TAIL      0x0D0A0D0A
#define UWB_FRAME_HEADER_T  0x4D410D0A  //TAG
#define UWB_FRAME_TAIL_T    0x414D0D0A

#define SHD_RAM_ADDR        0x38000000 //SHARED MEMERY
#define SHD_RAM_LEN         544

#define SYS_CONFIG_ADDR     0x081E0000
#define UWB_CONFIG_CAL      0x081FFA00
#define UWB_CONFIG_FILL1    0x081FFCDF//write flag
#define UWB_CONFIG_FILL2    0x081FFFBF//write flag
#define UWB_CONFIG_SOLT     0x081FFFC0
#define UWB_CONFIG_FILL3    0x081FFFDF//write flag
#define SYS_CONFIG_START    0x081FFFE0
#define SYS_CONFIG_ID       0x081FFFF0
#define SYS_CONFIG_MODE     0x081FFFF1
#define SYS_CONFIG_PORT     0x081FFFF2
#define SYS_CONFIG_IP       0x081FFFF4
#define SYS_CONFIG_OUTPUT   0x081FFFF8
#define SYS_CONFIG_DEBUG    0x081FFFF9
#define SYS_CONFIG_MASK     0x081FFFFA

#pragma pack (1)
typedef struct
{
	uint8_t   tid;
	uint8_t   los;
	uint64_t  ttime;
	uint64_t  rtime;
	uint16_t  range;
	int16_t   aoa;
	int16_t   elevation;
	int16_t   pdoa1;
	int16_t   pdoa2;
}UwbData;//28

typedef struct
{
	uint32_t head;
	uint16_t fsn;

	UwbData  uwb[19];

	uint8_t  lid;
	uint8_t  xor;
	uint32_t tail;
}UwbFrame;//544

typedef struct
{
	uint32_t head;
	uint8_t  fsn;
	uint8_t  lid;

	int16_t  tdoa[190];
	uint8_t  mask[24];
	int16_t  aoa[20];

	uint8_t  xor;
	uint32_t tail;
}UwbFrame_T;//455

typedef struct
{
	uint16_t   range;
	int16_t    aoa;
	int16_t    pitch;
	int16_t    pdoa1;
	int16_t    pdoa2;
}UwbType;//10

typedef struct
{
	uint8_t   head[4];
	uint8_t   id;
	uint8_t   fsn[2];
	UwbType   uwb[19];
	uint16_t  alt;
	int16_t   roll;
	int16_t   pitch;
	int16_t   yaw;
	int16_t   accx;
	int16_t   accy;
	int16_t   accz;
	int16_t   magx;
	int16_t   magy;
	int16_t   magz;
	uint32_t  press;
	uint8_t   sum;
	uint8_t   tail[4];
}OutFrame;//226

typedef struct
{
	uint8_t   head[4];
	uint8_t   id;
	uint8_t   fsn[2];
	int16_t   tdoa[190];
	uint8_t   mask[24];
	int16_t   aoa[20];
	uint16_t  alt;
	int16_t   roll;
	int16_t   pitch;
	int16_t   yaw;
	int16_t   accx;
	int16_t   accy;
	int16_t   accz;
	int16_t   magx;
	int16_t   magy;
	int16_t   magz;
	uint32_t  press;
	uint8_t   sum;
	uint8_t   tail[4];
}OutFrame_T;//480
#pragma pack ()
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
#define PWR_ON_Pin GPIO_PIN_6
#define PWR_ON_GPIO_Port GPIOE
#define KEY_IN_Pin GPIO_PIN_1
#define KEY_IN_GPIO_Port GPIOC
#define LED_0_Pin GPIO_PIN_0
#define LED_0_GPIO_Port GPIOG
#define LTE_POW_Pin GPIO_PIN_8
#define LTE_POW_GPIO_Port GPIOE
#define WIFI_RST_Pin GPIO_PIN_12
#define WIFI_RST_GPIO_Port GPIOD
#define WIFI_POW_Pin GPIO_PIN_13
#define WIFI_POW_GPIO_Port GPIOD
#define IMU_POW_Pin GPIO_PIN_0
#define IMU_POW_GPIO_Port GPIOE
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
