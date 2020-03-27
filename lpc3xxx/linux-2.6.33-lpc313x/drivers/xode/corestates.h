#ifndef __CORE_STATES_H__
#define __CORE_STATES_H__

/*** States ***/

typedef enum
{
	/* Figure 43 - Device bus Idle state diagram */
	State_DI0,			/* Device_Idle_SI */
	State_DI1,			/* Device_Idle_S */
	State_DIA,			/* Waiting for response from UL */

	/* Figure 46 - Device Non-Data state diagram */
	State_DND0,			/* Command_Execution */

	/* Figure 48 - Device PIO data-In state diagram */
	State_DPIOI0,		/* Prepare_Data */
	State_DPIOI1,		/* Transfer_Data */
	State_DPIOI2,		/* Data_Rdy_INTRQ */

  	/* Figure 54 - Device PACKET non-data and PIO data command state diagram */
  	State_DP0,
	State_DP1,
	State_DP2,
	State_DP4IN,
	State_DP4OUT,
	
	/* Figure 56 - Device PACKET DMA command state diagram */
	State_DPD0,			/* Prepare_A */
	State_DPD1,			/* Receive_packet */
	State_DPD2,			/* Prepare_B */
	State_DPD4IN,		/* Transfer_Data */
	State_DPD4OUT, 		/* Transfer_Data */

}State_t;

#endif
