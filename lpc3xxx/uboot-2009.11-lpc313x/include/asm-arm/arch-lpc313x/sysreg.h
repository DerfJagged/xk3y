/*
 * (C) Copyright 2010 NXP Semiconductors
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef SYSREG_H
#define SYSREG_H

#include <common.h>
#include <asm/arch/hardware.h>

#ifdef __cplusplus
extern "C"
{
#endif

/***********************************************************************
* System Control (SysCReg) Register Structures
**********************************************************************/
typedef volatile struct
{
	volatile ulong spare_reg0;
	volatile ulong activate_testpins;
	volatile ulong ebi_ip2024_1;
	volatile ulong ebi_ip2024_2;
	volatile ulong ebi_ip2024_3;
	volatile ulong ccp_ring_osc_cfg;
	volatile ulong ssa1_adc_pd_adc10bits;
	volatile ulong cgu_dyn_hp0;
	volatile ulong cgu_dyn_hp1;
	volatile ulong abc_cfg;
	volatile ulong sd_mmc_cfg;
	volatile ulong mci_delaymodes;
	volatile ulong usb_atx_pll_pd_reg;
	volatile ulong usb_otg_cfg;
	volatile ulong usb_otg_port_ind_ctl;
	volatile ulong sys_usb_tpr_dyn;
	volatile ulong usb_pll_ndec;
	volatile ulong usb_pll_mdec;
	volatile ulong usb_pll_pdec;
	volatile ulong usb_pll_selr;
	volatile ulong usb_pll_seli;
	volatile ulong usb_pll_selp;
	volatile ulong isram0_latency_cfg;
	volatile ulong isram1_latency_cfg;
	volatile ulong isrom_latency_cfg;
	volatile ulong ahb_mpmc_pl172_misc;
	volatile ulong mpmp_delaymodes;
	volatile ulong mpmc_waitread_delay0;
	volatile ulong mpmc_waitread_delay1;
	volatile ulong wire_ebi_msize_init;
	volatile ulong mpmc_testmode0;
	volatile ulong mpmc_testmode1;
	volatile ulong ahb0_extprio;
	volatile ulong arm926_shadow_pointer;
	volatile ulong sleepstatus;
	volatile ulong chip_id;
	volatile ulong mux_lcd_ebi_sel;
	volatile ulong mux_gpio_mci_sel;
	volatile ulong mux_nand_mci_sel;
	volatile ulong mux_uart_spi_sel;
	volatile ulong mux_dao_ipint_sel;
	volatile ulong ebi_d_9_pctrl;
	volatile ulong ebi_d_10_pctrl;
	volatile ulong ebi_d_11_pctrl;
	volatile ulong ebi_d_12_pctrl;
	volatile ulong ebi_d_13_pctrl;
	volatile ulong ebi_d_14_pctrl;
	volatile ulong dai_bck0_pctrl;
	volatile ulong mgpio9_pctrl;
	volatile ulong mgpio6_pctrl;
	volatile ulong mlcd_db_7_pctrl;
	volatile ulong mlcd_db_4_pctrl;
	volatile ulong mlcd_db_2_pctrl;
	volatile ulong mnand_rybn0_pctrl;
	volatile ulong gpio1_pctrl;
	volatile ulong ebi_d_4_pctrl;
	volatile ulong mdao_clk0_pctrl;
	volatile ulong mdao_bck0_pctrl;
	volatile ulong ebi_a_1_cle_pctrl;
	volatile ulong ebi_ncas_blout_0_pctrl;
	volatile ulong nand_ncs_3_pctrl;
	volatile ulong mlcd_db_0_pctrl;
	volatile ulong ebi_dqm_0_noe_pctrl;
	volatile ulong ebi_d_0_pctrl;
	volatile ulong ebi_d_1_pctrl;
	volatile ulong ebi_d_2_pctrl;
	volatile ulong ebi_d_3_pctrl;
	volatile ulong ebi_d_5_pctrl;
	volatile ulong ebi_d_6_pctrl;
	volatile ulong ebi_d_7_pctrl;
	volatile ulong ebi_d_8_pctrl;
	volatile ulong ebi_d_15_pctrl;
	volatile ulong dao_data1_pctrl;
	volatile ulong dao_bck1_pctrl;
	volatile ulong dao_ws1_pctrl;
	volatile ulong dai_data0_pctrl;
	volatile ulong dai_ws0_pctrl;
	volatile ulong dai_data1_pctrl;
	volatile ulong dai_bck1_pctrl;
	volatile ulong dai_ws1_pctrl;
	volatile ulong sysclk_o_pctrl;
	volatile ulong pwm_data_pctrl;
	volatile ulong uart_rxd_pctrl;
	volatile ulong uart_txd_pctrl;
	volatile ulong i2c_sda1_pctrl;
	volatile ulong i2c_scl1_pctrl;
	volatile ulong clk_256fs_o_pctrl;
	volatile ulong gpio0_pctrl;
	volatile ulong gpio2_pctrl;
	volatile ulong gpio3_pctrl;
	volatile ulong gpio4_pctrl;
	volatile ulong gpio_tst_0_dd_pctrl;
	volatile ulong gpio_tst_1_dd_pctrl;
	volatile ulong gpio_tst_2_dd_pctrl;
	volatile ulong gpio_tst_3_dd_pctrl;
	volatile ulong gpio_tst_4_dd_pctrl;
	volatile ulong gpio_tst_5_dd_pctrl;
	volatile ulong gpio_tst_6_dd_pctrl;
	volatile ulong gpio_tst_7_dd_pctrl;
	volatile ulong ad_nint_i_pctrl;
	volatile ulong play_det_i_pctrl;
	volatile ulong spi_miso_pctrl;
	volatile ulong spi_mosi_pctrl;
	volatile ulong spi_cs_in_pctrl;
	volatile ulong spi_sck_pctrl;
	volatile ulong spi_cs_out0_pctrl;
	volatile ulong nand_ncs_0_pctrl;
	volatile ulong nand_ncs_1_pctrl;
	volatile ulong nand_ncs_2_pctrl;
	volatile ulong mlcd_csb_pctrl;
	volatile ulong mlcd_db_1_pctrl;
	volatile ulong mlcd_e_rd_pctrl;
	volatile ulong mlcd_rs_pctrl;
	volatile ulong mlcd_rw_wr_pctrl;
	volatile ulong mlcd_db_3_pctrl;
	volatile ulong mlcd_db_5_pctrl;
	volatile ulong mlcd_db_6_pctrl;
	volatile ulong mlcd_db_8_pctrl;
	volatile ulong mlcd_db_9_pctrl;
	volatile ulong mlcd_db_10_pctrl;
	volatile ulong mlcd_db_11_pctrl;
	volatile ulong mlcd_db_12_pctrl;
	volatile ulong mlcd_db_13_pctrl;
	volatile ulong mlcd_db_14_pctrl;
	volatile ulong mlcd_db_15_pctrl;
	volatile ulong mgpio5_pctrl;
	volatile ulong mgpio7_pctrl;
	volatile ulong mgpio8_pctrl;
	volatile ulong mgpio10_pctrl;
	volatile ulong mnand_rybn1_pctrl;
	volatile ulong mnand_rybn2_pctrl;
	volatile ulong mnand_rybn3_pctrl;
	volatile ulong muart_cts_n_pctrl;
	volatile ulong muart_rts_n_pctrl;
	volatile ulong mdao_data0_pctrl;
	volatile ulong mdao_ws0_pctrl;
	volatile ulong ebi_nras_blout_1_pctrl;
	volatile ulong ebi_a_0_ale_pctrl;
	volatile ulong ebi_nwe_pctrl;
	volatile ulong eshctrl_sup4;
	volatile ulong eshctrl_sup8;
} SYSCREG_REGS_T;

/***********************************************************************
 * SYSREGS Pad control register definitions
 **********************************************************************/
#define SYSREG_PCTRL_PULLUP	_SBF(1, 0x00)
#define SYSREG_PCTRL_PULLDOWN	_SBF(1, 0x03)
#define SYSREG_PCTRL_RPTR	_SBF(1, 0x01)
#define SYSREG_PCTRL_INPUT	_SBF(1, 0x02)

/* Macro pointing to SysRegs registers */
#define SYS_REGS	((SYSCREG_REGS_T *)(SYSCREG_BASE))

#ifdef __cplusplus
}
#endif

#endif /* SYSREG_H */
