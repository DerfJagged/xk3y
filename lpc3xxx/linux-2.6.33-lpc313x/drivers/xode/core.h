#ifndef __ATA_H__
#define __ATA_H__

#define DMA_IN	0
#define DMA_OUT	1
#define PIO_IN	2
#define PIO_OUT	3


#define STIM(NAME)				Do_##NAME( );

#define DO_STIM_PROTO(NAME)		void Do_##NAME( void );

DO_STIM_PROTO(SoftwareReset)
DO_STIM_PROTO(SoftwarewareReset)
DO_STIM_PROTO(HardwareReset)
DO_STIM_PROTO(BusRelease)

DO_STIM_PROTO(ServiceWritten)
DO_STIM_PROTO(ServiceRead)

DO_STIM_PROTO(CommandPacketTransferComplete)
DO_STIM_PROTO(CommandComplete)

DO_STIM_PROTO(CommandProtocolSelected)

DO_STIM_PROTO(CommandAborted)

DO_STIM_PROTO(CommandRegisterWritten)
DO_STIM_PROTO(DeviceHeadResisterWritten)

DO_STIM_PROTO(StatusnIENSet)
DO_STIM_PROTO(StatusnIENClr)

DO_STIM_PROTO(StatusRegisterRead)

DO_STIM_PROTO(ReadyToRxCommandPacket)
DO_STIM_PROTO(ReadyToTransfer)

DO_STIM_PROTO(PIOXferDone)

DO_STIM_PROTO(DMATransferOutComplete)
DO_STIM_PROTO(LPCTransferComplete)

DO_STIM_PROTO(DACKGranted)
DO_STIM_PROTO(DMADataIn)
DO_STIM_PROTO(DMADataOut)
DO_STIM_PROTO(DMADataNone)

DO_STIM_PROTO(CRCFail)
DO_STIM_PROTO(CRCOk)

DO_STIM_PROTO(DataOutOk)
DO_STIM_PROTO(DataOutChk)
DO_STIM_PROTO(DataOutHang)

DO_STIM_PROTO(PacketOpt)


typedef enum
{
	CommandProtocol_Abort = 0,
	CommandProtocol_NoData,
	CommandProtocol_PIODataIn,
	CommandProtocol_Packet,
	CommandProtocol_PacketDMA,
	CommandProtocol_VendorIntroLiteon,
	CommandProtocol_VendorIntroBenq,
	CommandProtocol_VendorIntroSamsung,
}CommandProtocol_t;


void InitCoreStateMachine( void );
void CoreHWReset( void );
void CoreUserLandUp( void );
void CoreSetTrayState( unsigned long param );
#endif

