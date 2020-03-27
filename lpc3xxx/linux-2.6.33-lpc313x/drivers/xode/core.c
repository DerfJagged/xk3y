#include <linux/completion.h>
#include <mach/fpga.h>
#include <mach/gpio.h>


#include <linux/ktime.h>
#include <linux/sched.h>


#include "core.h"
#include "corestates.h"
#include "corestims.h"

/*** Driver Globals ***/

extern  struct completion crccomplete;

extern  struct completion readcomplete;
extern 	XODE_FPGA_REGS_t * xode_reg;

extern unsigned char task_file[];

CommandProtocol_t CommandProtocol;
unsigned int TransferLength;

/* HW/SW reset fired */
int Aborted 	= 0;
/* Pkt command optimisation */
int Packeted 	= 0;
/* Command aborted, SK */
unsigned long AbortParam;

/* User Land is up flag, clr the bsy flag */
static int ulup;

State_t State;

unsigned char * statename[] = {
	"DI0   ",
	"DI1   ",
	"DIA   ",

	/* Figure 46 - Device Non-Data state diagram */
	"DND0  ",

	/* Figure 48 - Device PIO data-In state diagram */
	"DPIOI0",
	"DPIOI1",
	"DPIOI2",

  	/* Figure 54 - Device PACKET non-data and PIO data command state diagram */
  	"DP0   ",
	"DP1   ",
	"DP2   ",
	"DP4IN ",
	"DP4OUT",

	/* Figure 56 - Device PACKET DMA command state diagram */
	"DPD0  ",
	"DPD1  ",
	"DPD2  ",
	"DPD4IN",
	"DPD4OUT",

};

unsigned char * stimname[] = {
	"SoftwareReset",
	"HardwareReset",
	"BusRelease",

	"ServiceWritten",
	"ServiceRead",
	
	"CommandPacketTransferComplete",
	"CommandComplete",
	
	"CommandProtocolSelected",
	
	"CommandAborted",
	
	"CommandRegisterWritten",
	"DeviceHeadRegisterWritten",

	"StatusnIENSet",
	"StatusnIENClr",
		
	"StatusRegisterRead",
	
	"ReadyToRxCommandPacket",
	"ReadyToTransfer",
	
	"PIOXferDone",
	
	"DMATransferOutComplete",
	"LPCTransferComplete",

	"DACKGranted",
	"DMADataIn",
	"DMADataOut",
	"DMADataNone",

	"CRCFail",
	"CRCOk",

	"DataOutOk",
	"DataOutCheckStatus",
	"DataOutHang",
	"PacketOpt",
};


//#define SDBG
#ifdef SDBG


#define STATE_DBG()				printk("%s : %s\n", statename[State],stimname[Stim]);



#else

#define STATE_DBG()

#endif


#define DPDBG
#ifdef  DPDBG
typedef struct
{
	Stim_t Stim;
	State_t State;
	unsigned int rd;
	char tbuf[50];
}SSDbg_t;

SSDbg_t SSDbg[0x10];
int ssidx;
static int ictr = 0;
#endif

#define STATE_HANDLER(NAME)		static void Handle_##NAME(Stim_t Stim)


#define STATE(NAME)				case State_##NAME:						\
									Handle_##NAME(Stim);				\
									break;

#define DO_STIM(NAME)			void Do_##NAME( void )					\
								{										\
									CoreStateMachine( stim_##NAME );				\
								}

/*
When executing a power-on, hardware, DEVICE RESET, or software reset, a device implementing the PACKET
Command feature set performs the same reset protocol as other devices but leaves the registers with a
signature unique to PACKET Command feature set devices (see 9.12).

In addition, the IDENTIFY DEVICE command shall not be executed but shall be command aborted and shall
return a signature unique to devices implementing the PACKET Command feature set. The IDENTIFY PACKET
DEVICE command is used by the host to get identifying parameter information for a device implementing the
PACKET Command feature set (see 8.12.5.2 and 8.13).
*/





#define CD_x() 
#define CD_0() 
#define CD_1() 

#define IO_x() 
#define IO_0() 
#define IO_1() 


#define nIEN()				0


#define WriteSectorCount(X)		xode_reg->sector_count	= X;
#define WriteSectorNumber(X)	xode_reg->sector_number = X;
#define WriteCylinderLow(X)		xode_reg->cylinder_low 	= X;
#define WriteCylinderHigh(X)	xode_reg->cylinder_high = X;
#define WriteDeviceHead(X)		xode_reg->device_select = X;

#define ReadDeviceHead()	0

extern void xode_dma_transfer( int type );


/*************************************************************
setup FPGA to receive command packet - PIO 
*************************************************************/
void SetupFPGAToReceiveCommandPacket( void )
{
}
/*************************************************************
*************************************************************/
int ServiceInterruptEnabled( void )
{
	return 1; 
}
/*************************************************************
*************************************************************/
static void WritePacketSignature(int maskdevbit)
{
	unsigned char DH = 0x00;
	
	WriteSectorCount(0x01);
	WriteSectorNumber(0x01);
	WriteCylinderLow(0x14);
	WriteCylinderHigh(0xEB);

	/* if executing DEVICE RESET we need to protect the DEV bit */
	if(maskdevbit)
	{
		DH = (ReadDeviceHead() & 0x10);
	}
	WriteDeviceHead(DH);
}
/****************************************************************
* Figure 43 - Device bus Idle state diagram *
****************************************************************/
/*** Device_Idle_SI ***/
STATE_HANDLER(DI0)
{
	STATE_DBG()
	switch(Stim)
	{
		/* DI0_DI0 */
		case stim_DeviceHeadRegisterWritten:
			State = State_DI0;
			/* Update device status */
			break;

		case stim_StatusRegisterRead:
		case stim_StatusnIENSet:
			/* we always assert INTRQ on error */
			CLR_ERROR()
			State = State_DI1;
			/* Update device status */
			DRQ_0()
			DSC_DRDY()
			break;

		case stim_CommandRegisterWritten:
			/* complete xode_read, push it up to UL */
			State = State_DIA;
			complete(&readcomplete);
			DSC_DRDY()
			CLR_ERROR()	
			break;

		default:
			break;
	}
}
/*** Device_Idle_S ***/
STATE_HANDLER(DI1)
{
	STATE_DBG()
	switch(Stim)
	{
		/* DI1_DI1 */
		case stim_DeviceHeadRegisterWritten:
			State = State_DI1;
			break;

		case stim_CommandRegisterWritten:
			/* complete xode_read, push it up to UL */
			State = State_DIA;
			/* Packet commands are handled immediately */
			Packeted = (task_file[14] == 0xA0) ? 1 : 0;
			complete(&readcomplete);
			DSC_DRDY()
			CLR_ERROR()	
			break;


		default:
			break;
	}
}
/*** Device_Idle_S ***/
STATE_HANDLER(DIA)
{
	STATE_DBG()
	switch(Stim)
	{

		case stim_PacketOpt:
			State = (task_file[2] & 0x01) ? State_DPD1 : State_DP1;
			
			xode_reg->error_reg 	= 0x00; /* 1 */
			xode_reg->sector_count	= 0x01; 			/*IO = 0, CD = 1 */
			xode_reg->sector_number = 0x00;
			xode_reg->cylinder_low	= 0x0C;
			xode_reg->cylinder_high = 0x00; /*	*/
			xode_reg->device_select = 0x00;
			
			DSC_DRDY()
			xode_reg->fpga_status = STS_DRQ;
			BSY_0()
			break;


		case stim_CommandProtocolSelected:
			/* Select Command Protocol - comes from UL via IOCTL*/
			switch(CommandProtocol & 0xFF)
			{
				case CommandProtocol_VendorIntroLiteon:
					VENDOR_LITEON((CommandProtocol >> 8) & 0xFF)
					BSY_0()
					State = State_DI0;
					INTRQ_A()
					break;

				case CommandProtocol_VendorIntroBenq:
					VENDOR_BENQ((CommandProtocol >> 8) & 0xFF)	/* Should be 0x73 */
					BSY_0()
					State = State_DI0;
					INTRQ_A()
					break;

				case CommandProtocol_VendorIntroSamsung:
					VENDOR_SAMSUNG((CommandProtocol >> 8) & 0xFF)	
					BSY_0()
					State = State_DI0;
					INTRQ_A()
					break;


				case CommandProtocol_Abort:
					ABORT()
					DRQ_0()
					DSC_DRDY()
					BSY_0()
					State = State_DI0;
					INTRQ_A()
					break;
					
				case CommandProtocol_NoData:
					State = State_DND0;
					break;
				case CommandProtocol_PIODataIn:
					State = State_DPIOI0;
					DRQ_0()
					DSC_DRDY()
					break;
				case CommandProtocol_Packet:
					State = State_DP0;
					break;
				case CommandProtocol_PacketDMA:
					State = State_DPD0;
					break;
			}
			break;
		default:
			break;
	}
}

/****************************************************************
* Figure 46 - Non-data command protocol *
****************************************************************/
/*** Command_Execution ***/
STATE_HANDLER(DND0)
{
	STATE_DBG()
	switch(Stim)
	{
		case stim_CommandComplete:
			DRQ_0()
			DSC_DRDY()
			BSY_0()
			if(nIEN())
			{
				State = State_DI1;
			}
			else
			{
				State = State_DI0;
				INTRQ_A()
			}
			break;

		default:
			break;
	}
}

/****************************************************************
* Figure 48 - Device PIO data-In state diagram *
****************************************************************/
/* Prepare_Data */
STATE_HANDLER(DPIOI0)
{
	STATE_DBG()
	switch(Stim)
	{
		case stim_CommandAborted:
			DRQ_0()
			DSC_DRDY()
			if(nIEN())
			{
				State = State_DI1;
			}
			else
			{
				State = State_DI0;
				INTRQ_A()
			}
			break;

		case stim_ReadyToTransfer:
			State = State_DPIOI2;
			xode_dma_transfer(DMA_IN);

			DSC_DRDY()
			CLR_ERROR()

			xode_reg->sector_count = 0;
			
			break;

		default:
			break;
	}
}
/* Transfer_Data */
STATE_HANDLER(DPIOI1)
{
	STATE_DBG()
	switch(Stim)
	{
		/* DRQ data block transferred */
#if 0
		case stim_DRQCleared:			/* Remove! */
			State = State_DI1;
			DRQ_0()
			DSC_DRDY()
			break;
#endif
		default:
			break;
	}
}
/* Data_Rdy_INTRQ */
STATE_HANDLER(DPIOI2)
{
	STATE_DBG()
	switch(Stim)
	{
		case stim_LPCTransferComplete:
//			DRQ_1()
			xode_reg->fpga_status = STS_DRQ;
			BSY_0()
			INTRQ_A()
			break;


		case stim_StatusRegisterRead:
			break;

		case stim_PIOXferDone:
			State = State_DI0;			/* no DRQ cleared any more, back to IDLE */
			DSC_DRDY()
			DRQ_0()
			BSY_0()
			break;

		default:
			break;
	}
}

/****************************************************************
* Figure 54 - Device PACKET non-data and PIO data command state diagram *
****************************************************************/
/*** Prepare_A ***/
STATE_HANDLER(DP0)
{
	STATE_DBG()
	switch(Stim)
	{
		case stim_ReadyToRxCommandPacket:
			State = State_DP1; /* clear bsy */
			xode_reg->sector_count	= 0x01;				/*IO = 0, CD = 1 */
			DRQ_1() 
			DSC_DRDY()
			BSY_0() 		
			break;

		default:
			break;
	}
}
/*** Receive_packet ***/
STATE_HANDLER(DP1)
{
	STATE_DBG()
	switch(Stim)
	{
		case stim_CommandPacketTransferComplete:
			DRQ_0()
			State = State_DP2;
			complete(&readcomplete);
			break;
	
		default:
			break;
	}
}
/*** Prepare_B ***/
STATE_HANDLER(DP2)
{
	STATE_DBG()
	switch(Stim)
	{
		case stim_DMADataIn:
			State = State_DP4IN;
			xode_reg->sector_count	= 0x02;				/*IO = 1, CD = 0 */
			xode_dma_transfer(PIO_IN);

			/* stay busy until DP4IN is completed or we get another cmd...*/
			break;

		case stim_DMADataOut:
			State = State_DP4OUT;
//printk("PIO OUT %X\n",TransferLength);
			xode_reg->cylinder_low	= (TransferLength >> 0) & 0xFF;
			xode_reg->cylinder_high	= (TransferLength >> 8) & 0xFF;

			xode_reg->fpga_dma_length = (TransferLength / 2) - 1;
			xode_reg->sector_count	= 0x00;				/*IO = 0, CD = 0 */

			xode_reg->fpga_status = STS_DRQ;
			BSY_0()
			DSC_DRDY()
			INTRQ_A()
			break;

		case stim_CommandAborted:

			DSC_DRDY()
			DRQ_0()
			xode_reg->error_reg 	= (AbortParam << 4) | 4; 
			xode_reg->sector_count	= 0x03;				/*IO = 1, CD = 1 */
			xode_reg->sector_number = 0x00;
			BSY_0()
			if(nIEN())
			{
				State = State_DI1;
			}
			else
			{
				State = State_DI0;
				INTRQ_A()
			}
			break;

		case stim_DMADataNone:
			xode_reg->sector_count	= 0x03;				/*IO = 1, CD = 1 */
			DSC_DRDY()
			DRQ_0() 
			BSY_0()
			if(nIEN())
			{
				State = State_DI1;
			}
			else
			{
				State = State_DI0;
				INTRQ_A()
			}
			break;

		default:
			break;
	}
}
/*** Ready_INTRQ ***/
STATE_HANDLER(DP4IN)
{
	STATE_DBG()
	switch(Stim)
	{
		case stim_LPCTransferComplete:
			xode_reg->cylinder_low  = (TransferLength >> 0) & 0xFF;
			xode_reg->cylinder_high = (TransferLength >> 8) & 0xFF;		  
			DSC_DRDY()
			xode_reg->fpga_status = STS_DRQ;	//No transferlength kludge
			BSY_0()
			INTRQ_A()
			break;

		case stim_StatusRegisterRead:
			break;
			
		case stim_PIOXferDone:
			State = State_DI0;
			xode_reg->sector_count	= 0x03;				/*IO = 1, CD = 1 */
			INTRQ_A()
			break;

		default:
			break;
	}
}
/*** Transfer_Data ***/
STATE_HANDLER(DP4OUT)
{
	STATE_DBG()
	switch(Stim)
	{
		case stim_PIOXferDone:
			DRQ_0()
			xode_dma_transfer(PIO_OUT);
			break;
			
		case stim_LPCTransferComplete:
			complete(&readcomplete);
			break;

		case stim_DataOutOk:
			State = State_DI0;
			xode_reg->error_reg		= 0x00;	/* 1 */
			xode_reg->sector_count	= 0x03;
			xode_reg->sector_number	= 0x00;
			xode_reg->cylinder_low	= (TransferLength >> 0) & 0xFF;
			xode_reg->cylinder_high	= (TransferLength >> 8) & 0xFF;
			xode_reg->device_select	= 0x00;
			DSC_DRDY()
			BSY_0()
			INTRQ_A()	
			break;

		case stim_DataOutChk:
			State = State_DI0;
			xode_reg->error_reg		= 0x04;	/* 1 */
			xode_reg->sector_count	= 0x03;
			xode_reg->sector_number	= 0x00;
			xode_reg->cylinder_low	= (TransferLength >> 0) & 0xFF;
			xode_reg->cylinder_high	= (TransferLength >> 8) & 0xFF;
			xode_reg->device_select	= 0x00;
			DSC_DRDY()
			BSY_0()
			INTRQ_A()	
			break;
			
		default:
			break;
	}
}

/*************************************************
* Figure 56 - Device PACKET DMA command state diagram *

DPD0 : ReadyToRxCommandPacket		=> DPD1
DPD1 : CommandPacketTransferComplete	=> DPD2 
DPD2 : DMADataOut					=> DPD4OUT
DPD4OUT : DACKGranted				=> DPD4OUT
DPD4OUT : DMATransferOutComplete		=> DPD4OUT
DPD4OUT : LPCTransferComplete			=> DI1


*************************************************/
/*** Prepare_A ***/
STATE_HANDLER(DPD0)
{
	STATE_DBG()
	switch(Stim)
	{
		case stim_ReadyToRxCommandPacket:
			State = State_DPD1;	

			xode_reg->error_reg 	= 0x00; /* 1 */
			xode_reg->sector_count	= 0x01;				/*IO = 0, CD = 1 */
			xode_reg->sector_number = 0x00;
			xode_reg->cylinder_low	= 0x0C;
			xode_reg->cylinder_high = 0x00; /*	*/
			xode_reg->device_select = 0x00;

			DSC_DRDY()
			xode_reg->fpga_status = STS_DRQ;
			BSY_0()
			break;

		default:
			break;
	}
}
/*** Receive_packet ***/
STATE_HANDLER(DPD1)
{
	STATE_DBG()
	switch(Stim)
	{
		case stim_CommandPacketTransferComplete:
			State = State_DPD2;
			DRQ_0()
			complete(&readcomplete);
			break;

		default:STATE_DBG()
			break;
	}
}
/*** Prepare_B ***/
STATE_HANDLER(DPD2)
{
	STATE_DBG()
	switch(Stim)
	{
		/* from user land */
		case stim_DMADataIn:
			State = State_DPD4IN;
			xode_reg->error_reg 	= 0x00; /* 1 */
			xode_reg->sector_count	= 0x02;
			xode_reg->sector_number = 0x00;
			xode_reg->cylinder_low	= 0x00;
			xode_reg->cylinder_high = 0x00; /*	*/
			xode_reg->device_select = 0x00;

			DSC_DRDY()
			DMARQ_A()		/*  */
			break;

		case stim_DMADataOut:
			State = State_DPD4OUT;
			DSC_DRDY()
			DMARQ_A()		/* includes DRQ_1 + BSY_0 */
			break;
			
		case stim_CommandAborted:
			DSC_DRDY()
			DRQ_0()
			xode_reg->error_reg		= (AbortParam << 4) | 4; 		//AbortParam << 4;	
			xode_reg->sector_count	= 0x03;
			xode_reg->sector_number = 0x00;
			BSY_0()
			if(nIEN())
			{
				State = State_DI1;
			}
			else
			{
				State = State_DI0;
				INTRQ_A()
			}
			break;

		case stim_DMADataNone:
			xode_reg->sector_count = 0x0003;
			DSC_DRDY()
			DRQ_0()
			BSY_0()
			if(nIEN())
			{
				State = State_DI1;
			}
			else
			{
				State = State_DI0;
				INTRQ_A()
			}
			break;

		default:STATE_DBG()
			break;
	}
}

/*** Transfer_Data ***
Order is
-DACK Granted
-DRQ Cleared
-LPC Transfer Complete
********************/
STATE_HANDLER(DPD4IN)
{
	STATE_DBG()
	switch(Stim)
	{
		case stim_DACKGranted:
			xode_dma_transfer(DMA_IN);
			break;

		case stim_LPCTransferComplete:
			DRQ_0()
			complete(&crccomplete);
			break;

		case stim_CRCOk:
			State = State_DI0;
			xode_reg->error_reg		= 0x00;	/* 1 */
			xode_reg->sector_count	= 0x03;
			xode_reg->sector_number	= 0x00;
			xode_reg->cylinder_low	= 0x00;
			xode_reg->cylinder_high	= 0x00;	
			xode_reg->device_select	= 0x00;

			DSC_DRDY()
			BSY_0() 
			INTRQ_A()
			break;

		case stim_CRCFail:
			State = State_DI0;
			ICRC()
			DSC_DRDY()
			BSY_0()
			INTRQ_A()		
			break;

			
		default:STATE_DBG()
			break;
	}
}

/*** Transfer_Data ***
Order is
-DACK Granted
-DRQ Cleared
-DMA Transfer Out Complete
-LPC Transfer Complete
********************/
STATE_HANDLER(DPD4OUT)
{
	STATE_DBG()
	switch(Stim)
	{
		case stim_DACKGranted:
			xode_reg->fpga_dma_length = (TransferLength / 2) - 1;
			DMAOUTSTART()
			break;
	
		case stim_DMATransferOutComplete:
			DRQ_0()
			complete(&crccomplete);
			xode_dma_transfer(DMA_OUT);
			break;
			
		case stim_LPCTransferComplete:
			complete(&readcomplete);
			break;
			
		case stim_CRCOk:
			xode_reg->error_reg		= 0x00;	/* 1 */
			xode_reg->sector_count	= 0x03;
			xode_reg->sector_number	= 0x00;
			xode_reg->cylinder_low	= 0x00;
			xode_reg->cylinder_high	= 0x00;	
			xode_reg->device_select	= 0x00;
			DSC_DRDY()
			break;

		case stim_DataOutOk:
			State = State_DI0;
			BSY_0()
			INTRQ_A()	
			break;

		case stim_DataOutHang:
			break;
			
		case stim_CRCFail:
			State = State_DI0; 
			ICRC()
			DSC_DRDY()
			BSY_0()
			INTRQ_A()
			break;

		default:STATE_DBG()
			break;
	}
}

/***************************************************
***************************************************/
static void CoreStateMachine(Stim_t Stim)
{
#ifdef DPDBG	
	unsigned long long t;
	unsigned long nanosec_rem;

	t = cpu_clock(UINT_MAX);
	nanosec_rem = do_div(t, 1000000000);
	sprintf(SSDbg[ssidx].tbuf, "[%5lu.%06lu] ",
			(unsigned long) t,
			nanosec_rem / 1000);

	SSDbg[ssidx].Stim = Stim;
	SSDbg[ssidx].rd = readcomplete.done;

	SSDbg[ssidx++].State = State;
	ssidx &= 0x0F;
	ictr++;
#endif
	switch(State)
	{
		/* Figure 43 - Device bus Idle state diagram */
		STATE(DI0)
		STATE(DI1)
		STATE(DIA)

		/* Figure 46 - Device Non-Data state diagram */
		STATE(DND0)

		/* Figure 48 - Device PIO data-In state diagram */
		STATE(DPIOI0)
		STATE(DPIOI1)
		STATE(DPIOI2)

		/* Figure 54 - Device PACKET / PIO IN command state diagram */
		STATE(DP0)
		STATE(DP1)
		STATE(DP2)
		STATE(DP4IN)
		STATE(DP4OUT)
	
		/* Figure 56 - Device PACKET DMA command state diagram */
		STATE(DPD0)
		STATE(DPD1)
		STATE(DPD2)
		STATE(DPD4IN)
		STATE(DPD4OUT)
	}
}
/***************************************************
***************************************************/
void CoreUserLandUp( void )
{
	BSY_0()
	ulup = 1;	
}
/***************************************************
***************************************************/
void CoreSetTrayState( unsigned long param )
{
	if(ulup)
	{
		if(param)
			xode_reg->fpga_alt_cause = 0x000C;
		else
			xode_reg->fpga_alt_cause = 0x0008;
	}
}

/***************************************************
***************************************************/
void CoreHWReset( void )
{
#ifdef DPDBG
	int i;
	for(i=0;i<0x10;i++)
	{
		printk("%s : %02X : %X : %s : %s\n", 
			SSDbg[(ssidx + i) & 0x0F].tbuf,
		i,
			SSDbg[(ssidx + i) & 0x0F].rd,
			statename[SSDbg[(ssidx + i) & 0x0F].State],
			stimname[SSDbg[(ssidx + i) & 0x0F].Stim]);	
	}
	printk("rdc.done %d state %s icnt %d\n",readcomplete.done,statename[State],ictr);

#if 0
	int j;
	for(i=0;i<0x40;i++)
	{
		printk("%02X: ",i * 0x10);
		for(j=0;j<0x10;j++)
		{
			printk("%04X ",xode_reg->fpga_dbgfifo); 
		}	
		printk("\n");
	}

#endif
#endif
#if 1
	/* abort any current command  */
	Aborted = 1;

	complete(&readcomplete); 

	complete(&crccomplete); 

	State = State_DI1;
	WritePacketSignature(0);

	CLR_ERROR()
	DRQ_0()
	DSC_DRDY()
	BSY_0()
#endif
}
/***************************************************
***************************************************/
void InitCoreStateMachine( void )
{
#ifdef DPDBG
	ssidx = 0;
#endif
	Aborted = 0;
	State = State_DI1;
	WritePacketSignature(0);

	CLR_ERROR()
	DRQ_0()
	DSC_DRDY()

	if(ulup)
	{
		BSY_0()
	}
}
/***************************************************
***************************************************/
DO_STIM(SoftwareReset)
DO_STIM(HardwareReset)
DO_STIM(BusRelease)

DO_STIM(ServiceWritten)
DO_STIM(ServiceRead)

DO_STIM(CommandPacketTransferComplete)
DO_STIM(CommandComplete)

DO_STIM(CommandProtocolSelected)

DO_STIM(CommandAborted)

DO_STIM(CommandRegisterWritten)
DO_STIM(DeviceHeadRegisterWritten)

DO_STIM(StatusnIENSet)
DO_STIM(StatusnIENClr)

DO_STIM(StatusRegisterRead)

DO_STIM(ReadyToRxCommandPacket)
DO_STIM(ReadyToTransfer)

DO_STIM(PIOXferDone)

DO_STIM(DMATransferOutComplete)
DO_STIM(LPCTransferComplete)

DO_STIM(DACKGranted)

DO_STIM(DMADataIn)
DO_STIM(DMADataOut)
DO_STIM(DMADataNone)

DO_STIM(CRCFail)
DO_STIM(CRCOk)

DO_STIM(DataOutOk)
DO_STIM(DataOutChk)
DO_STIM(DataOutHang)

DO_STIM(PacketOpt)



