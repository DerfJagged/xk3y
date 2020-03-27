
#include <asm/arch/nand.h>

#include "anticlone.h"
#include "ac_keys.h"

#define AES_BUF	((void*)0x70000000)
#define LEN	16

/*
 * To prevent cloning, the OTP contains 16 bytes of anti-clone data,
 * which is created by using the chip serial ID as IV and encrypting
 * ac_plaintext using ac_aeskey.
 */

int check_anticlone(unsigned int const serial[], char const acdata[])
{
	int res, i;
	
	for (i = 0; i < 4; i++) {
		NAND_CTRL->aes_iv[i] = serial[0];
		NAND_CTRL->aes_key[i] = ac_aeskey[i];
	}
	
	NAND_CTRL->aes_from_ahb = NAND_AES_AHB_EN;
	
	memcpy(AES_BUF, acdata, LEN);
	
	NAND_CTRL->aes_from_ahb = NAND_AES_AHB_EN|NAND_AES_AHB_DCRYPT_RAM0;
	
	while (!(NAND_CTRL->irq_status_raw & NAND_IRQ_AES_DONE_RAM0));
	
	NAND_CTRL->irq_status_raw = NAND_IRQ_AES_DONE_RAM0;
	
	res = !!memcmp(AES_BUF, ac_plaintext, LEN);
	
	NAND_CTRL->aes_from_ahb = 0;
	
	return res;
}
