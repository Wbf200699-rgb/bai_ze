/*! ----------------------------------------------------------------------------
 *  @file    instance.h
 *  @brief   DecaWave header for application level instance
 *
 * @attention
 *
 * Copyright 2015 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */
#ifndef _INSTANCE_H_
#define _INSTANCE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"
#include "deca_types.h"
#include "deca_device_api.h"

#define SPEED_OF_LIGHT     			  (299702547.0)

#define STANDARD_FRAME_SIZE           (127)
#define COMPATIBLE_FRAME_SIZE         (1023)

#define ADDR_BYTE_SIZE_L              (8)
#define ADDR_BYTE_SIZE_S              (2)

#define FRAME_CONTROL_BYTES           (2)
#define FRAME_SEQ_NUM_BYTES           (1)
#define FRAME_PANID                   (2)
#define FRAME_CRC					  (2)
#define FRAME_SOURCE_ADDRESS_S        (ADDR_BYTE_SIZE_S)
#define FRAME_DEST_ADDRESS_S          (ADDR_BYTE_SIZE_S)
#define FRAME_SOURCE_ADDRESS_L        (ADDR_BYTE_SIZE_L)
#define FRAME_DEST_ADDRESS_L          (ADDR_BYTE_SIZE_L)
#define FRAME_CTRLP					  (FRAME_CONTROL_BYTES + FRAME_SEQ_NUM_BYTES + FRAME_PANID)     //5
#define FRAME_CRTL_AND_ADDRESS_L      (FRAME_DEST_ADDRESS_L + FRAME_SOURCE_ADDRESS_L + FRAME_CTRLP) //21 bytes for 64-bit addresses)
#define FRAME_CRTL_AND_ADDRESS_S      (FRAME_DEST_ADDRESS_S + FRAME_SOURCE_ADDRESS_S + FRAME_CTRLP) //9 bytes for 16-bit addresses)
#define FRAME_CRTL_AND_ADDRESS_LS	  (FRAME_DEST_ADDRESS_L + FRAME_SOURCE_ADDRESS_S + FRAME_CTRLP) //15 bytes for one 16-bit address and one 64-bit address)
#define MAX_USER_PAYLOAD_STRING_SS    (COMPATIBLE_FRAME_SIZE - FRAME_CRTL_AND_ADDRESS_S - FRAME_CRC)//1023 - 9  - 2

#define BLINK_FRAME_CONTROL_BYTES     (1)
#define BLINK_FRAME_SEQ_NUM_BYTES     (1)
#define BLINK_FRAME_CRC				  (FRAME_CRC)
#define BLINK_FRAME_SOURCE_ADDRESS    (ADDR_BYTE_SIZE_L)
#define BLINK_FRAME_CTRLP			  (BLINK_FRAME_CONTROL_BYTES + BLINK_FRAME_SEQ_NUM_BYTES) //2
#define BLINK_FRAME_CRTL_AND_ADDRESS  (BLINK_FRAME_SOURCE_ADDRESS + BLINK_FRAME_CTRLP) //10 bytes
#define BLINK_FRAME_LEN_BYTES         (BLINK_FRAME_CRTL_AND_ADDRESS + BLINK_FRAME_CRC)

#define MAX_ANCHOR_LIST_SIZE		  (20)
#define NUM_EXPECTED_RESPONSES		  (MAX_ANCHOR_LIST_SIZE - 1)
#define EVERY_TIMESTAMP_LENGTH        (5)
#define EVERY_RESULT_LENGTH           (0)

#define FSN                           (0)               //0 //Frame number position
#define PSN                           (FSN + 1)         //1 //Next sender indication position, (shortAdd_idx + 1)
#define RANGE                         (PSN + 1)			//2 //Distance initial position, each 4 bytes(including 2 bytes distance and 2 bytes angle)
#define SENDTIME                      (RANGE + EVERY_RESULT_LENGTH * MAX_ANCHOR_LIST_SIZE)    //82	//Poll time position, 5 bytes
#define RECVTIME                      (SENDTIME + EVERY_TIMESTAMP_LENGTH)     //87 //Response time position,each 5bytes

#define ANCH_MESG_LEN       	      (RECVTIME + EVERY_TIMESTAMP_LENGTH * MAX_ANCHOR_LIST_SIZE) //107B
#define ANCH_FRAME_LEN                (FRAME_CRTL_AND_ADDRESS_S + ANCH_MESG_LEN + FRAME_CRC) //9+107+2=118B
#define TX_ANT_DLY                    (16385)
#define RX_ANT_DLY                    (16385)

typedef struct
{
	uint16_t rxLength;      // length of RX data (does not apply to TX events)
	uint64_t timeStamp;     // last timestamp (Tx or Rx) - 40 bit DW3000 time
	uint32_t timeStamp32l;  // last tx/rx timestamp - low 32 bits of the 40 bit DW3000 time
	uint32_t timeStamp32h;  // last tx/rx timestamp - high 32 bits of the 40 bit DW3000 time
	union {
		uint8_t frame[COMPATIBLE_FRAME_SIZE]; //holds received frame (after a good RX frame event)
	}msgu;
}event_data_t;

typedef struct
{
    uint8_t frameCtrl[2];                         	//  frame control bytes 00-01
    uint8_t seqNum;                               	//  sequence_number 02
    uint8_t panID[2];                             	//  PAN ID 03-04
    uint8_t destAddr[ADDR_BYTE_SIZE_S];             //  05-06
    uint8_t sourceAddr[ADDR_BYTE_SIZE_S];           //  07-08
    uint8_t messageData[MAX_USER_PAYLOAD_STRING_SS];//  09-1020 (application data and any user payload)
    uint8_t fcs[2];                              	//  1021-1022  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} srd_msg_dsss;

typedef struct
{
	uint16_t instanceAddress16; //contains tag/anchor 16 bit address
	uint16_t psduLength;        //used for storing the TX frame length
    uint8_t  frameSN;           //modulo 256 frame sequence number - it is incremented for each new frame transmission
	uint16_t panID;             // panid used in the frames
	uint8_t  shortAdd_idx;
	uint8_t	 rangeNum;          //incremented for each sequence of ranges (each slot)
    int8_t   rxResps;           //how many responses were received to a poll (in current ranging exchange)
    uint8_t  remainingRespToRx;	//how many responses should to be received (in current ranging exchange)
	srd_msg_dsss msg_f ;       //ranging message frame with 16-bit addresses
} instance_data_t;

extern instance_data_t* inst;
extern instance_data_t  instance;
extern srd_msg_dsss table_f[MAX_ANCHOR_LIST_SIZE + 1][2];
extern srd_msg_dsss table_f1[MAX_ANCHOR_LIST_SIZE + 1][2];

extern int16_t    inst_pdoa[MAX_ANCHOR_LIST_SIZE][2];
extern uint16_t   inst_tdist[MAX_ANCHOR_LIST_SIZE][MAX_ANCHOR_LIST_SIZE];

extern uint32_t sendtime;
extern uint32_t recvtime_f[MAX_ANCHOR_LIST_SIZE][2];

void rx_ok_cb(const dwt_cb_data_t *cb_data);
void rx_to_cb(const dwt_cb_data_t *cb_data);
void rx_err_cb(const dwt_cb_data_t *cb_data);
void tx_done_cb(const dwt_cb_data_t *cb_data);

#define DS_TWR_AOA_YW

void anch_range_loop();

#ifdef __cplusplus
}
#endif

#endif
