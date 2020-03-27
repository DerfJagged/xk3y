#ifndef __CORESTIMS_H__
#define __CORESTIMS_H__

typedef enum
{
	stim_SoftwareReset,
	stim_HardwareReset,
	stim_BusRelease,

	stim_ServiceWritten,
	stim_ServiceRead,

	stim_CommandPacketTransferComplete,
	stim_CommandComplete,

	/* Comes from UL via IOCTL */
	stim_CommandProtocolSelected,

	stim_CommandAborted,

	stim_CommandRegisterWritten,
	stim_DeviceHeadRegisterWritten,

	stim_StatusnIENSet,
	stim_StatusnIENClr,

	stim_StatusRegisterRead,

	stim_ReadyToRxCommandPacket,
	stim_ReadyToTransfer,

	stim_PIOXferDone,

	stim_DMATransferOutComplete,
	stim_LPCTransferComplete,
	
	stim_DACKGranted,
	stim_DMADataIn,
	stim_DMADataOut,
	stim_DMADataNone,
	stim_CRCFail,
	stim_CRCOk,

	stim_DataOutOk,
	stim_DataOutChk,
	stim_DataOutHang,

	stim_PacketOpt
}Stim_t;

#endif
