#include "mtidrv.h"
#include "main.h"

#define MEASURING_FREQ        10
extern uint8_t  dev_role;
extern UART_HandleTypeDef huart1;


uint8_t CalSum(uint8_t* data, int len)
{
	uint8_t sum = 0;
	for(int i=1; i < (len-1); i++)
	{
		sum -= data[i];
	}
	return sum;
}

void deal_mti_data(uint8_t* buf, uint8_t * mask)
{
	int pos = 0;
	float     f;
	uint8_t  s[2];
	uint16_t code;
	MtiHead * pMtiHead = (MtiHead *) buf;

	if(pMtiHead->preamble == XS_PREAMBLE && *(&buf[4]+pMtiHead->len) == CalSum(buf, pMtiHead->len+4+1))
	{
		pos += sizeof(MtiHead); //skip frame header,goto Mtdata2

		while(1)
		{
			code = buf[pos+1]  | buf[pos]<<8;  //Mtdata2 type
			pos          += 3;  //Skip Mtdata2 type 2B ,Skip Mtdata2 length

			switch(code)
			{
				case XDI_PacketCounter:
					s[0] = buf[pos];
					s[1] = buf[pos+1];
					g_PacketCounter = s[1] | s[0] << 8;
					pos            +=  2;
					break;
				case XDI_SampleTimeFine:
//					g_PacketCounter =__REV(*((uint32_t*)&buf[pos]));
					pos            +=  4;
					break;
				case XDI_EulerAngles:
					for(int m=0; m<3; m++)
					{
						uint32_t e = __REV(*((uint32_t*)&buf[pos]));
						f = *(float *)&e;
						g_EulerAngles[m]= (int16_t)(f * 100);
						pos        +=  4;
					}
					break;
				case XDI_Acceleration:
					for(int m=0; m<3; m++)
					{
						uint32_t a = __REV(*((uint32_t*)&buf[pos]));
						f = *(float *)&a;
						g_Acceleration[m]= (int16_t)(f*100);
						pos        +=  4;
					}
					break;
				case XDI_MagneticField:
					for(int m=0; m<3; m++)
					{
						uint32_t ma = __REV(*((uint32_t*)&buf[pos]));
						f = *(float *)&ma;
						g_MagneticField[m]= (int16_t)(f*100);
						pos        +=  4;
					}
					break;
				case XDI_BaroPressure:
					g_Pressure  = __REV(*((uint32_t*)&buf[pos]));
					pos          +=  4;
					break;
				case XDI_StatusWord:
					g_StatusWord  = __REV(*((uint32_t*)&buf[pos]));
					pos          +=  4;
					break;
				default:
					pos          +=  buf[pos]; //skip  Mtdata2 data length
					break;
			}

			if(pMtiHead->len > pos)
			{
				continue;
			}
			else
			{
				break;
			}
		}

		*mask |= 0x04;
	}
}

void send_mti_data(uint8_t cmd)
{
	uint8_t data[40];
	MtiHead mticmd;

	mticmd.preamble = XS_PREAMBLE;
	mticmd.bid      = 0xff;

	switch(cmd)
	{
		case XMID_GotoConfig://FA FF 30 00 D1
			mticmd.mid = cmd;
			mticmd.len =   0;
			break;
		case XMID_GotoMeasurement://FA FF 10 00 F1
			mticmd.mid = cmd;
			mticmd.len =   0;
			break;
		case XMID_ResetOrientation://FA FF A4 02 00 04 57
			mticmd.mid =  cmd;
			mticmd.len =   2;
			data[0]    = 0x00;
			data[1]    = 0x04;  //Alignment reset
			break;
		/*NOTE: if modify there, you need to modify MTI_FRAME_LEN in main. c*/
		case XMID_GotoOperational://FA FF C0 1C 10 20 FF FF 10 60 FF FF 20 30 00 0A 40 20 00 0A C0 20 00 0A 30 10 00 0A E0 20 FF FF 93
			mticmd.mid =  cmd;    //FA FF C0 1C 10 20 FF FF 10 60 FF FF 20 30 00 01 40 20 00 01 C0 20 00 01 30 10 00 01 E0 20 FF FF B7
			mticmd.len =   28;    //FA FF C0 1C 10 20 FF FF 10 60 FF FF 20 30 00 02 40 20 00 02 C0 20 00 02 30 10 00 02 E0 20 FF FF B3
			*(uint16_t*)(data + 0)  = __REV16(XDI_PacketCounter);
			*(uint16_t*)(data + 2)  = __REV16(0xffff);
			*(uint16_t*)(data + 4)  = __REV16(XDI_SampleTimeFine);
			*(uint16_t*)(data + 6)  = __REV16(0xffff);
			*(uint16_t*)(data + 8)  = __REV16(XDI_EulerAngles);
			*(uint16_t*)(data + 10) = __REV16(MEASURING_FREQ);
			*(uint16_t*)(data + 12) = __REV16(XDI_Acceleration);
			*(uint16_t*)(data + 14) = __REV16(MEASURING_FREQ);
			*(uint16_t*)(data + 16) = __REV16(XDI_MagneticField);
			*(uint16_t*)(data + 18) = __REV16(MEASURING_FREQ);
			*(uint16_t*)(data + 20) = __REV16(XDI_BaroPressure);
			*(uint16_t*)(data + 22) = __REV16(MEASURING_FREQ);
			*(uint16_t*)(data + 24) = __REV16(XDI_StatusWord);
			*(uint16_t*)(data + 26) = __REV16(0xffff);
			break;
		default:
			break;
	}
	uint8_t tmp[sizeof(MtiHead) + mticmd.len + 1];
	memcpy(tmp, &mticmd, sizeof(MtiHead));
	memcpy(tmp+sizeof(MtiHead), data, mticmd.len);
	tmp[sizeof(MtiHead)+mticmd.len] = CalSum((uint8_t*)&mticmd, sizeof(MtiHead) + mticmd.len + 1);
	HAL_UART_Transmit(&huart1, tmp, sizeof(tmp), 10);
}

void init_mti_dev()
{
	send_mti_data(XMID_GotoConfig);
	send_mti_data(XMID_GotoOperational);
	send_mti_data(XMID_GotoMeasurement);
	send_mti_data(XMID_ResetOrientation);
}
