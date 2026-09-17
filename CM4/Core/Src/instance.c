/*! ----------------------------------------------------------------------------
 *  @file    instance.c
 *  @brief   Decawave application demo
 *
 * @attention
 *
 * Copyright 2016 (c) Decawave Ltd, Dublin, Ireland
 *
 * All rights reserved.
 *
 * @author Decawave
 */
#include  <stdio.h>
#include  <stdbool.h>
#include  "deca_device_api.h"
#include  "deca_regs.h"
#include  "deca_spi.h"
#include  "instance.h"
#include  "port.h"
#include  "math.h"
#include  "main.h"
// -------------------------------------------------------------------------------------------------------------------
//      Data Definitions
// -------------------------------------------------------------------------------------------------------------------
#define    DS_TWR_ND_AOA_MODE_YW

#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define    DEBUG1            0x0//*(uint8_t*)(SYS_CONFIG_DEBUG)  /*0x1 - TimeStamp ; 0x2 - Original distance ; 0x4 - Original pdoa ; 0x8 - Original CIR */
#define    THIS_DEV_TYPE     0x0//*(uint8_t*)(SYS_CONFIG_MODE)   /*0 - ANCHOR or LISTENER(id > MAX_ANCHOR_LIST_SIZE) ; 1 - TAG */

#define    singleSlot        0x130B0000//0x9858000//*(uint64_t*)(UWB_CONFIG_SOLT + 24)
#define    UUS_TO_DWT_TIME   63898

volatile bool    rf_switch;
volatile bool    flag_g   = false;

uint8_t  tx_poll_msg[ ] = {0x41, 0xE0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t  tx_resp_msg1[] = {0x41, 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t  tx_resp_msg2[] = {0x41, 0xE2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t  tx_resp_msg3[] = {0x41, 0xE3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t  tx_resp_msg4[] = {0x41, 0xE4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

uint8_t  buf[544]; //Out frame
uint8_t  rx_msg[1024]; //In frame
int16_t  pdoa1 = 0, pdoa2 = 0;
uint8_t txTimeStamp[5] = {0, 0, 0, 0, 0};

extern int16_t pit;
extern void range_output(uint8_t* data, uint16_t len);
extern double calc_aoa(uint8_t src, int16_t pdoa1, int16_t pdoa2);
extern void kf_twr_init(float dt);
extern float kf_twr_update(float meas_d);
// -------------------------------------------------------------------------------------------------------------------
// Functions
// -------------------------------------------------------------------------------------------------------------------

void toggle_rf_switch(uint8_t selt)
{
	if (selt)
	{
		HAL_GPIO_WritePin(VC1_GPIO_Port, VC1_Pin, 0);
		HAL_GPIO_WritePin(VC2_GPIO_Port, VC2_Pin, 1);//01
//		dwt_setpdoaoffset(*(uint16_t *)(UWB_CONFIG_SOLT + 22));//3700
		dwt_modify16bitoffsetreg(CIA_ADJUST_ID, 0U, (uint16_t)~CIA_ADJUST_PDOA_ADJ_OFFSET_BIT_MASK, *(uint16_t *)(UWB_CONFIG_SOLT + 20));//3700
	    rf_switch = false;
	}
	else
	{
		HAL_GPIO_WritePin(VC1_GPIO_Port, VC1_Pin, 1);
		HAL_GPIO_WritePin(VC2_GPIO_Port, VC2_Pin, 0);//10
//		dwt_setpdoaoffset(*(uint16_t *)(UWB_CONFIG_SOLT + 22));//2300
		dwt_modify16bitoffsetreg(CIA_ADJUST_ID, 0U, (uint16_t)~CIA_ADJUST_PDOA_ADJ_OFFSET_BIT_MASK, *(uint16_t *)(UWB_CONFIG_SOLT + 22));//2300
	    rf_switch = true;
	}
}

#ifdef SS_TWR_MODE

int uwbLib_start()
{
	(*(uint32_t *)&tx_poll_msg[8])++;
	dwt_forcetrxoff();
    dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0);
    dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1);

    dwt_setgpiovalue(GPIO_4, 0x1);
    dwt_setgpiovalue(GPIO_6, 0x0);
    dwt_setgpiodir(0xFFAF);
    if (dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
    	uart_printf("TAG send poll failed\r\n");
    	return -1;
    } else {
	    uart_printf("TAG send poll sucess\r\n");
	    return 0;
	}
}

void rx_ok_cb(const dwt_cb_data_t *rxd)
{
		rxtime = 0;
	    uint16_t frame_len = dwt_getframelength(NULL);

	    dwt_readrxdata((uint8_t *)&rx_msg, frame_len, 0);
        //for (int i = 0 ; i < frame_len; i++) printf(" %02x ", *((uint8_t*)&rx_msg+i));

	    if (memcmp(&rx_msg, tx_poll_msg, 2) == 0) { //ANC

	        dwt_readrxtimestamp((uint8_t *)&rxtime2, 0);
	#if 1
	        delaytxtime = (rxtime2 + 1200 * UUS_TO_DWT_TIME) >> 8;
	        dwt_setdelayedtrxtime(delaytxtime);
	        txtime = (long long)delaytxtime << 8;
	#else
				  delaytxtime = rxtime2 + 1200 * UUS_TO_DWT_TIME;
	        dwt_setdelayedtrxtime(delaytxtime >> 8);
				  txtime = delaytxtime;
	#endif
			memcpy((uint8_t*)&tx_msg + 0, tx_final_msg, sizeof(tx_final_msg));
	        memcpy((uint8_t*)&tx_msg + 2, &rxtime2, 5);
			memcpy((uint8_t*)&tx_msg + 7, &txtime,  5);
	        dwt_writetxdata(sizeof(tx_msg) - 2, (uint8_t *)&tx_msg, 0);
	        dwt_writetxfctrl(sizeof(tx_msg), 0, 1);
			dwt_setrxtimeout(0);
	        if(dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
	            dwt_writesysstatuslo(0xFFFFFFFF);
	            dwt_rxenable(DWT_START_RX_IMMEDIATE);
	        }
			//if (DEBUG & 0x1) printf("ANC send resp,0x%02x%08x 0x%02x%08x\r\n",*((uint32_t*)&rxtime2+1),*((uint32_t*)&rxtime2+0),*((uint32_t*)&txtime+1),*((uint32_t*)&txtime+0));
		}
	    if (memcmp(&rx_msg, tx_final_msg, 2) == 0) { //TAG
	        long long poll_rx_ts = 0, resp_tx_ts  = 0;

	       // uint32_t poll_tx_ts, resp_rx_ts;
	       // uint32_t poll_rx_ts, resp_tx_ts;
	       // poll_tx_ts = dwt_readtxtimestamplo32();
	       // resp_rx_ts = dwt_readrxtimestamplo32();
	       // float clockOffsetRatio = ((float)dwt_readclockoffset()) / (uint32_t)(1<<26);
	       // memcpy(&poll_rx_ts, &rx_buffer[2], 4);
	       // memcpy(&resp_tx_ts, &rx_buffer[7], 4);
	       // int32_t Ra = resp_rx_ts - poll_tx_ts;
	       // int32_t Db = resp_tx_ts - poll_rx_ts;
	       // double tof = (Ra - Db * (1 - clockOffsetRatio)) / 2.0;
	       // double dis = tof * DWT_TIME_UNITS * SPEED_OF_LIGHT;
	       // uart_printf("0x%08x  0x%08x\r\n0x%08x  0x%08x\r\n%.2f  %.2f\r\n\r\n",poll_tx_ts,poll_rx_ts,resp_tx_ts,resp_rx_ts,dis,tof);

	        dwt_readtxtimestamp((uint8_t *)&txtime);
	        dwt_readrxtimestamp((uint8_t *)&rxtime, 0);
			float clockOffsetRatio = ((float)dwt_readclockoffset()) / (uint32_t)(1<<26);
	        memcpy(&poll_rx_ts, (uint8_t*)&rx_msg + 2, 5);
	        memcpy(&resp_tx_ts, (uint8_t*)&rx_msg + 7, 5);
	        long long Ra = rxtime - txtime;
	        long long Db = resp_tx_ts - poll_rx_ts;
	        double tof = (Ra - Db * (1 - clockOffsetRatio)) / 2.0;
	        double dis = tof * DWT_TIME_UNITS * SPEED_OF_LIGHT;
            //if (DEBUG == 0x0) printf("%.0f\r\n",dis * 100);
			dwt_setrxtimeout(0);
			dwt_rxenable(DWT_START_RX_IMMEDIATE);
		}
}
#elif defined(DS_TWR_MODE)

int uwbLib_start()
{
	(*(uint32_t *)&tx_poll_msg[8])++;
	dwt_forcetrxoff();
    dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0);
    dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1);

    dwt_setgpiovalue(GPIO_4, 0x1);
    dwt_setgpiovalue(GPIO_6, 0x0);
    dwt_setgpiodir(0xFFAF);
    if (dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
    	uart_printf("TAG send poll failed\r\n");
    	return -1;
    } else {
	    uart_printf("TAG send poll sucess\r\n");
	    return 0;
	}
}

void rx_ok_cb(const dwt_cb_data_t *cb_data)
{
    (void)cb_data;
    int16_t sts;
    rxtime = 0;

    uint16_t frame_len = dwt_getframelength(NULL);
    dwt_readrxdata((uint8_t *)&rx_msg, frame_len, 0);

    uart_printf("----> rx_ok_cb(%02x),%d ", rx_msg[1], frame_len);
#if 0
    for (int i = 0 ; i < frame_len; i++) uart_printf(" %02x ", *((uint8_t*)&rx_msg+i));
	dwt_rxenable(DWT_START_RX_IMMEDIATE);
	return;
#endif

    if (memcmp(&rx_msg, tx_poll_msg, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //ANC send RESP

    	if (start == 0) start = *(uint16_t*)&rx_msg[8];
    	else            end   = *(uint16_t*)&rx_msg[8];

        dwt_readrxtimestamp((uint8_t *)&rxtime2, 0);
#if 0
        delaytxtime = (rxtime2 + 1200 * UUS_TO_DWT_TIME) >> 8;
        dwt_setdelayedtrxtime(delaytxtime);
        txtime = (long long)delaytxtime << 8;
#else
		delaytxtime = rxtime2 + 3000 * UUS_TO_DWT_TIME;
        dwt_setdelayedtrxtime(delaytxtime >> 8);
#endif
        dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg), 0, 1);
        if(dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
        	uart_printf("ANC send resp failed\r\n");
        }
        else uart_printf("ANC send resp,0x%02x%08x 0x%02x%08x\r\n",*((uint32_t*)&rxtime2+1),*((uint32_t*)&rxtime2+0),*((uint32_t*)&txtime+1),*((uint32_t*)&txtime+0));
	}
    else if (memcmp(&rx_msg, tx_resp_msg, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //TAG SEND FINAL

        dwt_readrxtimestamp((uint8_t *)&rxtime, 0);
        dwt_readtxtimestamp((uint8_t *)&txtime2);
#if 0
        delaytxtime = (rxtime + 1200 * UUS_TO_DWT_TIME) >> 8;
        dwt_setdelayedtrxtime(delaytxtime);
        txtime = ((long long)delaytxtime << 8) + TX_ANT_DLY;
#else
		delaytxtime = rxtime + 3000 * UUS_TO_DWT_TIME;
        dwt_setdelayedtrxtime(delaytxtime >> 8);
        txtime = delaytxtime;
#endif
        memcpy(tx_fina_msg + 2, &txtime2, 5);
        memcpy(tx_fina_msg + 7, &rxtime,  5);
		memcpy(tx_fina_msg + 12,&txtime,  5);

        dwt_writetxdata(sizeof(tx_fina_msg), tx_fina_msg, 0);
        dwt_writetxfctrl(sizeof(tx_fina_msg), 0, 1);
        if(dwt_starttx(DWT_START_TX_DELAYED) != DWT_SUCCESS) {
        	uart_printf("TAG send final failed\r\n");
        }
        else uart_printf("TAG send final,0x%02x%08x 0x%02x%08x 0x%02x%08x\r\n",*((uint32_t*)&txtime2+1),*((uint32_t*)&txtime2+0),*((uint32_t*)&rxtime+1),*((uint32_t*)&rxtime+0),*((uint32_t*)&txtime+1),*((uint32_t*)&txtime+0));
	}
    else if (memcmp(&rx_msg, tx_fina_msg, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //ANC CALL DIS
        long long poll_tx_ts = 0, resp_rx_ts  = 0, final_tx_ts = 0;

        dwt_readtxtimestamp((uint8_t *)&txtime);
        dwt_readrxtimestamp((uint8_t *)&rxtime, 0);

        memcpy(&poll_tx_ts,  (uint8_t*)&rx_msg + 2, 5);
        memcpy(&resp_rx_ts,  (uint8_t*)&rx_msg + 7, 5);
        memcpy(&final_tx_ts, (uint8_t*)&rx_msg + 12,5);
        int64_t Ra = resp_rx_ts - poll_tx_ts;
        int64_t Rb = rxtime - txtime;
        int64_t Da = final_tx_ts - resp_rx_ts;
        int64_t Db = txtime - rxtime2;
        double dis = (Ra * Rb - Da * Db) / (Ra + Rb + Da + Db) * DWT_TIME_UNITS * SPEED_OF_LIGHT;
        uart_printf("(0x%02x%08x*0x%02x%08x-0x%02x%08x*0x%02x%08x) [%.3f]\r\n",*((uint32_t*)&Ra+1),*((uint32_t*)&Ra+0),*((uint32_t*)&Rb+1),*((uint32_t*)&Rb+0),*((uint32_t*)&Da+1),*((uint32_t*)&Da+0),*((uint32_t*)&Db+1),*((uint32_t*)&Db+0),dis-19.5);
        HAL_HSEM_FastTake(1);
	    *(uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 0) = 1; //display on 18#
	    *(uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 1) = 1;  //mask
        *(uint16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 2 + 8 + 8) = sn++;
        *(int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 2 + 8 + 8 + 2) = (dis - 19.5) * 100;
	    *(uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 0) = 2; //display on 18#
	    *(uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 1) = 1;  //mask
        *(uint16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 2 + 8 + 8) = end;
        *(int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 2 + 8 + 8 + 2) = start;
        HAL_HSEM_Release(1, 0);
		dwt_rxenable(DWT_START_RX_IMMEDIATE);
	}
    else if(dwt_readstsquality(&sts, 0) < 0) {
		dwt_rxenable(DWT_START_RX_IMMEDIATE);
    	uart_printf("----> sts error");
    }
}
#elif defined(DS_TWR_ND_MODE)

int uwbLib_start()
{
	(*(uint32_t *)&tx_poll_msg[8])++;
	dwt_forcetrxoff();
    dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0);
    dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1);

    dwt_setgpiovalue(GPIO_4, 0x1);
    dwt_setgpiovalue(GPIO_6, 0x0);
    dwt_setgpiodir(0xFFAF);
    if (dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
    	uart_printf("TAG send poll failed\r\n");
    	return -1;
    } else {
	    uart_printf("TAG send poll sucess\r\n");
	    return 0;
	}
}

void rx_ok_cb(const dwt_cb_data_t *cb_data)
{
    (void)cb_data;
    int16_t sts;

    uint16_t frame_len = dwt_getframelength(NULL);
    dwt_readrxdata((uint8_t *)&rx_msg, frame_len, 0);

    uart_printf("----> rx_ok_cb(%02x),%d ", rx_msg[1], frame_len);
#if 0
    for (int i = 0 ; i < frame_len; i++) uart_printf(" %02x ", *((uint8_t*)&rx_msg+i));
	dwt_rxenable(DWT_START_RX_IMMEDIATE);
	return;
#endif

    if (memcmp(&rx_msg, tx_poll_msg, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //ANC, receive poll and sent resp1 , save r1

    	if (start == 0) start = *(uint16_t*)&rx_msg[8];
    	else            end   = *(uint16_t*)&rx_msg[8];

        dwt_readrxtimestamp((uint8_t *)&r1, 0);
        dwt_writetxdata(sizeof(tx_resp_msg1), tx_resp_msg1, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg1), 0, 1);

        dwt_setgpiovalue(GPIO_4, 0x1);
        dwt_setgpiovalue(GPIO_6, 0x0);
        dwt_setgpiodir(0xFFAF);
        if(dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
        	uart_printf("ANC send resp failed\r\n");
        }
        uart_printf("ANC send resp1, 0x%02x%08x\r\n",*((uint32_t*)&r1+1),*((uint32_t*)&r1+0));
	}
    else if (memcmp(&rx_msg, tx_resp_msg1, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //TAG, receive resp1 and sent resp2, save t0, r2 for send

        dwt_readtxtimestamp((uint8_t *)&t0);
    	dwt_readrxtimestamp((uint8_t *)&r2, 0);

        memcpy(tx_resp_msg2 + 2, &t0, 5);
        memcpy(tx_resp_msg2 + 7, &r2,  5);

        dwt_writetxdata(sizeof(tx_resp_msg2), tx_resp_msg2, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg2), 0, 1);
        dwt_setgpiovalue(GPIO_4, 0x1);
        dwt_setgpiovalue(GPIO_6, 0x0);
        dwt_setgpiodir(0xFFAF);
        if(dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
        	uart_printf("TAG send resp2 failed\r\n");
        }
        uart_printf("TAG send resp2, 0x%02x%08x 0x%02x%08x\r\n",*((uint32_t*)&t0+1),*((uint32_t*)&t0+0),*((uint32_t*)&r2+1),*((uint32_t*)&r2+0));
	}
    else if (memcmp(&rx_msg, tx_resp_msg2, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //ANC, receive resp2 and sent resp3, save t0, r2 for use

        dwt_readtxtimestamp((uint8_t *)&t1);
    	dwt_readrxtimestamp((uint8_t *)&r3, 0);

        memcpy((uint8_t *)&t0,  (uint8_t*)&rx_msg + 2, 5);
        memcpy((uint8_t *)&r2,  (uint8_t*)&rx_msg + 7, 5);

        dwt_writetxdata(sizeof(tx_resp_msg3), tx_resp_msg3, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg3), 0, 1);

        dwt_setgpiovalue(GPIO_4, 0x1);
        dwt_setgpiovalue(GPIO_6, 0x0);
        dwt_setgpiodir(0xFFAF);
        if(dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
        	uart_printf("ANC send resp3 failed\r\n");
        }
        else uart_printf("ANC send resp3, 0x%02x%08x 0x%02x%08x\r\n",*((uint32_t*)&t0+1),*((uint32_t*)&t0+0),*((uint32_t*)&r2+1),*((uint32_t*)&r2+0));
	}
    else if (memcmp(&rx_msg, tx_resp_msg3, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //TAG, receive resp3 and sent resp4, save t2 for send

        dwt_readtxtimestamp((uint8_t *)&t2);
        memcpy(tx_resp_msg4 + 2, &t2, 5);

        dwt_writetxdata(sizeof(tx_resp_msg4), tx_resp_msg4, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg4), 0, 1);

        dwt_setgpiovalue(GPIO_4, 0x1);
        dwt_setgpiovalue(GPIO_6, 0x0);
        dwt_setgpiodir(0xFFAF);
        if(dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
        	uart_printf("TAG send resp4 failed\r\n");
        }
        uart_printf("TAG send resp4, 0x%02x%08x\r\n",*((uint32_t*)&t2+1),*((uint32_t*)&t2+0));

	}
    else if (memcmp(&rx_msg, tx_resp_msg4, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //ANC, receive resp4 and calculate distance, save t2 for use

        dwt_readtxtimestamp((uint8_t *)&t3);
        memcpy((uint8_t *)&t2, (uint8_t*)&rx_msg + 2, 5);
        int64_t Ra = r2 - t0;//resp_rx_ts - poll_tx_ts;
        int64_t Rb = r3 - t1;//rxtime - txtime;
        int64_t Da = t2 - r2;//final_tx_ts - resp_rx_ts;
        int64_t Db = t1 - r1;//txtime - rxtime2;
        double dis = (Ra * Rb - Da * Db) / (Ra + Rb + Da + Db) * DWT_TIME_UNITS * SPEED_OF_LIGHT;
        uart_printf("(0x%02x%08x*0x%02x%08x-0x%02x%08x*0x%02x%08x) [%.3f]\r\n",*((uint32_t*)&Ra+1),*((uint32_t*)&Ra+0),*((uint32_t*)&Rb+1),*((uint32_t*)&Rb+0),*((uint32_t*)&Da+1),*((uint32_t*)&Da+0),*((uint32_t*)&Db+1),*((uint32_t*)&Db+0),dis);
        HAL_HSEM_FastTake(1);
	    *(uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 0) = 1; //display on 18#
	    *(uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 1) = 1;  //mask
        *(uint16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 2 + 8 + 8) = sn++;
        *(int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 2 + 8 + 8 + 2) = dis * 100;
	    *(uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 0) = 2; //display on 18#
	    *(uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 1) = 1;  //mask
        *(uint16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 2 + 8 + 8) = end;
        *(int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 2 + 8 + 8 + 2) = start;
        HAL_HSEM_Release(1, 0);
		dwt_rxenable(DWT_START_RX_IMMEDIATE);
	}
    else if(dwt_readstsquality(&sts, 0) < 0) {
		dwt_rxenable(DWT_START_RX_IMMEDIATE);
    	uart_printf("----> sts error");
    }
}
#elif defined(DS_TWR_ND_AOA_MODE)
void rx_ok_cb(const dwt_cb_data_t *cb_data)
{
    (void)cb_data;
    int16_t sts;

    uint16_t frame_len = dwt_getframelength(NULL);
    dwt_readrxdata((uint8_t *)&rx_msg, frame_len, 0);

    uart_printf("----> rx_ok_cb(%02x),%d ", rx_msg[1], frame_len);
#if 0
    for (int i = 0 ; i < frame_len; i++) uart_printf(" %02x ", *((uint8_t*)&rx_msg+i));
	dwt_rxenable(DWT_START_RX_IMMEDIATE);
	return;
#endif
    if (memcmp(&rx_msg, tx_poll_msg, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //ANC, receive poll and sent resp1 , save r1

    	if (start == 0) start = *(uint16_t*)&rx_msg[8];
    	else            end   = *(uint16_t*)&rx_msg[8];

    	inst_pdoa[1][0] = dwt_readpdoa();
        dwt_readrxtimestamp((uint8_t *)&r1, 0);
        toggle_rf_switch(1);
        dwt_writetxdata(sizeof(tx_resp_msg1), tx_resp_msg1, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg1), 0, 1);
        dwt_setgpiovalue(GPIO_4, 0x1);
        dwt_setgpiovalue(GPIO_6, 0x0);
        dwt_setgpiodir(0xFFAF);
        if(dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
        	uart_printf("ANC send resp failed\r\n");
        }
        uart_printf("ANC send resp1, 0x%02x%08x\r\n",*((uint32_t*)&r1+1),*((uint32_t*)&r1+0));
	}
    else if (memcmp(&rx_msg, tx_resp_msg1, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //TAG, receive resp1 and sent resp2, save t0, r2 for send

        dwt_readtxtimestamp((uint8_t *)&t0);
    	dwt_readrxtimestamp((uint8_t *)&r2, 0);

        memcpy(tx_resp_msg2 + 2, &t0, 5);
        memcpy(tx_resp_msg2 + 7, &r2,  5);

        dwt_writetxdata(sizeof(tx_resp_msg2), tx_resp_msg2, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg2), 0, 1);
        dwt_setgpiovalue(GPIO_4, 0x1);
        dwt_setgpiovalue(GPIO_6, 0x0);
        dwt_setgpiodir(0xFFAF);
        if(dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
        	uart_printf("TAG send resp2 failed\r\n");
        }
        uart_printf("TAG send resp2, 0x%02x%08x 0x%02x%08x\r\n",*((uint32_t*)&t0+1),*((uint32_t*)&t0+0),*((uint32_t*)&r2+1),*((uint32_t*)&r2+0));
	}
    else if (memcmp(&rx_msg, tx_resp_msg2, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //ANC, receive resp2 and sent resp3, save t0, r2 for use

    	inst_pdoa[1][1] = dwt_readpdoa();
        dwt_readtxtimestamp((uint8_t *)&t1);
    	dwt_readrxtimestamp((uint8_t *)&r3, 0);

        memcpy((uint8_t *)&t0,  (uint8_t*)&rx_msg + 2, 5);
        memcpy((uint8_t *)&r2,  (uint8_t*)&rx_msg + 7, 5);

        toggle_rf_switch(0);
        dwt_writetxdata(sizeof(tx_resp_msg3), tx_resp_msg3, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg3), 0, 1);
        dwt_setgpiovalue(GPIO_4, 0x1);
        dwt_setgpiovalue(GPIO_6, 0x0);
        dwt_setgpiodir(0xFFAF);
        if(dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
        	uart_printf("ANC send resp3 failed\r\n");
        }
        uart_printf("ANC send resp3, 0x%02x%08x 0x%02x%08x\r\n",*((uint32_t*)&t0+1),*((uint32_t*)&t0+0),*((uint32_t*)&r2+1),*((uint32_t*)&r2+0));
	}
    else if (memcmp(&rx_msg, tx_resp_msg3, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //TAG, receive resp3 and sent resp4, save t2 for send

        dwt_readtxtimestamp((uint8_t *)&t2);
        memcpy(tx_resp_msg4 + 2, &t2, 5);

        dwt_writetxdata(sizeof(tx_resp_msg4), tx_resp_msg4, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg4), 0, 1);

        dwt_setgpiovalue(GPIO_4, 0x1);
        dwt_setgpiovalue(GPIO_6, 0x0);
        dwt_setgpiodir(0xFFAF);
        if(dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
        	uart_printf("TAG send resp4 failed\r\n");
        }
        uart_printf("TAG send resp4, 0x%02x%08x\r\n",*((uint32_t*)&t2+1),*((uint32_t*)&t2+0));

	}
    else if (memcmp(&rx_msg, tx_resp_msg4, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //ANC, receive resp4 and calculate distance, save t2 for use

        dwt_readtxtimestamp((uint8_t *)&t3);
        memcpy((uint8_t *)&t2, (uint8_t*)&rx_msg + 2, 5);
        int64_t Ra = r2 - t0;//resp_rx_ts - poll_tx_ts;
        int64_t Rb = r3 - t1;//rxtime - txtime;
        int64_t Da = t2 - r2;//final_tx_ts - resp_rx_ts;
        int64_t Db = t1 - r1;//txtime - rxtime2;
        double dis = (Ra * Rb - Da * Db) / (Ra + Rb + Da + Db) * DWT_TIME_UNITS * SPEED_OF_LIGHT;
        double ao1 = atan2(sqrt(3) * (double)inst_pdoa[1][0], 2 * (double)inst_pdoa[1][1] - (double)inst_pdoa[1][0]) * (180 / 3.1415926);
        double aoa = (((int)ao1 - 40) + 540) % 360 - 180;
        uart_printf("(0x%02x%08x*0x%02x%08x-0x%02x%08x*0x%02x%08x) [%.3f (%.2f)]\r\n",*((uint32_t*)&Ra+1),*((uint32_t*)&Ra+0),*((uint32_t*)&Rb+1),*((uint32_t*)&Rb+0),*((uint32_t*)&Da+1),*((uint32_t*)&Da+0),*((uint32_t*)&Db+1),*((uint32_t*)&Db+0),dis,aoa);
        HAL_HSEM_FastTake(1);
	    *( uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 0) = 1; //display on 1#
	    *( uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 1) = 1;  //mask
        *(uint16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 2 + 8 + 8)     = dis * 100;
        *( int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 2 + 8 + 8 + 2) = aoa;
        *( int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 2 + 8 + 8 + 6) = inst_pdoa[1][0];
        *( int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 2 + 8 + 8 + 8) = inst_pdoa[1][1];
	    *( uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 0) = 2; //display on 2
	    *( uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 1) = 1;  //mask
        *(uint16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 2 + 8 + 8) = end;
        *(int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 2 + 8 + 8 + 2) = start;
        HAL_HSEM_Release(1, 0);
		dwt_rxenable(DWT_START_RX_IMMEDIATE);
	}
    else if(dwt_readstsquality(&sts, 0) < 0) {
		dwt_rxenable(DWT_START_RX_IMMEDIATE);
    	uart_printf("----> sts error\r\n");
    } else {
    	uart_printf("----> rfc error\r\n");
        dwt_forcetrxoff();
    }
}
#elif defined(DS_TWR_ND_AOA_MODE_1)
dwt_cirdiags_t  rx_diag;
int16_t         rssi_value;
float           rssi_value1;
float           rssi_value2;
int16_t         power_value;
float           power_value1;
float           power_value2;
uint64_t t0 = 0, t1 = 0, t2 = 0, t3 = 0, t4 = 0;
uint64_t r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0;

void rx_ok_cb(const dwt_cb_data_t *cb_data)
{
    (void)cb_data;
    int16_t sts;

    uint16_t frame_len = dwt_getframelength(NULL);
    dwt_readrxdata((uint8_t *)&rx_msg, frame_len, 0);

    uart_printf("----> rx_ok_cb(%02x),%d ", rx_msg[1], frame_len);
#if 0
    for (int i = 0 ; i < frame_len; i++) uart_printf(" %02x ", *((uint8_t*)&rx_msg+i));
	dwt_rxenable(DWT_START_RX_IMMEDIATE);
	return;
#endif
    if (memcmp(&rx_msg, tx_poll_msg, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //ANC, receive poll and sent resp1 , save r1

    	if (start == 0) start = *(uint16_t*)&rx_msg[8];
    	else            end   = *(uint16_t*)&rx_msg[8];

        if (dwt_readdiagnostics_acc(&rx_diag, DWT_ACC_IDX_IP_M) == DWT_SUCCESS) {
            if (dwt_calculate_rssi(&rx_diag, DWT_ACC_IDX_IP_M, &rssi_value) == DWT_SUCCESS)              rssi_value1  = (float)rssi_value / 256.0f;
            if (dwt_calculate_first_path_power(&rx_diag, DWT_ACC_IDX_IP_M, &power_value) == DWT_SUCCESS) power_value1 = (float)power_value / 256.0f;
        }

    	inst_pdoa[1][0] = dwt_readpdoa();
        dwt_readrxtimestamp((uint8_t *)&r1, 0);
        toggle_rf_switch(1);
        dwt_writetxdata(sizeof(tx_resp_msg1), tx_resp_msg1, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg1), 0, 1);
        dwt_setgpiovalue(GPIO_4, 0x1);
        dwt_setgpiovalue(GPIO_6, 0x0);
        dwt_setgpiodir(0xFFAF);
        if(dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
        	uart_printf("ANC send resp failed\r\n");
        }
        uart_printf("ANC send resp1, 0x%02x%08x\r\n",*((uint32_t*)&r1+1),*((uint32_t*)&r1+0));
	}
    else if (memcmp(&rx_msg, tx_resp_msg1, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //TAG, receive resp1 and sent resp2, save t0, r2 for send

        dwt_readtxtimestamp((uint8_t *)&t0);
    	dwt_readrxtimestamp((uint8_t *)&r2, 0);

        memcpy(tx_resp_msg2 + 2, &t0, 5);
        memcpy(tx_resp_msg2 + 7, &r2,  5);

        dwt_writetxdata(sizeof(tx_resp_msg2), tx_resp_msg2, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg2), 0, 1);
        dwt_setgpiovalue(GPIO_4, 0x1);
        dwt_setgpiovalue(GPIO_6, 0x0);
        dwt_setgpiodir(0xFFAF);
        if(dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
        	uart_printf("TAG send resp2 failed\r\n");
        }
        uart_printf("TAG send resp2, 0x%02x%08x 0x%02x%08x\r\n",*((uint32_t*)&t0+1),*((uint32_t*)&t0+0),*((uint32_t*)&r2+1),*((uint32_t*)&r2+0));
	}
    else if (memcmp(&rx_msg, tx_resp_msg2, 2) == 0 && dwt_readstsquality(&sts, 0) >= 0) { //ANC, receive resp2 and sent resp3, save t0, r2 for use

        if (dwt_readdiagnostics_acc(&rx_diag, DWT_ACC_IDX_IP_M) == DWT_SUCCESS) {
            if (dwt_calculate_rssi(&rx_diag, DWT_ACC_IDX_IP_M, &rssi_value) == DWT_SUCCESS)              rssi_value2  = (float)rssi_value / 256.0f;
            if (dwt_calculate_first_path_power(&rx_diag, DWT_ACC_IDX_IP_M, &power_value) == DWT_SUCCESS) power_value2 = (float)power_value / 256.0f;
        }

    	inst_pdoa[1][1] = dwt_readpdoa();
        dwt_readtxtimestamp((uint8_t *)&t1);
    	dwt_readrxtimestamp((uint8_t *)&r3, 0);

        memcpy((uint8_t *)&t0,  (uint8_t*)&rx_msg + 2, 5);
        memcpy((uint8_t *)&r2,  (uint8_t*)&rx_msg + 7, 5);

        toggle_rf_switch(0);
        dwt_writetxdata(sizeof(tx_resp_msg3), tx_resp_msg3, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg3), 0, 1);
        dwt_setgpiovalue(GPIO_4, 0x1);
        dwt_setgpiovalue(GPIO_6, 0x0);
        dwt_setgpiodir(0xFFAF);
        if(dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
        	uart_printf("ANC send resp3 failed\r\n");
        }
        uart_printf("ANC send resp3, 0x%02x%08x 0x%02x%08x\r\n",*((uint32_t*)&t0+1),*((uint32_t*)&t0+0),*((uint32_t*)&r2+1),*((uint32_t*)&r2+0));
	}
    else if (memcmp(&rx_msg, tx_resp_msg3, 2) == 0 /*&& dwt_readstsquality(&sts, 0) >= 0*/) { //TAG, receive resp3 and sent resp4, save t2 for send

        dwt_readtxtimestamp((uint8_t *)&t2);
        memcpy(tx_resp_msg4 + 2, &t2, 5);

        dwt_writetxdata(sizeof(tx_resp_msg4), tx_resp_msg4, 0);
        dwt_writetxfctrl(sizeof(tx_resp_msg4), 0, 1);

        dwt_setgpiovalue(GPIO_4, 0x1);
        dwt_setgpiovalue(GPIO_6, 0x0);
        dwt_setgpiodir(0xFFAF);
        if(dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) != DWT_SUCCESS) {
        	uart_printf("TAG send resp4 failed\r\n");
        }
        uart_printf("TAG send resp4, 0x%02x%08x\r\n",*((uint32_t*)&t2+1),*((uint32_t*)&t2+0));

	}
    else if (memcmp(&rx_msg, tx_resp_msg4, 2) == 0 /*&& dwt_readstsquality(&sts, 0) >= 0*/) { //ANC, receive resp4 and calculate distance, save t2 for use

        dwt_readtxtimestamp((uint8_t *)&t3);
        memcpy((uint8_t *)&t2, (uint8_t*)&rx_msg + 2, 5);
        int64_t Ra = r2 - t0;//resp_rx_ts - poll_tx_ts;
        int64_t Rb = r3 - t1;//rxtime - txtime;
        int64_t Da = t2 - r2;//final_tx_ts - resp_rx_ts;
        int64_t Db = t1 - r1;//txtime - rxtime2;
        double dis = (Ra * Rb - Da * Db) / (Ra + Rb + Da + Db) * DWT_TIME_UNITS * SPEED_OF_LIGHT;
        double ao1 = atan2(sqrt(3) * (double)inst_pdoa[1][0], 2 * (double)inst_pdoa[1][1] - (double)inst_pdoa[1][0]) * (180 / 3.1415926);
        double aoa = (((int)ao1 - 40) + 540) % 360 - 180;
        uart_printf("(0x%02x%08x*0x%02x%08x-0x%02x%08x*0x%02x%08x) [%.3f (%.2f)]\r\n",*((uint32_t*)&Ra+1),*((uint32_t*)&Ra+0),*((uint32_t*)&Rb+1),*((uint32_t*)&Rb+0),*((uint32_t*)&Da+1),*((uint32_t*)&Da+0),*((uint32_t*)&Db+1),*((uint32_t*)&Db+0),dis,aoa);
        HAL_HSEM_FastTake(1);
	    *( uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 0) = 1; //display on 1#
	    *( uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 1) = 1;  //mask
        *(uint16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 2 + 8 + 8 + 0) = dis * 100;
        *( int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 2 + 8 + 8 + 2) = aoa;
        *( int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 2 + 8 + 8 + 6) = inst_pdoa[1][0];
        *( int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 0 + 2 + 8 + 8 + 8) = inst_pdoa[1][1];
	    *( uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 0) = 2; //display on 2
	    *( uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 1) = 1;  //mask
        *(uint16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 2 + 8 + 8 + 0) = end;
        *( int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 2 + 8 + 8 + 2) = start;
        *( int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 2 + 8 + 8 + 6) = (int)rssi_value1;
        *( int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 1 + 2 + 8 + 8 + 8) = (int)power_value1;
	    *( uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 2 + 0) = 2; //display on 3
	    *( uint8_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 2 + 1) = 1;  //mask
        *( int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 2 + 2 + 8 + 8 + 6) = (int)rssi_value2;
        *( int16_t*)(SHD_RAM_ADDR  + 4 + 2 + sizeof(UwbData) * 2 + 2 + 8 + 8 + 8) = (int)power_value2;
        HAL_HSEM_Release(1, 0);
		dwt_rxenable(DWT_START_RX_IMMEDIATE);
	}
    else if(dwt_readstsquality(&sts, 0) < 0) {
		dwt_rxenable(DWT_START_RX_IMMEDIATE);
    	uart_printf("----> sts error\r\n");
    } else {
    	uart_printf("----> rfc error\r\n");
        dwt_forcetrxoff();
    }
}
#elif defined(DS_TWR_ND_AOA_MODE_YW)
uint8_t    buffer[512];
double     inst_idist[MAX_ANCHOR_LIST_SIZE];
int16_t    inst_aoa[  MAX_ANCHOR_LIST_SIZE];
int16_t    inst_pdoa[ MAX_ANCHOR_LIST_SIZE][2];
volatile uint8_t timeout = MAX_ANCHOR_LIST_SIZE;

void instance_config_frameheader_16bit(instance_data_t *inst)
{
    //set frame type (0-2), SEC (3), Pending (4), ACK (5), PanIDcomp(6)
    inst->msg_f.frameCtrl[0] = 0x1 /*frame type 0x1 == data*/ | 0x40 /*PID comp*/;
    inst->msg_f.frameCtrl[1] = 0x8 /*dest extended address (16bits)*/ | 0x80 /*src extended address (16bits)*/;
    inst->msg_f.panID[0] = (inst->panID) & 0xff;
    inst->msg_f.panID[1] = inst->panID >> 8;
    inst->msg_f.seqNum = 0;
}

void tag_send_poll()
{
    instance_config_frameheader_16bit(inst);

    inst->psduLength = ANCH_FRAME_LEN;
    inst->msg_f.seqNum = inst->frameSN++;

    inst->msg_f.destAddr[0] = 0xff;
    inst->msg_f.destAddr[1] = 0xff;
    inst->msg_f.sourceAddr[0] = inst->shortAdd_idx;//inst->eui64[1];
    inst->msg_f.sourceAddr[1] = 0;//inst->eui64[1];

    inst->msg_f.messageData[FSN] = 0;
    inst->msg_f.messageData[PSN] = 1;
    inst->msg_f.messageData[ANCH_MESG_LEN - 1] = 0xff;//Defining data boundaries
    inst->rxResps = inst->shortAdd_idx + 1;//Specify the next sender

    dwt_writetxdata(inst->psduLength, (uint8_t*)&inst->msg_f, 0);
    dwt_writetxfctrl(inst->psduLength, 0, 1);

    uart_printf("send 1st frame\r\n");
    dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);
}

void anch_calc_distance(uint8_t* txtimestamp)
{
    uint64_t pollTxTime = 0; //t0
    uint64_t pollRxTime = 0; //r1
    uint64_t respTxTime = 0; //t1
    uint64_t respRxTime = 0; //r2
    uint64_t finaTxTime = 0; //t3
    uint64_t finaRxTime = 0; //r3

    buf[0]   = 0xaa;
    buf[1]   = 0x55;
    buf[2]   = 0xa5;
    buf[3]   = 0x5a;
    buf[4]   = inst->frameSN;
    buf[538] = (inst->shortAdd_idx == 0) ? 20 : inst->shortAdd_idx;//Rid
    buf[539] =   0;//Xor
    buf[540] = 0xd;
    buf[541] = 0xa;
    buf[542] = 0xd;
    buf[543] = 0xa;

    for (uint8_t src = 0 ; src < NUM_EXPECTED_RESPONSES; src++)
    {
        if (src == inst->shortAdd_idx || (sendtime - MIN(recvtime_f[src][1],recvtime_f[src][0])) > 2 * (singleSlot / (128 * 512) *1.0256 / 1000) * MAX_ANCHOR_LIST_SIZE) continue;//skip self //>3times send

        // get local timestamp
        memcpy(&pollTxTime, &table_f1[inst->shortAdd_idx][0].messageData[SENDTIME], EVERY_TIMESTAMP_LENGTH);
        memcpy(&respRxTime, &table_f1[inst->shortAdd_idx][1].messageData[RECVTIME + src * EVERY_TIMESTAMP_LENGTH], EVERY_TIMESTAMP_LENGTH);
        memcpy(&finaTxTime, txtimestamp, 5);

        // get Get timestamp of the other party
        memcpy(&pollRxTime, &table_f1[src][1].messageData[RECVTIME + inst->shortAdd_idx * EVERY_TIMESTAMP_LENGTH], EVERY_TIMESTAMP_LENGTH);
        memcpy(&respTxTime, &table_f1[src][0].messageData[SENDTIME], EVERY_TIMESTAMP_LENGTH);
        memcpy(&finaRxTime, &table_f1[src][0].messageData[RECVTIME + inst->shortAdd_idx * EVERY_TIMESTAMP_LENGTH], EVERY_TIMESTAMP_LENGTH);

        int64_t Ra = respRxTime - pollTxTime;
        int64_t Rb = finaRxTime - respTxTime;
        int64_t Da = finaTxTime - respRxTime;
        int64_t Db = respTxTime - pollRxTime;
        inst_idist[src] = (Ra > 0 && Rb > 0 && Da > 0 && Db > 0) ? (Ra * Rb - Da * Db) / (Ra + Rb + Da + Db) * DWT_TIME_UNITS * SPEED_OF_LIGHT : 0;

        if (inst_pdoa[src][0] !=0 && inst_pdoa[src][1] !=0 && Ra > 0 && Rb > 0 && Da > 0 && Db > 0 && inst_idist[src] > 0 && inst_idist[src] < 120)
        {
            buf[4+2+28*(src+0)+0] = (src == 0) ? 20 : src;//Sid
            buf[4+2+28*(src+0)+1] = 0x1;
            *(uint16_t *)&buf[4+2+28*(src+0)+2+8+8+0] = kf_twr_update(inst_idist[src]) * 100 - *(uint32_t *)(UWB_CONFIG_SOLT + 16);
            *( int16_t *)&buf[4+2+28*(src+0)+2+8+8+2] = (int16_t)calc_aoa(src, inst_pdoa[src][0], inst_pdoa[src][1]);
            *( int16_t *)&buf[4+2+28*(src+0)+2+8+8+4] = (int16_t)pit;
            *( int16_t *)&buf[4+2+28*(src+0)+2+8+8+6] = inst_pdoa[src][0];
            *( int16_t *)&buf[4+2+28*(src+0)+2+8+8+8] = inst_pdoa[src][1];
//            uart_printf("(0x%02x%08x*0x%02x%08x-0x%02x%08x*0x%02x%08x) [%.3f (%d)] on %d \r\n", *((uint32_t*)&Ra+1), *((uint32_t*)&Ra+0), *((uint32_t*)&Rb+1), *((uint32_t*)&Rb+0), *((uint32_t*)&Da+1), *((uint32_t*)&Da+0), *((uint32_t*)&Db+1), *((uint32_t*)&Db+0), inst_idist[src], src, sendtime/100);
        }
    }
    for (int i = 4; i < 539; i++) buf[539]  ^= buf[i];
    if (HAL_HSEM_FastTake(1) == HAL_OK)
    {
    	memcpy((uint8_t*)SHD_RAM_ADDR, buf, sizeof(buf));
    	HAL_HSEM_Release(1,(HSEM->CR & HSEM_CR_COREID) >> HSEM_CR_COREID_Pos);
    }
}

void anch_range_loop() {
    while(1) {
        if (flag_g == true) {
            flag_g = false;
            anch_calc_distance(txTimeStamp);
        }
        HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin);
        HAL_Delay(2);
    }
}

/**
 * @brief this function prepares and write response frame into the TX buffer
 */
void anch_prepare_anc2anc_response()
{
    instance_config_frameheader_16bit(inst);

    inst->psduLength = ANCH_FRAME_LEN;
    inst->msg_f.destAddr[0]   = 0xff;
    inst->msg_f.destAddr[1]   = 0xff;
    inst->msg_f.sourceAddr[0] = inst->shortAdd_idx;
    inst->msg_f.sourceAddr[1] = 0;
    inst->msg_f.seqNum        = inst->frameSN++;

    table_f[inst->shortAdd_idx][0].messageData[FSN] = inst->rangeNum++;

    if (inst->shortAdd_idx < NUM_EXPECTED_RESPONSES)
    	table_f[inst->shortAdd_idx][0].messageData[PSN] = inst->shortAdd_idx + 1;//Specify the next sender
    else
    	table_f[inst->shortAdd_idx][0].messageData[PSN] = 0;//The next sender of 19 is 0
}

/**
 * @brief this function either re-enables the receiver (delayed or immediate) or transmits the response frame
 */
void anch_txresponse_or_rxreenable(uint8_t src)
{
    dwt_readtxtimestamp(txTimeStamp);
    if (inst->rxResps == inst->shortAdd_idx || inst->remainingRespToRx == 0) //send frame
    {
#ifdef CAL_IN_TS
        //cale other 19 distance and write to frame
toggle_rf_switch(rf_switch);
        anch_calc_distance(txTimeStamp);
        memcpy(&table_f[inst->shortAdd_idx][0].messageData[RANGE + src * EVERY_RESULT_LENGTH], &inst_idist[src], EVERY_RESULT_LENGTH);
toggle_rf_switch(rf_switch);
#else
        //cale other 19 distance in while(1) loop
        memcpy(table_f1, table_f, sizeof(table_f));
        memset(inst_idist, 0, MAX_ANCHOR_LIST_SIZE * sizeof(double));
        memset(inst_aoa,   0, MAX_ANCHOR_LIST_SIZE * sizeof(int16_t));
        memset(buf,        0, sizeof(buf));
        flag_g = true;
#endif
        //back table_f[loc][0] to table_f[loc][1]
        memcpy(table_f[inst->shortAdd_idx][1].messageData, &table_f[inst->shortAdd_idx][0].messageData, ANCH_MESG_LEN);
        //read and fill previous frame txtime
        memcpy(&table_f[inst->shortAdd_idx][0].messageData[SENDTIME], txTimeStamp, EVERY_TIMESTAMP_LENGTH);

        //fill send frame header and other
        anch_prepare_anc2anc_response();

        //use table_f[0] built send frame data
        memcpy(inst->msg_f.messageData, table_f[inst->shortAdd_idx][0].messageData, ANCH_MESG_LEN);

         //send frame
        dwt_writetxdata(inst->psduLength, (uint8_t*)&inst->msg_f, 0);
        dwt_writetxfctrl(inst->psduLength, 0, 1);
        dwt_setrxtimeout(singleSlot / (128*512) * MAX_ANCHOR_LIST_SIZE);
        dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);
        sendtime = portGetTickCnt();
        memset(&table_f[inst->shortAdd_idx][0].messageData[RANGE], 0, MAX_ANCHOR_LIST_SIZE * EVERY_RESULT_LENGTH);
    }
    else //keep receive
    {
        timeout= inst->remainingRespToRx;
        dwt_setrxtimeout(singleSlot / (128 * 512) * timeout); //unit is 1.0256us, max 1.07541852s
        dwt_rxenable(DWT_START_RX_IMMEDIATE);
        if (DEBUG1 & 0x2) uart_printf("RR->%d\r\n\r\n\r\n",timeout);
    }
}

/**
 * @brief this is the receive event callback handler, the received event is processed and the instance either
 * responds by sending a response frame or re-enables the receiver to await the next frame
 */
void rx_ok_cb(const dwt_cb_data_t *rxd)
{
    int16_t cpqual;
    event_data_t dw_event;
    uint8_t rxTimeStamp[5] = {0, 0, 0, 0, 0};

    dwt_readrxtimestamp(rxTimeStamp);

    dwt_readrxdata((uint8_t *)&dw_event.msgu.frame[0], rxd->datalength, 0);
    uint16_t src = (((uint16_t)dw_event.msgu.frame[FRAME_CTRLP + ADDR_BYTE_SIZE_S + 1]) << 8) | dw_event.msgu.frame[FRAME_CTRLP + ADDR_BYTE_SIZE_S];

    recvtime_f[src][1] = recvtime_f[src][0];
    recvtime_f[src][0] = portGetTickCnt();

    //need to check if time to send response or go back to RX
    inst->rxResps = dw_event.msgu.frame[FRAME_CRTL_AND_ADDRESS_S + PSN];
    if (inst->rxResps < inst->shortAdd_idx)
        inst->remainingRespToRx = inst->shortAdd_idx - inst->rxResps;
    else if (inst->rxResps > inst->shortAdd_idx)
        inst->remainingRespToRx = MAX_ANCHOR_LIST_SIZE - (inst->rxResps - inst->shortAdd_idx);

    //first,back table_f[src][0] to table_f[src][1]
    memcpy(&table_f[src][1], &table_f[src][0], ANCH_FRAME_LEN);
    //second,save frame to table_f[src][0]
    memcpy(&table_f[src][0], dw_event.msgu.frame, ANCH_FRAME_LEN);

    if (THIS_DEV_TYPE == 0) //ANCHOR
    {
        if (rf_switch) inst_pdoa[src][0] = dwt_readstsquality(&cpqual) ? dwt_readpdoa() : 0;
        else           inst_pdoa[src][1] = dwt_readstsquality(&cpqual) ? dwt_readpdoa() : 0;

        //third,save receive time to table_f[loc][0].RECVTIME for next transmission
        memcpy(&table_f[inst->shortAdd_idx][0].messageData[RECVTIME + src * EVERY_TIMESTAMP_LENGTH], rxTimeStamp, EVERY_TIMESTAMP_LENGTH);
        //determine whether send or receive
        uart_printf("\r\n-----------%d\r\n", src);
        anch_txresponse_or_rxreenable(src);
    }
/*    else //TAG
    {
//        if(rf_switch) inst_pdoa[src][0] = dwt_readstsquality(&cpqual, 0) ? dwt_readpdoa() : 0;
        //keep rf switch in 0 to MAX_ANCHOR_LIST_SIZE
//      if(src < local_id) rf_toggle_switch();
        local_id = src;

	//back up table_f[tag][0].RECVTIME to table_f[tag][1].RECVTIME
	//write new receive time to table_f[tag][0].RECVTIME
	memcpy(&table_f[MAX_ANCHOR_LIST_SIZE][1].messageData[RECVTIME + src * EVERY_TIMESTAMP_LENGTH], &table_f[MAX_ANCHOR_LIST_SIZE][0].messageData[RECVTIME + src * EVERY_TIMESTAMP_LENGTH], EVERY_TIMESTAMP_LENGTH);
	memcpy(&table_f[MAX_ANCHOR_LIST_SIZE][0].messageData[RECVTIME + src * EVERY_TIMESTAMP_LENGTH], rxTimeStamp, EVERY_TIMESTAMP_LENGTH);
//	tag_calc_distance(src);
	dwt_rxenable(DWT_START_RX_IMMEDIATE);
    }   */
}

void rx_to_cb(const dwt_cb_data_t *cb_data)
{
    (void)cb_data;
    if (THIS_DEV_TYPE == 0 && inst->shortAdd_idx < MAX_ANCHOR_LIST_SIZE)//ANCHOR
    {
        inst->remainingRespToRx = 0;
        anch_txresponse_or_rxreenable(0);
    }
//    else//LISTEROR && TAG
//    {
//        dwt_rxenable(DWT_START_RX_IMMEDIATE);
//    }
}
#endif

void tx_done_cb(const dwt_cb_data_t *cb_data)
{
#if defined(DS_TWR_AOA_YW)
    toggle_rf_switch(rf_switch);
#endif
}

/**
 * @brief this is the receive error event callback handler
 */
void rx_err_cb(const dwt_cb_data_t *cb_data)
{
    dwt_setrxtimeout(0xFFFFF);
	dwt_rxenable(DWT_START_RX_IMMEDIATE);
}

/* ==========================================================

Notes:

Previously code handled multiple instances in a single console application

Now have changed it to do a single instance only. With minimal code changes...(i.e. kept [instance] index but it is always 0.

Windows application should call instance_init() once and then in the "main loop" call instance_run().

*/
