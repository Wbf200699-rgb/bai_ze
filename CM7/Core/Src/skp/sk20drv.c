#include "stdio.h"
#include "string.h"
#include "main.h"

extern UART_HandleTypeDef huart4;


#define FRAME_HEAD   0x55
#define FRAME_TAIL   0xaa
#define FRAME_DATA   0x07
#define CMD_START    0x05
#define CMD_STOP     0x06
#define CMD_FREQ     0x03


#pragma pack (1)

typedef struct
{
	uint8_t  head;
	uint8_t  key;
	uint32_t value;
	uint8_t  crc;
	uint8_t  tail;
} SkpData;

#pragma pack ( )

uint32_t g_MeasureFreq = 1;
uint16_t g_SkpAltitude;

uint8_t CalCRC(uint8_t* ptr, uint8_t len)
{
	uint8_t i;
	uint8_t crc=0x00;

	while(len--)
	{
		crc ^= *ptr++;
		for (i=8; i>0 ; --i)
		{
			if(crc & 0x80)
				crc = (crc << 1) ^ 0x31;
			else
				crc = (crc << 1);
		}
	}

	return crc;
}


void deal_sk_data(uint8_t* buf, uint8_t * mask)
{
	SkpData* pSkpData = (SkpData*) buf;

	if(pSkpData->head == FRAME_HEAD && pSkpData->tail == FRAME_TAIL)
	{
		if(pSkpData->crc ==	CalCRC(&buf[1],5))
		{
			if(pSkpData->key == FRAME_DATA)
			{
				if(0 == *(uint8_t*)&pSkpData->value)//test normal
				{
					g_SkpAltitude = __REV(pSkpData->value);
					*mask |= 0x02;
				}
				else //no must
				{
					g_SkpAltitude = 0;
					*mask |= 0x02;
				}
			}
		}
	}
}

void send_sk_data(uint8_t cmd,uint32_t value)
{
	SkpData skpcmd;

	skpcmd.head = FRAME_HEAD;
	skpcmd.tail = FRAME_TAIL;

	switch(cmd)
	{
		case CMD_START:
			skpcmd.key   = CMD_START;
			skpcmd.value = __REV(0);
			break;
		case CMD_FREQ:
			skpcmd.key   = CMD_FREQ;
			skpcmd.value = __REV(value);
			break;
		case CMD_STOP:
		default:
			skpcmd.key   = CMD_STOP;
			skpcmd.value = __REV(0);
			break;
	}
	skpcmd.crc = CalCRC((uint8_t*)&skpcmd.key, 5);

	HAL_UART_Transmit(&huart4, (uint8_t*)&skpcmd, sizeof(SkpData),10);
}

void init_skp_dev()
{
	send_sk_data(CMD_STOP,  1);
	HAL_Delay(1000);
	send_sk_data(CMD_FREQ, g_MeasureFreq);
	HAL_Delay(1000);
	send_sk_data(CMD_START, 1);
}
