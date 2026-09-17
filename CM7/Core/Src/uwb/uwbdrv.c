#include <string.h>
#include "main.h"


uint8_t CalXor(uint8_t* data, int len)
{
	uint8_t sum = 0;
	for(int i=0; i < len; i++)
	{
		sum ^= data[i];
	}
	return sum;
}

uint8_t dev_id   = 0xff;
uint8_t dev_role = 0xff;
void deal_uwb_data(uint8_t* buf, OutFrame * UwbOut, uint8_t * mask) //anchor
{
	uint8_t tid;

	UwbFrame* pUwbFrame = (UwbFrame *) buf;

	if(pUwbFrame->head == __REV(UWB_FRAME_HEADER) /*&&
	   pUwbFrame->xor  == CalXor(&buf[4], 2 + sizeof(UwbData)*19 + 1)*/) //HY USE
	{
		for(int i=0; i<19; i++)
		{
			if(pUwbFrame->uwb[i].los)
			{
				if(pUwbFrame->lid > pUwbFrame->uwb[i].tid)
					tid = pUwbFrame->uwb[i].tid - 1; //HY 1~id save 0~(id -1)
				else if(pUwbFrame->lid < pUwbFrame->uwb[i].tid)
					tid = pUwbFrame->uwb[i].tid - 2; //HY (id+1)~20 save (id-2)~20
				else //id skip
					continue;

				UwbOut->uwb[tid].range = pUwbFrame->uwb[i].range;
				UwbOut->uwb[tid].aoa   = pUwbFrame->uwb[i].aoa;
				UwbOut->uwb[tid].pitch = pUwbFrame->uwb[i].elevation;
				UwbOut->uwb[tid].pdoa1 = pUwbFrame->uwb[i].pdoa1;
				UwbOut->uwb[tid].pdoa2 = pUwbFrame->uwb[i].pdoa2;
				*mask |= 0x01;
			}
		}

		UwbOut->id     = pUwbFrame->lid;
		UwbOut->fsn[0] = pUwbFrame->fsn;
		if(dev_id == 0xff)
		{
			dev_id   = pUwbFrame->lid;
			dev_role = 0x0;
		}
	}
}

void deal_uwb_data_t(uint8_t* buf, OutFrame_T* UwbOut, uint8_t * mask) //tag
{
	UwbFrame_T * pUwbFrame = (UwbFrame_T *) buf;

	if(pUwbFrame->head == __REV(UWB_FRAME_HEADER_T) && \
	   pUwbFrame->xor  == CalXor(buf, 4 + 2 + 190*2 + 24 + 20*2) && \
	   pUwbFrame->lid  == 0)
	{
		if(dev_id == 0xff)
		{
			dev_id   = *(uint8_t*)(SYS_CONFIG_ID);
			dev_role = 0x1;
		}
		UwbOut->id  = dev_id;
		UwbOut->fsn[0] = pUwbFrame->fsn;
		memcpy((uint8_t*)UwbOut->tdoa, (uint8_t*)pUwbFrame->tdoa, 2*190 + 24 + 2*20);
		*mask |= 0x01;
	}
}
