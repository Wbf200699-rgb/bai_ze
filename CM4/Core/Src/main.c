/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdarg.h>
#include "instance.h"
#include "deca_regs.h"
#include "deca_device_api.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define MEASURE_TYPE      0  //0 - intence mode //1 - Functional  Testing
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
extern int uwbLib_start();
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint32_t  DEV_ROLE;
uint64_t  poll_rx_ts = 0;

static dwt_config_t config = {
		5,               // Channel number//5 - 6239.6 ~ 6739.6 MHz , 9 - 7737.2 ~ 8237.2 MHz
		DWT_PLEN_256,    // Preamble length. Used in TX only
		DWT_PAC8,        // Preamble acquisition chunk size. Used in RX only
		9,               // TX preamble code. Used in TX only
		9,               // RX preamble code. Used in RX only
		1,               // 0 to use standard 8 symbol SFD, 1 to use non-standard 8 symbol, 2 for non-standard 16 symbol SFD and 3 for 4z 8 symbol SDF type
		DWT_BR_6M8,      // Data rate
		DWT_PHRMODE_EXT, // PHY header mode
		DWT_PHRRATE_STD, // PHY header rate
		(257 + 8 - 8),   // SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only
		(DWT_STS_MODE_1 | DWT_STS_MODE_SDC), // STS enabled
		DWT_STS_LEN_256, // STS length see allowed values in Enum dwt_sts_lengths_e
		DWT_PDOA_M3      // PDOA mode 3
};
static dwt_txconfig_t txconfig_options = {
	    0x34,                // PG delay
	    0xfffffcff,          // TX power
	    0x0                  // PG count
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
#if 1
  dwt_isr();
#else
  while(port_CheckEXT_IRQ() != 0)
  {
    dwt_isr();
  }
#endif
}

void uart_printf(char* fmt,...)
{
	uint8_t UserTxBuffer[256];
	va_list ap;
	va_start(ap,fmt);
	vsprintf((char*)UserTxBuffer,fmt,ap);
	va_end(ap);
	HAL_UART_Transmit(&huart7,UserTxBuffer,strlen((const char*)UserTxBuffer),10);
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

/* USER CODE BEGIN Boot_Mode_Sequence_1 */
  /*HW semaphore Clock enable*/
  __HAL_RCC_HSEM_CLK_ENABLE();
  /* Activate HSEM notification for Cortex-M4*/
  HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));
  /*
  Domain D2 goes to STOP mode (Cortex-M4 in deep-sleep) waiting for Cortex-M7 to
  perform system initialization (system clock config, external memory configuration.. )
  */
  HAL_PWREx_ClearPendingEvent();
  HAL_PWREx_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFE, PWR_D2_DOMAIN);
  /* Clear HSEM flag */
  __HAL_HSEM_CLEAR_FLAG(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));

/* USER CODE END Boot_Mode_Sequence_1 */
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI6_Init();
  MX_UART7_Init();
  /* USER CODE BEGIN 2 */
  uart_printf("************This uwb3 soft build on %s,%s************\r\n",__DATE__,__TIME__);

  /* Array initialization */
  for(int i = 0; i < 360; i++)
  {
	cal_angles[i] = (double)((int16_t)__REV16(*(uint16_t *)(UWB_CONFIG_CAL + i * 2))) / 100;
	cal_values[i] = (double)((int16_t)__REV16(*(uint16_t *)(UWB_CONFIG_CAL + 736 + i * 2))) / 100;
  }
  kf_twr_init(0.1f);
  /* UWB initialization */
  inst= &instance;
  port_set_dw_ic_spi_fastrate();
  reset_DWIC();
  while (!dwt_checkidlerc());
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  if (dwt_initialise(DWT_DW_IDLE) != DWT_SUCCESS)
  {
    return -1;
  }

  dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);
  dwt_setlnapamode(DWT_PA_ENABLE);
  dwt_setfinegraintxseq(0);

  if(dwt_configure(&config))
  {
  	return -1;
  }

  dwt_setrxantennadelay(RX_ANT_DLY);
  dwt_settxantennadelay(TX_ANT_DLY);
  dwt_configuretxrf(&txconfig_options);

#if defined(DS_TWR_AOA_YW)
  dwt_setrxtimeout(0xFFFFF);
  dwt_setcallbacks(tx_done_cb, rx_ok_cb, rx_to_cb, rx_err_cb, NULL, NULL);
  dwt_setinterrupt(SYS_ENABLE_LO_TXFRS_ENABLE_BIT_MASK | SYS_ENABLE_LO_RXFCG_ENABLE_BIT_MASK | SYS_STATUS_RXFTO_BIT_MASK | SYS_STATUS_RXPTO_BIT_MASK | SYS_STATUS_RXPHE_BIT_MASK | SYS_STATUS_RXFCE_BIT_MASK | SYS_STATUS_RXFSL_BIT_MASK | SYS_STATUS_RXSTO_BIT_MASK | SYS_STATUS_ARFE_BIT_MASK | SYS_STATUS_CIAERR_BIT_MASK, 0, DWT_ENABLE_INT);
#else
//dwt_setpreambledetecttimeout(0);
//dwt_setrxaftertxdelay(200);
  dwt_setrxtimeout(0);

  dwt_callbacks_s cbs = {NULL};
  cbs.cbRxOk          = rx_ok_cb;
  cbs.cbRxTo          = rx_err_cb;
  cbs.cbRxErr         = rx_err_cb;
  cbs.cbTxDone        = tx_done_cb;
//dwt_setcallbacks(&cbs);
//dwt_setinterrupt(DWT_INT_RXFCG_BIT_MASK | DWT_INT_RXFTO_BIT_MASK |SYS_STATUS_ALL_RX_ERR | DWT_INT_TXFRS_BIT_MASK, 0, DWT_ENABLE_INT);
#endif
  dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RCINIT_BIT_MASK | SYS_STATUS_SPIRDY_BIT_MASK);
  HAL_GPIO_WritePin(VC1_GPIO_Port, VC1_Pin, 1);
  HAL_GPIO_WritePin(VC2_GPIO_Port, VC2_Pin, 0);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  HAL_HSEM_FastTake(1);
  memset((void*)SHD_RAM_ADDR, 0, sizeof(UwbFrame));//Max length for Anchor and Tag
  HAL_HSEM_Release(1,0);
#if defined(DS_TWR_AOA_YW)
  inst->shortAdd_idx = *(uint8_t*)(SYS_CONFIG_ID);
  uart_printf(" IS ANC %d#\r\n",inst->shortAdd_idx);
  dwt_rxenable(DWT_START_RX_IMMEDIATE);
  anch_range_loop();
#else
#if 1
  dwt_setgpiovalue(GPIO_4, 0x0);
  dwt_setgpiovalue(GPIO_6, 0x0);
  dwt_setgpiodir(0xFFAF);
  dwt_rxenable(DWT_START_RX_IMMEDIATE);
  while(1)
  {
	HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin);
	HAL_Delay(2000);
  }
#else
  while(1)
  {
	if(uwbLib_start() == 0) HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin);
	HAL_Delay(100);
  }
#endif
#endif
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
