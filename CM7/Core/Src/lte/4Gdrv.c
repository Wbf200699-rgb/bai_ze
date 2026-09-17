#include "stdio.h"
#include "string.h"
#include "main.h"

#define EC20_USE_DMA

uint8_t USART_RX_BUF_G[512];
uint8_t USART_PC_BUF_G[336];
uint8_t USART_TX_BUF_G[64];

volatile uint8_t usart_rx_flag;
volatile uint8_t usart_rx_len;
volatile uint8_t ec20_reboot_num = 0;

const char hex_table[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

extern UART_HandleTypeDef huart6;

void delay_us(uint32_t us)
{
    __asm volatile ("1: subs %0, #1\n" "bne 1b\n" : "=r" (us) : "0" (us) );
}

void power_off_on()
{
	HAL_GPIO_WritePin(LTE_POW_GPIO_Port, LTE_POW_Pin, 0);
	delay_us(SystemCoreClock/3);
	HAL_GPIO_WritePin(LTE_POW_GPIO_Port, LTE_POW_Pin, 1);
	delay_us(SystemCoreClock/3);
	HAL_GPIO_WritePin(LTE_POW_GPIO_Port, LTE_POW_Pin, 1);
}

#ifdef EC20_USE_DMA
#pragma GCC optimize ("O0")
int ec20_send_cmd(char *cmd, uint8_t code, uint16_t times)
{
  uint8_t  n = strlen((const char *)cmd);
  uint32_t t = SystemCoreClock/16;
  uint16_t m = 0;
  uint32_t i;

  memset(USART_TX_BUF_G,0,sizeof(USART_TX_BUF_G));
  memcpy((uint8_t*)USART_TX_BUF_G,(uint8_t*)cmd,n);

  while(1)
  {
	usart_rx_flag = 0;
	if(m++%3==0) HAL_UART_Transmit_DMA(&huart6,(uint8_t*)USART_TX_BUF_G,n);
	i = 0;
	while(usart_rx_flag == 0)//Wait for IT or timeout,this can used in system && no system
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
	   if(m > 3*times) return times;
	}

	if(m > 15) //Modules do not connect 4G base stations in very very poor signal conditions, power off and on
	{
		if(!memcmp(cmd,"AT+QIOPEN",9))
		{
			power_off_on();
			ec20_reboot_num++;
		}
		return -1;
	}
  }
}
#else
int ec20_send_cmd(char *cmd, char *ack, uint16_t waittime)//not fit cache mode,why?
{
  char    USART_Buff[64];    //UART TX BUF
  uint8_t USART_RX_BUF[64];

  while(waittime--)
  {
    memset(USART_Buff,0,sizeof(USART_Buff));
    memset(USART_RX_BUF,0,sizeof(USART_RX_BUF));

	uint8_t n=strlen((const char *)cmd);
	memcpy((uint8_t*)USART_Buff,(uint8_t*)cmd,n);
	if(HAL_UART_Transmit(&huart6,(uint8_t*)USART_Buff,n,100) == HAL_OK)
	{
		HAL_UART_Receive(&huart6,USART_RX_BUF,sizeof(USART_RX_BUF),1000);//block 1s*waittime

		if(strstr((char*)USART_RX_BUF, ack))
		{
			return 0;//	break;
		}
	}
  }

  return -1;
}
#endif

static uint8_t i;
void EC20_4G_CONNECT()
{
    char SendBuff[50];

#ifdef EC20_USE_DMA
	i = ec20_send_cmd("ATE0\r\n",1, 0);
	if(i != -1)
	{
		/*Configuration is required once after changing ISP*/
		/*The current operator is China Mobile*/
#if 0
		i = ec20_send_cmd("AT+QICSGP=1,1,\"CMIOT\",\"\",\"\",0\r\n",1, 0);//CMNET CMIOT UNINET
		i = ec20_send_cmd("AT+QIACT=1\r\n",  1, 0);
#endif
		i = ec20_send_cmd("AT+QIACT?\r\n",   1, 0);

		i = ec20_send_cmd("AT+QICLOSE=0\r\n",1, 0);

		i = ec20_send_cmd("AT+QICFG=\"tcp/retranscfg\",3,5\r\n", 1, 0);//Net retry 3 times,TCP retransmit interval 50ms
	}

	if(*(uint8_t*)(SYS_CONFIG_IP+0) != 0xff)
		sprintf(SendBuff,"AT+QIOPEN=1,0,\"TCP\",\"%d.%d.%d.%d\",%d%d,0,1\r\n",*(uint8_t*)(SYS_CONFIG_IP+0),
			*(uint8_t*)(SYS_CONFIG_IP+1),*(uint8_t*)(SYS_CONFIG_IP+2),*(uint8_t*)(SYS_CONFIG_IP+3),
			*(uint8_t*)(SYS_CONFIG_PORT+0), *(uint8_t*)(SYS_CONFIG_PORT+1));
	else
		sprintf(SendBuff,"AT+QIOPEN=1,0,\"TCP\",\"%d.%d.%d.%d\",%d,0,1\r\n", 59, 110, 39, 58, 8080);
	i = ec20_send_cmd(SendBuff, 2, 0);
#else
	i = ec20_send_cmd("ATE0\r\n", "OK", 30);
/*  //no usefull
	ec20_send_cmd("AT+CPIN?\r\n", "OK", 2);
	ec20_send_cmd("AT+CREG?\r\n", "OK", 2);
	ec20_send_cmd("AT+CGREG?\r\n", "OK",2);
	ec20_send_cmd("AT+CGATT?\r\n", "+CGATT: 1",5);
*/
    i = ec20_send_cmd("AT+QIACT?\r\n", "OK", 10);

	sprintf(SendBuff,"AT+QIOPEN=1,0,\"TCP\",\"%d.%d.%d.%d\",%d,0,2\r\n",*(uint8_t*)(SYS_CONFIG_IP+0),
			*(uint8_t*)(SYS_CONFIG_IP+1),*(uint8_t*)(SYS_CONFIG_IP+2),*(uint8_t*)(SYS_CONFIG_IP+3),
			*(uint16_t*)(SYS_CONFIG_PORT));
	i = ec20_send_cmd(SendBuff, "CONNECT", 10);
#endif
}

uint8_t error = 0;  //for reconnect net counter
void EC20_SEND_DATA(uint8_t* buffer, uint8_t len)//AT+QISENDEX LESS THEN 256B
{
	static uint8_t buf[2048];
	/*Used for network condition detection during the data transmission phase
	 * if the network transmission fails
	 * usart_rx_flag != 1
	 * the network reconnects*/
	if( usart_rx_flag )
	{
		memcpy(buf,"AT+QISENDEX=0,\"",15);
		for(int m=0; m < len; m++)
		{
			buf[15 + 2 * m]     =  hex_table[(buffer[m] >> 4) & 0x0f];
			buf[15 + 2 * m + 1] =  hex_table[(buffer[m] >> 0) & 0x0f];
		}
		memcpy(&buf[15 + len * 2],"\"\r\n",3);
		HAL_UART_Transmit_DMA(&huart6, buf, 18 + len * 2);
	}
	else//reconnect net
	{
		if(++error > 10) {
			EC20_4G_CONNECT();
			HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, 1);
		}
	}
}

void EC20_SEND_DATAEX(uint8_t* buffer, uint16_t len)//AT+QISEND MORE THEN 256B, LESS THEN 1024B
{
	uint8_t buf[20];
	/*Used for network condition detection during the data transmission phase
	 * if the network transmission fails
	 * usart_rx_flag != 1
	 * the network reconnects*/
	if(usart_rx_flag == 1 || usart_rx_flag == 2) {
//		buf[sprintf(buf, "AT+QISEND=0,%d\r\n", len)] = 0;
//		HAL_UART_Transmit_DMA(&huart6, buf, strlen(buf));
		if(len == 480)       HAL_UART_Transmit_DMA(&huart6, "AT+QISEND=0,480\r\n", sizeof(buf));
		else if(len == 188)  HAL_UART_Transmit_DMA(&huart6, "AT+QISEND=0,188\r\n", sizeof(buf));
		else if(len == 226)  HAL_UART_Transmit_DMA(&huart6, "AT+QISEND=0,226\r\n", sizeof(buf));
		usart_rx_flag = 0xff;
	}
	else if(usart_rx_flag == 3) {
		error = 0;
		HAL_UART_Transmit_DMA(&huart6, buffer, len);
		usart_rx_flag = 0xff;
	}
	else//reconnect net
	{
		if(++error >10)
			EC20_4G_CONNECT();
		else if(usart_rx_flag == 0xff)
			HAL_UART_Transmit_DMA(&huart6, buffer, len);
	}
}
