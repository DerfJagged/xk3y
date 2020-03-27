
#include <asm/arch/i2c.h>
#include <asm/arch/ioconf.h>
#include <asm/arch/clock.h>

#include "otp.h"
#include "revocount.h"

#define OTP_MODE_WRITE	(0x2 << 16)
#define OTP_MODE_COPY	(0x1 << 16)

// GPIO 16 == bit 10
static unsigned int const VPP = (1 << 10);

static void vpp_set_read(void)
{
	IOCONF->block[IOCONF_GPIO].mode1_set = VPP;
	IOCONF->block[IOCONF_GPIO].mode0_set = VPP;
	
	udelay(10000);
}

static void vpp_set_write(void)
{
	IOCONF->block[IOCONF_GPIO].mode1_set = VPP;
	IOCONF->block[IOCONF_GPIO].mode0_clear = VPP;

	udelay(10000);
}

static void program_bit(unsigned int bit)
{
	OTP.con = (bit & 0x1FF);
	udelay(6);
	OTP.con = (bit & 0x1FF) | OTP_MODE_WRITE;
	udelay(6);
	OTP.con = (bit & 0x1FF);
}

void otp_init()
{
	/* enable OTP clock */
	cgu_clk_en_dis(CGU_SB_OTP_PCLK_ID, 1);
	cgu_clk_set_exten(CGU_SB_OTP_PCLK_ID, 0);
	
	// set read protect on uboot key
	OTP.rprot = 0xF0 | (1 << 31);

	vpp_set_read();
}

int otp_update_revocount()
{
	int count = MAX_REVO_COUNT;
	
	vpp_set_write();
	
	OTP.wprot &= ~(1 << 8);
	
	while (count--)
		program_bit(256 + count);
	
	vpp_set_read();
	
	return 0;
}

int otp_program(struct otp const *otp)
{
	// otp content starts at nxp 'serial' field
	char const *p = (char*)otp->_reserved0;
	unsigned int i;
	
	vpp_set_write();

	// unprotect everything except serial
	OTP.wprot = 0xF;
	
	// program all rows except serial
	for (i = 128; i < 512; i++)
		if (p[i >> 3] & (1 << (i & 7)))
			program_bit(i);
	
	vpp_set_read();

	return 0;
}
