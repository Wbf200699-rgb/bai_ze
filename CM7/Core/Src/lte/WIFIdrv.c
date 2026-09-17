#include "stdio.h"
#include "string.h"
#include "main.h"

extern uint8_t USART_RX_BUF_G[512];
extern uint8_t USART_PC_BUF_G[336];
extern uint8_t USART_TX_BUF_G[64];
extern uint8_t usart_rx_flag;
extern uint8_t usart_rx_len;
extern UART_HandleTypeDef huart3;
volatile uint8_t e103_reboot_num = 0;


static void delay_us(uint32_t us)
{
    __asm volatile ("1: subs %0, #1\n" "bne 1b\n" : "=r" (us) : "0" (us) );
}

void e103_mesh_power()
{
	HAL_GPIO_WritePin(WIFI_POW_GPIO_Port, WIFI_POW_Pin, 0);
	delay_us(SystemCoreClock/3);
	HAL_GPIO_WritePin(WIFI_POW_GPIO_Port, WIFI_POW_Pin, 1);
	delay_us(SystemCoreClock/3);
	HAL_GPIO_WritePin(WIFI_POW_GPIO_Port, WIFI_POW_Pin, 1);
}

#pragma GCC optimize ("O0")
int e103_send_cmd(char *cmd, uint8_t code, uint16_t times)
{
  uint8_t  n = strlen((const char *)cmd);
  uint32_t t = SystemCoreClock/16;
  uint16_t m = 0;
  uint32_t i = 0;

  memset(USART_TX_BUF_G,0,sizeof(USART_TX_BUF_G));
  memcpy((uint8_t*)USART_TX_BUF_G,(uint8_t*)cmd,n);

  while(1)
  {
	usart_rx_flag = 0; i = 0;
	HAL_UART_Transmit_DMA(&huart3,(uint8_t*)USART_TX_BUF_G,n);
	while(usart_rx_flag == 0)//Wait for interrupt or timeout,this can used in system and no system
	{
	    if(++i > t) break;
	}
	if(i > t) continue;

	if(code == usart_rx_flag)//Git the right ack
	{
		return 0;
	}

	if(times != 0) //no continue try mode
	{
	   if(m > 3 * times) return times;
	}
  }
}

static uint8_t i;
void E103_MESH_CONNECT()
{
	i = e103_send_cmd("AT\r\n",1, 0);
	if(i != -1)
	{
		i = e103_send_cmd("AT+METYPE=2\r\n",1, 0);
//		i = e103_send_cmd("AT+MECHANNEL=13,1\r\n",1, 0);
//		i = e103_send_cmd("AT+MECAPACITY=2,5,30\r\n",1, 0);
//		i = e103_send_cmd("AT+MESTART\r\n",1, 0);
		i = e103_send_cmd("AT+MESTATUS?\r\n",2, 0);
	}
}

void E103_SEND_DATAEX(uint8_t* buffer, uint16_t len)
{
//	uint8_t cmd[17];
	if(usart_rx_flag == 1 || usart_rx_flag == 2) {
		if(len == 480) HAL_UART_Transmit_DMA(&huart3, "AT+MESEND=\"\",480\r\n", 18);
		else if(len == 184) HAL_UART_Transmit_DMA(&huart3, "AT+MESEND=\"\",184\r\n", 18);
//		cmd[sprintf(cmd, "AT+MESEND=\"\",%d\r\n", len)] = 0;
//		HAL_UART_Transmit_DMA(&huart3, cmd, strlen(cmd));
	} else if(usart_rx_flag == 3)
	{
	    HAL_UART_Transmit_DMA(&huart3,buffer, len);
	    if(e103_send_cmd("AT+MESTATUS?\r\n",2, 1) != 0) e103_send_cmd("AT+MESTART\r\n",1, 1);
	}
}
