#ifndef __FPGA_H__
#define __FPGA_H__


/* Interrupt Causes */

#define FPGA_INT_DMA_OUT_COMPLETE				0x0002

#define FPGA_INT_PIO_XFER_DONE					0x0010

#define FPGA_INT_PKT_WRITTEN					0x0020
#define FPGA_INT_CMD_WRITE						0x0040

#define FPGA_INT_ALT_CAUSE						0x0200
//#define FPGA_INT_DRQ_CLEARED					0x0400
#define FPGA_INT_INTRQ_CLEARED					0x0800

#define FPGA_INT_DEV_RESET						0x2000
#define FPGA_INT_IDE_RESET						0x4000
#define FPGA_INT_DACK_GRANTED					0x8000


#define FPGA_INT_ALL 	(	FPGA_INT_DMA_OUT_COMPLETE	|	\
							FPGA_INT_PKT_WRITTEN		|	\
							FPGA_INT_CMD_WRITE			|	\
							FPGA_INT_PIO_XFER_DONE		|	\
							FPGA_INT_INTRQ_CLEARED		|	\
							FPGA_INT_DEV_RESET			|	\
							FPGA_INT_IDE_RESET			|	\
							FPGA_INT_DACK_GRANTED			)


#define STS_DMA_OUT_START	(0x01)

#define STS_IDX		(0x02)
#define STS_RDRQ	(0x04)
#define STS_DRQ		(0x08)
#define STS_DSC		(0x10)
#define STS_DWF		(0x20)
#define STS_DRDY	(0x40)
#define STS_BSY		(0x80)


#define STS_DREQ	(0x0100)
#define STS_INTRQ	(0x1000)

#define ERR_ABORT	(0x04)
#define ERR_ICRC	(0x80)



#define BSY_0() 		xode_reg->fpga_status = STS_BSY;


#define DRQ_0()			xode_reg->fpga_status = STS_RDRQ;
#define DRQ_1() 		xode_reg->fpga_dma_length = 2;		\
						xode_reg->fpga_status = STS_DRQ;


#define INTRQ_A() 		xode_reg->fpga_status = STS_INTRQ;	

#define DMARQ_N()
#define DMARQ_A()		xode_reg->fpga_status = STS_DREQ;

#define DMAOUTSTART()	xode_reg->fpga_status = STS_DMA_OUT_START | STS_RDRQ;

/* direct access to status/command reg */




#define VENDOR_LITEON(P)	xode_reg->status_command_reg 	= (STS_DSC | STS_DWF | STS_DRDY | STS_IDX);		\
							xode_reg->data_reg				= 0;											\
							xode_reg->error_reg				= 0;			  								\
							xode_reg->sector_count			= 0; 		  									\
							xode_reg->sector_number			= (P);											\
							xode_reg->cylinder_low			= 0; 		  									\
							xode_reg->cylinder_high			= 0;		  									\
							xode_reg->device_select			= 0;

/*FIXME: use error reg = 0x100*/
#define VENDOR_BENQ(P)		xode_reg->status_command_reg 	= (STS_DSC | STS_DWF | STS_DRDY | STS_IDX);		\
							xode_reg->error_reg 			= 0x20;											\
							xode_reg->sector_number			= (P);

#define VENDOR_SAMSUNG(P)	xode_reg->status_command_reg 	= (STS_DSC | STS_DWF | STS_DRDY);				\
							xode_reg->sector_number			= (P);

#define DSC_DRDY() 			xode_reg->status_command_reg 	= (STS_DSC | STS_DRDY);

#define CLR_ERROR() 		xode_reg->error_reg 			= 0;
#define ABORT() 			xode_reg->error_reg 			= ERR_ABORT;
#define ICRC() 				xode_reg->error_reg 			= ERR_ICRC | ERR_ABORT;


typedef volatile struct
{
  volatile unsigned char 	pad[0x400]; 			// FPGA_REG_ADDRESS

  volatile unsigned short	data_reg;				//+0x400
  volatile unsigned short	error_reg;				//+0x402
  volatile unsigned short	sector_count;			//+0x404
  volatile unsigned short	sector_number;			//+0x406
  volatile unsigned short	cylinder_low;			//+0x408
  volatile unsigned short	cylinder_high;			//+0x40A
  volatile unsigned short	device_select;			//+0x40C
  volatile unsigned short	status_command_reg; 	//+0x40E
	
  volatile unsigned short	pad3[0x08]; 			//+0x410

  volatile unsigned short	shadow[0x10];			//+0x420	
  
  volatile unsigned short	fpga_status;			//+0x440

  volatile unsigned short	fpga_dma_length;		//+0x442

  volatile unsigned char	pad4[0x0C]; 			//+0x444

  volatile unsigned short	fpga_crc;				//+0x450	actually the CRC calced by the GL chip!!!
  volatile unsigned short	fpga_crc_status;		//+0x452	

  volatile unsigned short	fpga_id; 				//+0x454

  volatile unsigned short	fpga_alt_cause;			//+0x456
  volatile unsigned short	fpga_trigger;			//+0x458	hacked to allow read of FPGA CRC calc
  volatile unsigned short	fpga_dbgfifo;			//+0x45A

} XODE_FPGA_REGS_t;


#endif
