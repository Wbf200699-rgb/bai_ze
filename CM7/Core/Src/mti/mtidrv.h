/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */
#ifndef MTIDRV_H__
#define MTIDRV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx.h"
#include "stdio.h"
#include "string.h"

#define XS_PREAMBLE           0xFA
#define XS_EXTLENCODE         0xFF

uint16_t g_PacketCounter;
uint32_t g_SampleTimeFine;
uint32_t g_StatusWord;
uint32_t g_Pressure;
int16_t  g_EulerAngles[3];//X[0],Y[1],Z[2]
int16_t  g_Acceleration[3] ;//X[0],Y[1],Z[2]
int16_t  g_MagneticField[3];//X[0],Y[1],Z[2]

typedef struct
{
	uint8_t  preamble;
	uint8_t  bid;
	uint8_t  mid;
	uint8_t  len;
}MtiHead;


enum XbusMessageId {
	XMID_InvalidMessage           	= 0x00,

	// Config state messages
	XMID_ReqDid                   	= 0x00,
	XMID_DeviceId                 	= 0x01,
	XMID_Initbus                  	= 0x02,
	XMID_InitBusResults           	= 0x03,
	XMID_ReqPeriod                	= 0x04,
	XMID_ReqPeriodAck             	= 0x05,
	XMID_SetPeriod                	= 0x04,
	XMID_SetPeriodAck             	= 0x05,
	// XbusMaster
	XMID_SetBid                   	= 0x06,
	XMID_SetBidAck                	= 0x07,
	XMID_AutoStart                	= 0x06,
	XMID_AutoStartAck             	= 0x07,
	XMID_BusPower                 	= 0x08,
	XMID_BusPowerAck              	= 0x09,
	// End XbusMaster=
	XMID_ReqDataLength            	= 0x0A,
	XMID_DataLength               	= 0x0B,
	XMID_ReqConfiguration         	= 0x0C,
	XMID_Configuration            	= 0x0D,
	XMID_RestoreFactoryDef        	= 0x0E,
	XMID_RestoreFactoryDefAck     	= 0x0F,

	XMID_GotoMeasurement          	= 0x10,
	XMID_GotoMeasurementAck       	= 0x11,
	XMID_ReqFirmwareRevision      	= 0x12,
	XMID_FirmwareRevision         	= 0x13,
	// XbusMaster
	XMID_ReqBluetoothDisable      	= 0x14,
	XMID_ReqBluetoothDisableAck   	= 0x15,
	XMID_DisableBluetooth         	= 0x14,
	XMID_DisableBluetoothAck      	= 0x15,
	XMID_ReqXmOutputMode          	= 0x16,
	XMID_ReqXmOutputModeAck       	= 0x17,
	XMID_SetXmOutputMode          	= 0x16,
	XMID_SetXmOutputModeAck       	= 0x17,
	// End XbusMaster
	XMID_ReqBaudrate              	= 0x18,
	XMID_ReqBaudrateAck           	= 0x19,
	XMID_SetBaudrate              	= 0x18,
	XMID_SetBaudrateAck           	= 0x19,
	// XbusMaster
	XMID_ReqSyncMode              	= 0x1A,
	XMID_ReqSyncModeAck           	= 0x1B,
	XMID_SetSyncMode              	= 0x1A,
	XMID_SetSyncModeAck           	= 0x1B,
	// End XbusMaster
	XMID_ReqProductCode           	= 0x1C,
	XMID_ProductCode              	= 0x1D,

	XMID_ReqProcessingFlags       	= 0x20,
	XMID_ReqProcessingFlagsAck    	= 0x21,
	XMID_SetProcessingFlags       	= 0x20,
	XMID_SetProcessingFlagsAck    	= 0x21,

	XMID_SetNoRotation            	= 0x22,
	XMID_SetNoRotationAck         	= 0x23,

	XMID_RunSelfTest              	= 0x24,
	XMID_SelfTestResults          	= 0x25,

	XMID_GotoConfig               	= 0x30,
	XMID_GotoConfigAck            	= 0x31,
	XMID_BusData                  	= 0x32,
	XMID_MtData                   	= 0x32,

	XMID_ReqInputTrigger          	= 0x26,
	XMID_ReqInputTriggerAck       	= 0x27,
	XMID_SetInputTrigger          	= 0x26,
	XMID_SetInputTriggerAck       	= 0x27,

	XMID_ReqOutputTrigger         	= 0x28,
	XMID_ReqOutputTriggerAck      	= 0x29,
	XMID_SetOutputTrigger         	= 0x28,
	XMID_SetOutputTriggerAck      	= 0x29,

	XMID_SetSyncStationMode       	= 0x2A,
	XMID_SetSyncStationModeAck    	= 0x2B,
	XMID_ReqSyncStationMode       	= 0x2A,
	XMID_ReqSyncStationModeAck    	= 0x2B,
	// deprecated names
	XMID_SetSyncBoxMode           	= 0x2A,
	XMID_SetSyncBoxModeAck        	= 0x2B,
	XMID_ReqSyncBoxMode           	= 0x2A,
	XMID_ReqSyncBoxModeAck        	= 0x2B,
	// end of deprecated names

	XMID_SetSyncConfiguration       = 0x2C,
	XMID_SetSyncConfigurationAck    = 0x2D,
	XMID_ReqSyncConfiguration       = 0x2C,
	XMID_SyncConfiguration          = 0x2D,

	XMID_DriverDisconnect         	= 0x2E,
	XMID_DriverDisconnectAck      	= 0x2F,

	// Manual
	XMID_PrepareData              	= 0x32,
	XMID_ReqData                  	= 0x34,
	XMID_ReqDataAck               	= 0x35,

	XMID_MtData2                  	= 0x36,
	XMID_MtData2Ack	                = 0x37,

	XMID_RequestControl             = 0x38,
	XMID_RequestControlAck          = 0x39,

	XMID_SetDataPort                = 0x3A,
	XMID_SetDataPortAck             = 0x3B,

	XMID_ReqRetransmission			= 0x3C,
	XMID_ReqRetransmissionAck		= 0x3D,

	// Wakeup state messages
	XMID_Wakeup                   	= 0x3E,
	XMID_WakeupAck                	= 0x3F,

	// Valid in all states
	XMID_Reset                    	= 0x40,
	XMID_ResetAck                 	= 0x41,
	XMID_Error                    	= 0x42,
	// end Valid in all states

	// XbusMaster
	XMID_XmPowerOff               	= 0x44,
	// End XbusMaster

	// Wireless
	XMID_MasterIndication         	= 0x46,

	XMID_ReqOptionFlags             = 0x48,
	XMID_ReqOptionFlagsAck          = 0x49,
	XMID_SetOptionFlags             = 0x48,
	XMID_SetOptionFlagsAck          = 0x49,
	XMID_ReqStealthMode            	= 0x4A,
	XMID_StealthMode               	= 0x4B,
	XMID_SetStealthMode            	= 0x4A,
	XMID_SetStealthModeAck         	= 0x4B,

	XMID_UserInterface              = 0x4C,
	XMID_UserInterfaceAck           = 0x4D,

	XMID_EndOfRecording				= 0x4E,
	XMID_EndOfRecordingAck			= 0x4F,

	XMID_GotoTransparentMode	  	= 0x50,
	XMID_GotoTransparentModeAck	  	= 0x51,

	XMID_RunFactoryTest             = 0x56,
	XMID_FactoryTestResults         = 0x57,
	XMID_FactoryTestConnect         = 0x58,
	XMID_FactoryTestConnectAck      = 0x59,

	XMID_SetUtcTime               	= 0x60,
	XMID_ReqUtcTime               	= 0x60,
	XMID_SetUtcTimeAck             	= 0x61,
	XMID_UtcTime                  	= 0x61,
	XMID_FactoryTestSensorTiming		= 0x60,
	XMID_FactoryTestSensorTimingResults	= 0x61,

	XMID_ReqAvailableFilterProfiles	= 0x62,
	XMID_AvailableFilterProfiles	= 0x63,

	XMID_ReqFilterProfile          	= 0x64,
	XMID_ReqFilterProfileAck        = 0x65,
	XMID_SetFilterProfile           = 0x64,
	XMID_SetFilterProfileAck        = 0x65,

	XMID_ReqAvailableScenarios    	= XMID_ReqAvailableFilterProfiles,	//!< \deprecated The name 'Scenario' has been deprecated for this use in favor of the name 'FilterProfile'
	XMID_AvailableScenarios       	= XMID_AvailableFilterProfiles,		//!< \deprecated The name 'Scenario' has been deprecated for this use in favor of the name 'FilterProfile'
	XMID_ReqScenario              	= XMID_ReqFilterProfile,			//!< \deprecated The name 'Scenario' has been deprecated for this use in favor of the name 'FilterProfile'
	XMID_ReqScenarioAck           	= XMID_ReqFilterProfileAck,			//!< \deprecated The name 'Scenario' has been deprecated for this use in favor of the name 'FilterProfile'
	XMID_SetScenario              	= XMID_SetFilterProfile,			//!< \deprecated The name 'Scenario' has been deprecated for this use in favor of the name 'FilterProfile'
	XMID_SetScenarioAck           	= XMID_SetFilterProfileAck,			//!< \deprecated The name 'Scenario' has been deprecated for this use in favor of the name 'FilterProfile'

	XMID_ReqGravityMagnitude      	= 0x66,
	XMID_ReqGravityMagnitudeAck   	= 0x67,
	XMID_SetGravityMagnitude      	= 0x66,
	XMID_SetGravityMagnitudeAck   	= 0x67,

	XMID_ReqGnssLeverArm           	= 0x68,
	XMID_ReqGnssLeverArmAck        	= 0x69,
	XMID_SetGnssLeverArm           	= 0x68,
	XMID_SetGnssLeverArmAck        	= 0x69,

	XMID_ReqGpsLeverArm           	= XMID_ReqGnssLeverArm,		//!< \deprecated
	XMID_ReqGpsLeverArmAck        	= XMID_ReqGnssLeverArmAck,	//!< \deprecated
	XMID_SetGpsLeverArm           	= XMID_SetGnssLeverArm,		//!< \deprecated
	XMID_SetGpsLeverArmAck        	= XMID_SetGnssLeverArmAck,	//!< \deprecated

	XMID_ReqReplayMode				= 0x6C,
	XMID_ReqReplayModeAck			= 0x6D,
	XMID_SetReplayMode				= 0x6C,
	XMID_SetReplayModeAck			= 0x6D,

	XMID_ReqLatLonAlt             	= 0x6E,
	XMID_ReqLatLonAltAck          	= 0x6F,
	XMID_SetLatLonAlt             	= 0x6E,
	XMID_SetLatLonAltAck          	= 0x6F,

	// Xbus Master
	XMID_ReqXmErrorMode           	= 0x82,
	XMID_ReqXmErrorModeAck        	= 0x83,
	XMID_SetXmErrorMode           	= 0x82,
	XMID_SetXmErrorModeAck        	= 0x83,

	XMID_ReqBufferSize            	= 0x84,
	XMID_ReqBufferSizeAck         	= 0x85,
	XMID_SetBufferSize            	= 0x84,
	XMID_SetBufferSizeAck         	= 0x85,
	// End Xbus Master

	XMID_ReqHeading               	= 0x82,
	XMID_ReqHeadingAck            	= 0x83,
	XMID_SetHeading               	= 0x82,
	XMID_SetHeadingAck            	= 0x83,

	XMID_ReqMagneticField         	= 0x6A,
	XMID_ReqMagneticFieldAck      	= 0x6B,
	XMID_SetMagneticField         	= 0x6A,
	XMID_SetMagneticFieldAck      	= 0x6B,

	XMID_KeepAlive					= 0x70,
	XMID_KeepAliveAck				= 0x71,

	XMID_CloseConnection			= 0x72,
	XMID_CloseConnectionAck			= 0x73,

	XMID_IccCommand					= 0x74,
	XMID_IccCommandAck				= 0x75,

	XMID_ReqGnssPlatform			= 0x76,
	XMID_ReqGnssPlatformAck			= 0x77,
	XMID_SetGnssPlatform			= 0x76,
	XMID_SetGnssPlatformAck			= 0x77,

	XMID_BodyPackBundle				= 0x7A,
	XMID_BodyPackBundleAck			= 0x7B,

	XMID_ReqStationOptions			= 0x7C,
	XMID_ReqStationOptionsAck		= 0x7D,

	XMID_ReqLocationId            	= 0x84,
	XMID_ReqLocationIdAck         	= 0x85,
	XMID_SetLocationId            	= 0x84,
	XMID_SetLocationIdAck         	= 0x85,

	XMID_ReqExtOutputMode         	= 0x86,
	XMID_ReqExtOutputModeAck      	= 0x87,
	XMID_SetExtOutputMode         	= 0x86,
	XMID_SetExtOutputModeAck      	= 0x87,

	XMID_ReqStringOutputType		= 0x8E,
	XMID_ReqStringOutputTypeAck		= 0x8F,
	XMID_SetStringOutputType		= 0x8E,
	XMID_SetStringOutputTypeAck		= 0x8F,

	// XbusMaster
	XMID_ReqBatteryLevel          	= 0x88,
	XMID_Batterylevel             	= 0x89,
	// End XbusMaster

	XMID_ReqInitTrackMode         	= 0x88,
	XMID_ReqInitTrackModeAck      	= 0x89,
	XMID_SetInitTrackMode         	= 0x88,
	XMID_SetInitTrackModeAck      	= 0x89,

	XMID_ReqMasterSettings        	= 0x8A,
	XMID_MasterSettings           	= 0x8B,

	XMID_StoreFilterState          	= 0x8A,
	XMID_StoreFilterStateAck       	= 0x8B,

	XMID_ReqEmts                  	= 0x90,
	XMID_EmtsData                 	= 0x91,
	XMID_UpdateFilterProfile       	= 0x92,
	XMID_UpdateFilterProfileAck     = 0x93,

	XMID_RestoreEmts			  	= 0x94,
	XMID_RestoreEmtsAck			  	= 0x95,
	XMID_StoreEmts			      	= 0x96,
	XMID_StoreEmtsAck			  	= 0x97,

	XMID_AdjustUtcTime				= 0xA8,
	XMID_AdjustUtcTimeAck			= 0xA9,

	XMID_ReqActiveClockCorrection  	= 0x9C,
	XMID_ActiveClockCorrection    	= 0x9D,
	XMID_StoreActiveClockCorrection = 0x9E,
	XMID_StoreActiveClockCorrectionAck = 0x9F,

	XMID_ReqFilterSettings        	= 0xA0,
	XMID_ReqFilterSettingsAck     	= 0xA1,
	XMID_SetFilterSettings        	= 0xA0,
	XMID_SetFilterSettingsAck     	= 0xA1,
	XMID_ReqAmd                   	= 0xA2,
	XMID_ReqAmdAck                	= 0xA3,
	XMID_SetAmd                   	= 0xA2,
	XMID_SetAmdAck                	= 0xA3,
	XMID_ResetOrientation         	= 0xA4,
	XMID_ResetOrientationAck      	= 0xA5,

	XMID_ReqGnssStatus             	= 0xA6,
	XMID_GnssStatus                	= 0xA7,
	XMID_ReqGpsStatus             	= XMID_ReqGnssStatus,//!< \deprecated
	XMID_GpsStatus                	= XMID_GnssStatus,	//!< \deprecated

	XMID_ReqComponentsInformation	= 0xAA,
	XMID_ComponentsInformation		= 0xAB,

	XMID_ReqAccessControlList		= 0xAE,
	XMID_AccessControlList			= 0xAF,
	XMID_SetAccessControlList		= 0xAE,
	XMID_SetAccessControlListAck	= 0xAF,

	// Wireless
	XMID_ScanChannels             	= 0xB0,
	XMID_ScanChannelsAck          	= 0xB1,
	XMID_EnableMaster             	= 0xB2,
	XMID_EnableMasterAck          	= 0xB3,
	XMID_DisableMaster            	= 0xB4,
	XMID_DisableMasterAck         	= 0xB5,
	XMID_ReqRadioChannel			= 0xB6,
	XMID_ReqRadioChannelAck			= 0xB7,
	XMID_SetClientPriority        	= 0xB8,
	XMID_SetClientPriorityAck     	= 0xB9,
	XMID_ReqClientPriority        	= 0xB8,
	XMID_ReqClientPriorityAck     	= 0xB9,
	XMID_SetWirelessConfig        	= 0xBA,
	XMID_SetWirelessConfigAck     	= 0xBB,
	XMID_ReqWirelessConfig        	= 0xBA,
	XMID_ReqWirelessConfigAck     	= 0xBB,
	XMID_UpdateBias               	= 0xBC,
	XMID_UpdateBiasAck            	= 0xBD,
	XMID_ToggleIoPins				= 0xBE,
	XMID_ToggleIoPinsAck			= 0xBF,

	XMID_GotoOperational          	= 0xC0,
	XMID_GotoOperationalAck       	= 0xC1,

	XMID_SetTransportMode         	= 0xC2,
	XMID_SetTransportModeAck      	= 0xC3,
	XMID_ReqTransportMode         	= 0xC2,
	XMID_ReqTransportModeAck      	= 0xC3,

	XMID_AcceptMtw                	= 0xC4,
	XMID_AcceptMtwAck             	= 0xC5,
	XMID_RejectMtw                	= 0xC6,
	XMID_RejectMtwAck             	= 0xC7,
	XMID_InfoRequest              	= 0xC8,
	XMID_InfoRequestAck           	= 0xC9,

	XMID_ReqFrameRates            	= 0xCA,
	XMID_ReqFrameRatesAck         	= 0xCB,

	XMID_StartRecording           	= 0xCC,
	XMID_StartRecordingAck        	= 0xCD,
	XMID_StopRecording            	= 0xCE,
	XMID_StopRecordingAck         	= 0xCF,
	// End Wireless

	XMID_ReqOutputConfiguration   	= 0xC0,
	XMID_ReqOutputConfigurationAck	= 0xC1,
	XMID_SetOutputConfiguration   	= 0xC0,
	XMID_SetOutputConfigurationAck	= 0xC1,

	XMID_ReqOutputMode            	= 0xD0,
	XMID_ReqOutputModeAck         	= 0xD1,
	XMID_SetOutputMode            	= 0xD0,
	XMID_SetOutputModeAck         	= 0xD1,

	XMID_ReqOutputSettings        	= 0xD2,
	XMID_ReqOutputSettingsAck     	= 0xD3,
	XMID_SetOutputSettings        	= 0xD2,
	XMID_SetOutputSettingsAck     	= 0xD3,

	XMID_ReqOutputSkipFactor      	= 0xD4,
	XMID_ReqOutputSkipFactorAck   	= 0xD5,
	XMID_SetOutputSkipFactor      	= 0xD4,
	XMID_SetOutputSkipFactorAck   	= 0xD5,

	XMID_ReqSyncInSettings        	= 0xD6,
	XMID_ReqSyncInSettingsAck     	= 0xD7,
	XMID_SetSyncInSettings        	= 0xD6,
	XMID_SetSyncInSettingsAck     	= 0xD7,

	XMID_ReqSyncOutSettings       	= 0xD8,
	XMID_ReqSyncOutSettingsAck    	= 0xD9,
	XMID_SetSyncOutSettings       	= 0xD8,
	XMID_SetSyncOutSettingsAck    	= 0xD9,

	XMID_ReqErrorMode             	= 0xDA,
	XMID_ReqErrorModeAck          	= 0xDB,
	XMID_SetErrorMode             	= 0xDA,
	XMID_SetErrorModeAck          	= 0xDB,

	XMID_ReqTransmitDelay         	= 0xDC,
	XMID_ReqTransmitDelayAck      	= 0xDD,
	XMID_SetTransmitDelay         	= 0xDC,
	XMID_SetTransmitDelayAck      	= 0xDD,

	XMID_SetMfmResults              = 0xDE,
	XMID_SetMfmResultsAck           = 0xDF,

	XMID_ReqObjectAlignment       	= 0xE0,
	XMID_ReqObjectAlignmentAck    	= 0xE1,
	XMID_SetObjectAlignment       	= 0xE0,
	XMID_SetObjectAlignmentAck    	= 0xE1,

	XMID_ReqAlignmentRotation       = 0xEC,
	XMID_ReqAlignmentRotationAck    = 0xED,
	XMID_SetAlignmentRotation       = 0xEC,
	XMID_SetAlignmentRotationAck    = 0xED,

	XMID_ExtensionReserved1         = 0xEE,
	XMID_ExtensionReserved2         = 0xEF,

	XMID_SetDeviceIdContext			= 0xFE,
	XMID_SetDeviceIdContextAck		= 0xFF
};

enum XsDataIdentifier
{
	XDI_None					= 0x0000,	//!< Empty datatype
	XDI_TypeMask				= 0xFE00,	//!< Mask for checking the group which a dataidentifier belongs to, Eg. XDI_TimestampGroup or XDI_OrientationGroup
	XDI_FullTypeMask			= 0xFFF0,	//!< Mask to get the type of data, without the data format
	XDI_FullMask				= 0xFFFF,	//!< Complete mask to get entire data identifier
	XDI_FormatMask				= 0x01FF,	//!< Mask for getting the data id without checking the group
	XDI_DataFormatMask			= 0x000F,	//!< Mask for extracting just the data format /sa XDI_SubFormat

	XDI_SubFormatMask			= 0x0003,	//!< Determines, float, fp12.20, fp16.32, double output... (where applicable)
	XDI_SubFormatFloat			= 0x0000,	//!< Floating point format
	XDI_SubFormatFp1220			= 0x0001,	//!< Fixed point 12.20
	XDI_SubFormatFp1632			= 0x0002,	//!< Fixed point 16.32
	XDI_SubFormatDouble			= 0x0003,	//!< Double format

	XDI_TemperatureGroup		= 0x0800,	//!< Group for temperature outputs
	XDI_Temperature				= 0x0810,	//!< Temperature

	XDI_TimestampGroup			= 0x1000,	//!< Group for time stamp related outputs
	XDI_UtcTime					= 0x1010,	//!< Utc time from the GPS receiver
	XDI_PacketCounter			= 0x1020,	//!< Packet counter, increments every packet
	XDI_Itow					= 0x1030,	//!< Itow. Time Of Week from the GPS receiver
	XDI_GpsAge					= 0x1040,	//!< Age of Gps sample \deprecated Replaced by XDI_GnssAge
	XDI_GnssAge					= 0x1040,	//!< Gnss age from the GPS receiver
	XDI_PressureAge				= 0x1050,	//!< Age of a pressure sample, in packet counts
	XDI_SampleTimeFine			= 0x1060,	//!< Sample Time Fine
	XDI_SampleTimeCoarse		= 0x1070,	//!< Sample Time Coarse
	XDI_FrameRange				= 0x1080,	//!< Reserved \internal add for MTw (if needed)
	XDI_PacketCounter8			= 0x1090,	//!< 8 bit packet counter, wraps at 256
	XDI_SampleTime64			= 0x10A0,	//!< 64 bit sample time

	XDI_OrientationGroup		= 0x2000,	//!< Group for orientation related outputs
	XDI_CoordSysMask			= 0x000C,	//!< Mask for the coordinate system part of the orientation data identifier
	XDI_CoordSysEnu				= 0x0000,	//!< East North Up orientation output
	XDI_CoordSysNed				= 0x0004,	//!< North East Down orientation output
	XDI_CoordSysNwu				= 0x0008,	//!< North West Up orientation output
	XDI_Quaternion				= 0x2010,	//!< Orientation in quaternion format
	XDI_RotationMatrix			= 0x2020,	//!< Orientation in rotation matrix format
	XDI_EulerAngles				= 0x2030,	//!< Orientation in euler angles format

	XDI_PressureGroup			= 0x3000,	//!< Group for pressure related outputs
	XDI_BaroPressure			= 0x3010,	//!< Pressure output recorded from the barometer

	XDI_AccelerationGroup		= 0x4000,	//!< Group for acceleration related outputs
	XDI_DeltaV					= 0x4010,	//!< DeltaV SDI data output
	XDI_Acceleration			= 0x4020,	//!< Acceleration output in m/s2
	XDI_FreeAcceleration		= 0x4030,	//!< Free acceleration output in m/s2
	XDI_AccelerationHR			= 0x4040,	//!< AccelerationHR output

	XDI_PositionGroup			= 0x5000,	//!< Group for position related outputs
	XDI_AltitudeMsl				= 0x5010,	//!< Altitude at Mean Sea Level
	XDI_AltitudeEllipsoid		= 0x5020,	//!< Altitude at ellipsoid
	XDI_PositionEcef			= 0x5030,	//!< Position in earth-centered, earth-fixed format
	XDI_LatLon					= 0x5040,	//!< Position in latitude, longitude

	XDI_SnapshotGroup			= 0xC800,	//!< Group for snapshot related outputs
	XDI_RetransmissionMask		= 0x0001,	//!< Mask for the retransmission bit in the snapshot data
	XDI_RetransmissionFlag		= 0x0001,	//!< Bit indicating if the snapshot if from a retransmission
	XDI_AwindaSnapshot 			= 0xC810,	//!< Awinda type snapshot
	XDI_FullSnapshot 			= 0xC820,	//!< Full snapshot

	XDI_GnssGroup				= 0x7000,	//!< Group for Gnss related outputs
	XDI_GnssPvtData				= 0x7010,	//!< Gnss position, velocity and time data
	XDI_GnssSatInfo				= 0x7020,	//!< Gnss satellite information

	XDI_AngularVelocityGroup	= 0x8000,	//!< Group for angular velocity related outputs
	XDI_RateOfTurn				= 0x8020,	//!< Rate of turn data in rad/sec
	XDI_DeltaQ					= 0x8030,	//!< DeltaQ SDI data
	XDI_RateOfTurnHR			= 0x8040,	//!< Rate of turn HR data

	XDI_GpsGroup				= 0x8800,	//!< Group for GPS only related data \deprecated Replaced by XDI_GnssGroup
	XDI_GpsDop					= 0x8830,	//!< Gps dilution of precision data \deprecated
	XDI_GpsSol					= 0x8840,	//!< Gps navigation solution information \deprecated Replaced by XDI_GnssPvtData
	XDI_GpsTimeUtc				= 0x8880,	//!< Gps time in UTC format \deprecated Replaced by XDI_GnssPvtData
	XDI_GpsSvInfo				= 0x88A0,	//!< Gps satellite vehicle information \deprecated Replaced by XDI_GnssSatInfo

	XDI_RawSensorGroup			= 0xA000,	//!< Group for raw sensor data related outputs
	XDI_RawUnsigned				= 0x0000,	//!< Tracker produces unsigned raw values, usually fixed behavior
	XDI_RawSigned				= 0x0001,	//!< Tracker produces signed raw values, usually fixed behavior
	XDI_RawAccGyrMagTemp		= 0xA010,	//!< Raw acceleration, gyroscope, magnetometer and temperature data
	XDI_RawGyroTemp				= 0xA020,	//!< Raw gyroscope and temperature data
	XDI_RawAcc					= 0xA030,	//!< Raw acceleration data
	XDI_RawGyr					= 0xA040,	//!< Raw gyroscope data
	XDI_RawMag					= 0xA050,	//!< Raw magnetometer data
	XDI_RawDeltaQ				= 0xA060,	//!< Raw deltaQ SDI data
	XDI_RawDeltaV				= 0xA070,	//!< Raw deltaV SDI data
	XDI_RawBlob					= 0xA080,	//!< Raw blob data

	XDI_AnalogInGroup			= 0xB000,	//!< Group for analog in related outputs
	XDI_AnalogIn1				= 0xB010,	//!< Data containing adc data from analog in 1 line (if present)
	XDI_AnalogIn2				= 0xB020,	//!< Data containing adc data from analog in 2 line (if present)

	XDI_MagneticGroup			= 0xC000,	//!< Group for magnetometer related outputs
	XDI_MagneticField			= 0xC020,	//!< Magnetic field data in a.u.

	XDI_VelocityGroup			= 0xD000,	//!< Group for velocity related outputs
	XDI_VelocityXYZ				= 0xD010,	//!< Velocity in XYZ coordinate frame

	XDI_StatusGroup				= 0xE000,	//!< Group for status related outputs
	XDI_StatusByte				= 0xE010,	//!< Status byte
	XDI_StatusWord				= 0xE020,	//!< Status word
	XDI_Rssi					= 0xE040,	//!< Rssi information
	XDI_DeviceId				= 0xE080,	//!< DeviceId output

	XDI_IndicationGroup			= 0x4800,	//!< 0100.1000 -> bit reverse = 0001.0010 -> type 18
	XDI_TriggerIn1				= 0x4810,	//!< Trigger in 1 indication
	XDI_TriggerIn2				= 0x4820,	//!< Trigger in 2 indication
};

#ifdef __cplusplus
}
#endif

#endif
