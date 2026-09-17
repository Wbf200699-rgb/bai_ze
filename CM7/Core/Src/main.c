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
#include "cmsis_os.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdlib.h"
#include "string.h"
#include "./flash/flash_if.h"
#include "./ringbuf/ringbuffer.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

#define SKP_FRAME_LEN        8
#define MTI_FRAME_LEN       76
#define UWB_FRAME_LEN      544 //ANC
#define UWB_FRAME_LEN_T    455 //TAG
#define UART_MAX_LEN       550 //HY
#define USE_MULTI_CORE_SHARED_CODE 1

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
uint8_t bufferG[2048];
uint8_t buf[sizeof(bufferG)];

struct rt_ringbuffer ring_buf;

OutFrame frame;
OutFrame_T frame_t;

uint8_t buffer1[UART_MAX_LEN];
uint16_t rxLen1;
uint8_t buffer3[UART_MAX_LEN];
uint16_t rxLen3;
uint8_t buffer4[UART_MAX_LEN];
uint16_t rxLen4;
uint8_t buffer6[UART_MAX_LEN*3];
uint16_t rxLen6;

uint8_t frame_num = 0;
uint8_t dev_has_skp = 0;
volatile uint8_t  system_init_flag =1;//Power loss during MTI630 configuration will result in loss of firmware

#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
extern uint8_t  error;
extern uint8_t  dev_id;
extern uint8_t  dev_role;
extern uint8_t  usart_rx_flag;
extern uint8_t  ec20_reboot_num;

extern uint32_t g_Pressure;
extern uint32_t g_MeasureFreq;
extern uint16_t g_SkpAltitude;
extern int16_t  g_EulerAngles[3];//X,Y,Z
extern int16_t  g_Acceleration[3];//X,Y,Z
extern int16_t  g_MagneticField[3];//X,Y,Z

extern void EC20_4G_CONNECT();
extern void E103_MESH_CONNECT();
extern void EC20_SEND_DATA(uint8_t* buffer, uint8_t len);
extern void EC20_SEND_DATAEX(uint8_t* buffer, uint16_t len);
extern void E103_SEND_DATAEX(uint8_t* buffer, uint16_t len);
extern void deal_mti_data(uint8_t* buf, uint8_t* mask);
extern int  deal_sk_data (uint8_t* buf, uint8_t* mask);
extern void deal_uwb_data(uint8_t* buf, OutFrame * UwbOut, uint8_t* mask);
extern void deal_uwb_data_t(uint8_t* buf, OutFrame_T* UwbOut, uint8_t * mask);
extern void send_sk_data(uint8_t cmd,uint32_t value);
extern void init_skp_dev();
extern void init_mti_dev();
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void UART_DMA_START()
{
  HAL_UART_Receive_DMA(&huart1, buffer1, sizeof(buffer1));  //MTi630
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);

  HAL_UART_Receive_DMA(&huart3, buffer3, sizeof(buffer3));  //MESH
  __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);

  HAL_UART_Receive_DMA(&huart4, buffer4, sizeof(buffer4));  //SKP30
  __HAL_UART_ENABLE_IT(&huart4, UART_IT_IDLE);

  HAL_UART_Receive_DMA(&huart6, buffer6, sizeof(buffer6));  //EC20
  __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);
}

void UART_RxIdle()
{
  if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET)
  {
	  __HAL_UART_CLEAR_IDLEFLAG(&huart1);
	  HAL_UART_DMAStop(&huart1);
	  rxLen1 = sizeof(buffer1) - __HAL_DMA_GET_COUNTER(huart1.hdmarx);
      if(rxLen1 == MTI_FRAME_LEN)
      {
//		deal_mti_data(buffer1, rxLen1);
    	rt_ringbuffer_put_force(&ring_buf, buffer1, rxLen1);
      }
	  memset(buffer1, 0, sizeof(buffer1));
	  HAL_UART_Receive_DMA(&huart1, buffer1, sizeof(buffer1));
  }
  else if(__HAL_UART_GET_FLAG(&huart3, UART_FLAG_IDLE) != RESET)
  {
	  __HAL_UART_CLEAR_IDLEFLAG(&huart3);
	  HAL_UART_DMAStop(&huart3);
	  rxLen3 = sizeof(buffer3) - __HAL_DMA_GET_COUNTER(huart3.hdmarx);
	  if(strstr((char *)buffer3, "+OK\r\n>")) //for mesh send data
	  {
	      usart_rx_flag = 3;
	  }
	  else if(strstr((char *)buffer3, "+OK")) //for which AT cmd returns
	  {
	      usart_rx_flag = 1;
	  }
	  else if(strstr((char *)buffer3, "+MESTATUS:2,1")) //for mesh connect success
	  {
	  	  usart_rx_flag = 2;
	  }
	  memset(buffer3, 0, sizeof(buffer3));
	  HAL_UART_Receive_DMA(&huart3, buffer3, sizeof(buffer3));
  }
  else if(__HAL_UART_GET_FLAG(&huart4, UART_FLAG_IDLE) != RESET)
  {
  	  __HAL_UART_CLEAR_IDLEFLAG(&huart4);
  	  HAL_UART_DMAStop(&huart4);
  	  rxLen4 = sizeof(buffer4) - __HAL_DMA_GET_COUNTER(huart4.hdmarx);
      if(rxLen4 == SKP_FRAME_LEN)
      {
//		deal_sk_data(buffer4);
        rt_ringbuffer_put_force(&ring_buf, buffer4, rxLen4);
      }
  	  memset(buffer4, 0, sizeof(buffer4));
  	  HAL_UART_Receive_DMA(&huart4, buffer4, sizeof(buffer4));
    }
  else if(__HAL_UART_GET_FLAG(&huart6, UART_FLAG_IDLE) != RESET)
  {
	  __HAL_UART_CLEAR_IDLEFLAG(&huart6);
	  HAL_UART_DMAStop(&huart6);
	  rxLen6 = sizeof(buffer6) - __HAL_DMA_GET_COUNTER(huart6.hdmarx);
	  if(strstr((char *)buffer6, "+QIURC:")) //processing commands received by 4G
	  {
		  if(buffer6[rxLen6-2]==0x0D && buffer6[rxLen6-1]==0x0A)
		  {
			  int i;
			  for(i=rxLen6-6; i > 7; i--) //from tail,skip /r/n id /r/n, 6Byte
			  {
				  if(buffer6[i-1] == 0x0D && buffer6[i] == 0x0A) break; //findadd aliyun  down-forwarding header pc/r/n
			  }

			  static char buf[407]={'M','C',0x0d,0x0a};uint16_t s=4; //add aliyun up-forwarding header

			  switch(buffer6[++i]) //from i(23) to rxLen6-4 is the real data
			  {
	  	  	  	  case '#': //Write to Flash //pc/r/n#CFG,id,port_high,port_low,ip,out_sel,debug,ID, ID added for select right device
	  	  	  		  if(buffer6[i+1]=='C' && buffer6[i+2]=='F' && buffer6[i+3] == 'G' && \
	  	  	  			((dev_role == 0x0 && atoi((char*)&buffer6[rxLen6-4]) == dev_id) || dev_role ==0xff || \
	  	  	  			(dev_role == 0x1 && (atoi((char*)&buffer6[rxLen6-4]) == *(uint8_t*)(SYS_CONFIG_ID) || dev_id == 0xff))))//ANC || TAG || NEW TAG
	  	  	  		  {
	  	  	  			  uint8_t dat[32] = {0xff}; //once write 32B
	  	  	  			  uint8_t k = 0;
	  	  	  			  memset(dat, 0xff,sizeof(dat));
	  	  	  			  dat[16] = atoi((char*)&buffer6[i+5]); //get first section and align the last 16 bytes
	  	  	  			  for(int j = i + 6; j < (rxLen6 - 5); j++)
	  	  	  			  {
	  	  	  				  if(buffer6[j] == ',') *(uint8_t *)(dat + 16 + (++k)) = atoi((char*)&buffer6[j + 1]);
	  	  	  			  }
	  	  	  			  FLASH_If_Erase(SYS_CONFIG_ADDR);
	  	  	  			  FLASH_If_Write(SYS_CONFIG_START, (uint32_t *)dat, sizeof(dat)/sizeof(uint32_t));

	  	  	  			  s += sprintf(&buf[4], "EC20 Configure write OK,ID=%u(%u),IP=%u.%u.%u.%u:%d,222222222222222222222",dev_id,*(uint8_t*)(SYS_CONFIG_ID),\
	  	  	  					  *(uint8_t*)(SYS_CONFIG_IP+0),*(uint8_t*)(SYS_CONFIG_IP+1),*(uint8_t*)(SYS_CONFIG_IP+2),*(uint8_t*)(SYS_CONFIG_IP+3),\
								  *(uint16_t*)(SYS_CONFIG_PORT));
	  	  	  			  HAL_NVIC_SystemReset();
	  	  	  		  }
	  	  	  		  break;
	  	  	  	  case '$': //CMD to UWBD //pc/r/n$CFG .........................../r/nID, ID added for select right device
	  	  	  		  if(buffer6[i+1]=='C' && buffer6[i+2]=='F' && buffer6[i+3] == 'G' && \
	  	  	  			((dev_role == 0x0 && atoi((char*)&buffer6[rxLen6-4]) == dev_id) || \
	  	  	  			(dev_role == 0x1 && atoi((char*)&buffer6[rxLen6-4]) == *(uint8_t*)(SYS_CONFIG_ID))))//ANC || TAG
	  	  	  		  {
//	  	  	  			  HAL_UART_Transmit(&huart3, &buffer6[i], rxLen6-i-4,100); //$CFG .................../r/n                          //for HY UWB
	  	  	  			  uint32_t result = 1;
	  	  	  			  if(buffer6[i+5] == 'U') {
			  	  	  		  uint8_t dat[32]; //once write 32B
		  	  	  			  memset(dat, 0xff,sizeof(dat));
		  	  	  			  memcpy(dat + 16, &buffer6[i + 7], 16);
		  	  	  			  if(*(uint8_t*)UWB_CONFIG_FILL3 == 0xff) result = FLASH_If_Write(UWB_CONFIG_SOLT, (uint32_t *)dat, sizeof(dat)/sizeof(uint32_t));
	  	  	  			  }
	  	  	  			  else if (buffer6[i+5] == '+') {
	  	  	  				  if(*(uint8_t*)UWB_CONFIG_FILL1 == 0xff)
	  	  	  					  result = FLASH_If_Write(UWB_CONFIG_CAL, (uint32_t *)&buffer6[i + 7], (360 * 2 + 16)/sizeof(uint32_t));//write 360°, every 2B, fill 16B for 32B align
	  	  	  			  }
	  	  	  			  else if (buffer6[i+5] == '-') {
	  	  	  				  if(*(uint8_t*)UWB_CONFIG_FILL2 == 0xff)
	  	  	  					  result = FLASH_If_Write(UWB_CONFIG_CAL + 360 * 2 + 16, (uint32_t *)&buffer6[i + 7], (360 * 2 + 16)/sizeof(uint32_t));//write (180*4 -16)B for 32B align
	  	  	  			  }
	  	  	  			  if (result == 0) s += sprintf(&buf[4], "UWB ID: %u(%u) CONFG OK,1111111111111111111111111111111111111111111111111", dev_id, buffer6[i+5]);
	  	  	  			  else             s += sprintf(&buf[4], "UWB ID: %u(%u) CONFG ERROR,xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", dev_id, buffer6[i+5]);
	  	  	  		  }
	  	  	  		  break;
	  	  	  	  case '%': //CMD to MTi630 //pc/r/n%FA FF 0E 00 F3/r/nID, ID added for select right device
	  	  	  		  if(buffer6[i+1]== 'F' && buffer6[i+2]== 'A' && \
	  	  	  	  		((dev_role == 0x0 && atoi((char*)&buffer6[rxLen6-4]) == dev_id) || \
	  	  	  	  		(dev_role == 0x1 && atoi((char*)&buffer6[rxLen6-4]) == *(uint8_t*)(SYS_CONFIG_ID))))//ANC || TAG
	  	  	  		  {
	  	  	  			  uint8_t buff[200];
	  	  	  			  for(int k=0; k < (rxLen6-(i+1)-6+1)/3; k++)
	  	  	  			  {
	  	  	  			      buff[k] = 16*(buffer6[(i+1)+3*k+0] >= 'A' ? buffer6[(i+1)+3*k+0] - 'A' + 10 : buffer6[(i+1)+3*k+0] - '0') \
	  	  	  				             + (buffer6[(i+1)+3*k+1] >= 'A' ? buffer6[(i+1)+3*k+1] - 'A' + 10 : buffer6[(i+1)+3*k+1] - '0'); //FA -> 0xFA
	  	  	  			  }
	  	  	  			  HAL_UART_Transmit(&huart1, buff, (rxLen6-(i+1)-6+1)/3, 100); //FA FF 0E 00 F3
	  	  	  			  s += sprintf(&buf[4], "IMU ID: %u(%u) CONFG OK,3333333333333333333333333333333333333333333333333", dev_id, *(uint8_t*)(SYS_CONFIG_ID));
	  	  	  		  }
	  	  	  		  break;
	  	  	  	  case 'F': //SKP20 frequency //0-9  //pc/r/nF 4/r/nFF
	  	  	  		  if((buffer6[rxLen6-7] - '0') < 10) //ASCII TO INT,simple deal 1-10;
	  	  	  		  {
	  	  	  			  g_MeasureFreq = buffer6[rxLen6-7]-'0'+1;
	  	  	  			  send_sk_data(0x03, g_MeasureFreq);
	  	  	  			  s += sprintf(&buf[4], "SKP30 Frequency %d OK,44444444444444444444444444444444444444444444444", (int)g_MeasureFreq);
	  	  	  		  }
	  	  	  		  break;
	  	  	  	  case 'M': //MTi630 ON/OFF //0-1  //pc/r/nM 1/r/nFF
	  	  	  		  if((buffer6[rxLen6-7] - '0') == 0) //ASCII TO INT,simple deal 1-10;
	  	  	  			  HAL_GPIO_WritePin(IMU_POW_GPIO_Port, IMU_POW_Pin, 0);
					  else
						  HAL_GPIO_WritePin(IMU_POW_GPIO_Port, IMU_POW_Pin, 1);
	  	  	  		  break;
	  	  	  	  case 'P': //Device Power Off //pc/r/nP 0/r/nFF
	  	  	  		  if((buffer6[rxLen6-7] - '0') == 0)
	  	  	  		      HAL_GPIO_WritePin(PWR_ON_GPIO_Port, PWR_ON_Pin, 0);
	  	  	  		  else if(dev_role == 0x1 && (buffer6[rxLen6-7] - '0') == dev_id)
	  	  	  			  HAL_NVIC_SystemReset();
	  	  	  		  break;
	  	  	  	  default:
	  	  	  	  	  break;
			  }

			  if(s > 4) EC20_SEND_DATA((uint8_t *)buf, s); //MC\r\n
		      HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, 0);
		  }
	  }
	  else if(strstr((char *)buffer6, "OK")) //for which AT cmd return "OK" and QISSEND(EX) response "SEND OK"
	  {
	      usart_rx_flag = 1;
	  }
	  else if(strstr((char *)buffer6, "+QIOPEN: 0,0")) //for AT cmd QIOPEN success
	  {
	  	  usart_rx_flag = 2;
	  }
	  else if(strstr((char *)buffer6, ">")) //for QISSEND response ">"
	  {
	      usart_rx_flag = 3;
	      if(*(uint8_t *)SYS_CONFIG_MODE == 0)      EC20_SEND_DATAEX(&frame.head[0], sizeof(frame));
	      else if(*(uint8_t *)SYS_CONFIG_MODE == 1) EC20_SEND_DATAEX(&frame_t.head[0], sizeof(frame_t));
	  }
	  else
	  {
		  usart_rx_flag = 0;//for net reconnect
	  }
	  memset(buffer6, 0, sizeof(buffer6));
	  HAL_UART_Receive_DMA(&huart6, buffer6, sizeof(buffer6));
  }
}

void frame_transfer()
{
	__disable_irq();
	if(HAL_HSEM_FastTake(1) == HAL_OK)
	{
		rt_ringbuffer_put_force(&ring_buf, (void *)SHD_RAM_ADDR, SHD_RAM_LEN);
		HAL_HSEM_Release(1, 0);
	}
	uint16_t len = rt_ringbuffer_get(&ring_buf, buf, sizeof(buf));
	__enable_irq();

	if(len >= UWB_FRAME_LEN)
	{
		uint8_t mask = 0, m = 1; int i = 0;
		do
		{
			for( mask =0 ; i < len -1; i++)
			{
				if(buf[i] == 0xaa && buf[i+1] == 0x55) //ANC
				{
					memset(&frame.head, 0, sizeof(frame));
					deal_uwb_data(&buf[i], &frame, &mask);
					if(mask & 0x01)  i += (UWB_FRAME_LEN-1);
				}
				else if(buf[i] == 'M' && buf[i+1] == 'A') //TAG
				{
					memset(&frame_t.head, 0, sizeof(frame_t));
					deal_uwb_data_t(&buf[i], &frame_t, &mask);
					if(mask & 0x01)  i += (UWB_FRAME_LEN_T-1);
				}
				else if(buf[i] == 0x55 && buf[i+1] == 0x07) //SKP30
				{
					deal_sk_data(&buf[i], &mask);
					if(mask & 0x02)  i += (SKP_FRAME_LEN-1);
					if(dev_has_skp == 0) dev_has_skp = 1;
				}
				else if(buf[i] == 0xfa && buf[i+1] == 0xff) //MTi630
				{
					deal_mti_data(&buf[i], &mask);
					if(mask & 0x04)  i += (MTI_FRAME_LEN-1);
				}

				if (i > (len - UWB_FRAME_LEN)) m = 0;

				if((dev_role == 0 && ((dev_has_skp == 0 && mask == 5) || mask == 7)) || i > (len-SKP_FRAME_LEN/2)) break;
				if((dev_role == 1 && mask == 5) || i > (len-SKP_FRAME_LEN/2)) break;//TAG
			}

			if(dev_role == 0 && (mask == *(uint8_t *)SYS_CONFIG_MASK || mask ==7) && (dev_id >= 0 && dev_id < 21)) //ANC
			{
				memcpy(frame.head, "MC\r\n", 4);
				memcpy(frame.tail, "CM\r\n", 4);

				frame.fsn[0] = frame_num++ % 32;
				frame.fsn[1] = ec20_reboot_num;
				frame.alt    = g_SkpAltitude;
				frame.roll   = g_EulerAngles[0];
				frame.pitch  = g_EulerAngles[1];
				frame.yaw    = g_EulerAngles[2];
				frame.accx   = g_Acceleration[0];
				frame.accy   = g_Acceleration[1];
				frame.accz   = g_Acceleration[2];
				frame.magx   = g_MagneticField[0];
				frame.magy   = g_MagneticField[1];
				frame.magz   = g_MagneticField[2];
				frame.press  = g_Pressure;

				for(int j=0; j<(sizeof(OutFrame)-9);j++) //not contain head tail crc
				{
					frame.sum += *(uint8_t *)(&frame.id+j);
				}

				if(*(uint8_t*)SYS_CONFIG_OUTPUT == 0x10)
					E103_SEND_DATAEX(&frame.head[0], sizeof(frame));
				else
					EC20_SEND_DATAEX(&frame.head[0], sizeof(frame));//EC20_SEND_DATA(&frame.head[0], sizeof(frame));
				HAL_GPIO_TogglePin(LED_0_GPIO_Port, LED_0_Pin);
			}
			else if(dev_role == 1 && (mask == 1 || mask ==5)) //TAG
			{
				memcpy(frame_t.head, "MR\r\n", 4);
				memcpy(frame_t.tail, "RM\r\n", 4);

				frame.fsn[0] = frame_num++ % 32;
				frame_t.fsn[1] = ec20_reboot_num;
				frame_t.roll   = g_EulerAngles[0];
				frame_t.pitch  = g_EulerAngles[1];
				frame_t.yaw    = g_EulerAngles[2];
				frame_t.accx   = g_Acceleration[0];
				frame_t.accy   = g_Acceleration[1];
				frame_t.accz   = g_Acceleration[2];
				frame_t.magx   = g_MagneticField[0];
				frame_t.magy   = g_MagneticField[1];
				frame_t.magz   = g_MagneticField[2];
				frame_t.press  = g_Pressure;

				for(int j=0; j<(sizeof(OutFrame_T)-9); j++) //not contain head tail crc
				{
					frame_t.sum += *(uint8_t *)(&frame_t.id+j);
				}

				if(*(uint8_t*)SYS_CONFIG_OUTPUT == 0x10)
					E103_SEND_DATAEX(&frame_t.head[0], sizeof(frame_t));
				else
					EC20_SEND_DATAEX(&frame_t.head[0], sizeof(frame_t));
				HAL_GPIO_TogglePin(LED_0_GPIO_Port, LED_0_Pin);
			}
		}while(m);
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
/* USER CODE BEGIN Boot_Mode_Sequence_0 */
  int32_t timeout;
/* USER CODE END Boot_Mode_Sequence_0 */

/* USER CODE BEGIN Boot_Mode_Sequence_1 */
  /* Wait until CPU2 boots and enters in stop mode or timeout*/
  timeout = 0xFFFF;
  while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0));
  if ( timeout < 0 )
  {
  Error_Handler();
  }
/* USER CODE END Boot_Mode_Sequence_1 */
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();
/* USER CODE BEGIN Boot_Mode_Sequence_2 */
/* When system initialization is finished, Cortex-M7 will release Cortex-M4 by means of
HSEM notification */
/*HW semaphore Clock enable*/
__HAL_RCC_HSEM_CLK_ENABLE();
/*Take HSEM */
HAL_HSEM_FastTake(HSEM_ID_0);
/*Release HSEM in order to notify the CPU2(CM4)*/
HAL_HSEM_Release(HSEM_ID_0,0);
/* wait until CPU2 wakes up from stop mode */
timeout = 0xFFFF;
while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET) && (timeout-- > 0));
if ( timeout < 0 )
{
Error_Handler();
}
/* USER CODE END Boot_Mode_Sequence_2 */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_DMA_Init();
  MX_USART6_UART_Init();
  MX_USART3_UART_Init();
  MX_UART4_Init();
  /* USER CODE BEGIN 2 */
#if 1//1 - ANCHOR need POWER UP IMU , 0 - TAG power <2w need POWER DOWN IMU
  HAL_GPIO_WritePin(IMU_POW_GPIO_Port, IMU_POW_Pin, 1);
#endif
  for(int i=0; i < 20; i++)
  {
	HAL_GPIO_TogglePin(LED_0_GPIO_Port, LED_0_Pin);
	HAL_Delay(100);
  }
//  usb_printf("************This uwbd soft build on %s,%s************\r\n",__DATE__,__TIME__);
  rt_ringbuffer_init(&ring_buf, bufferG, sizeof(bufferG));
  UART_DMA_START();
  EC20_4G_CONNECT();
//  if(*(uint8_t*)SYS_CONFIG_OUTPUT == 0x10) E103_MESH_CONNECT();

  init_skp_dev();

  system_init_flag = 0;
  init_mti_dev();
  system_init_flag = 1;
#if 0 // IF USE FREE-RTOS
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();
  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

/* Infinite loop */
#endif
  while(1)
  {
	frame_transfer();
    HAL_Delay(100);
    /* USER CODE END WHILE */

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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 20;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
