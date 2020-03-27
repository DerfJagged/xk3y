/*
 * lpc3143_aes.c - Linux cryptoapi driver for LPC3143 built-in
 *                 AES decryption engine
 */

#include <crypto/aes.h>
#include <crypto/algapi.h>
#include <linux/crypto.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>

#include <mach/cgu.h>
#include <mach/dma.h>

#define NandIRQStatus1		0x00/4
#define NandIRQMask1		0x04/4
#define NandIRQStatusRaw1	0x08/4
#define NandConfig		0x0C/4
#define	KEY			0x54/4
#define	IV			0x64/4
#define AES_FROM_AHB 		0x7C/4

#define	CHUNK			512

//~ #define ASYNC

static struct lpc3143_aes_priv
{
	unsigned long __iomem *regs;
	unsigned long phys_sram[2];
	int irq;
	int dma_ch;

#ifdef ASYNC
	struct crypto_queue queue;
	spinlock_t qlock;
	struct task_struct *workers[2];
#endif
} aeshw;

struct lpc3143_aes_instance
{
	unsigned int key[4];
	unsigned int round_key[44];
};

static DECLARE_WAIT_QUEUE_HEAD(aesreqq);

static DEFINE_MUTEX(aeshw_mtx);

static DECLARE_COMPLETION(aescomplete);

static void encrypt_block(unsigned int block[], unsigned int const rk[]);
static void expand_key(unsigned char const *key, unsigned int round_key[]);

static void lpc3143_aes_dma_irq(int ch, dma_irq_type_t dtype, void *handle)
{
	// not used
}

static void lpc3143_dmacpy(unsigned long dest, unsigned long src, int len)
{
	dma_setup_t tx =
	{
		.cfg		= DMA_CFG_TX_BURST,
		.trans_length	= (len >> 4) - 1,
		.src_address	= src,
		.dest_address	= dest
	};
	
	dma_prog_channel(aeshw.dma_ch, &tx);
	dma_start_channel(aeshw.dma_ch);

	while (dma_channel_enabled(aeshw.dma_ch));
}

static irqreturn_t lpc3143_aes_irq(int irq, void *priv)
{
	unsigned int l = readl(&aeshw.regs[NandIRQStatus1]);

	writel(l, &aeshw.regs[NandIRQStatusRaw1]);

	complete(&aescomplete);
	
	return IRQ_HANDLED;
}

static int lpc3143_aes_setkey(struct crypto_ablkcipher *cipher,
	const u8 *key, unsigned int len)
{
	struct crypto_tfm *tfm = crypto_ablkcipher_tfm(cipher);
	struct lpc3143_aes_instance *ctx = crypto_tfm_ctx(tfm);

	memcpy(ctx->key, key, sizeof(ctx->key));
	expand_key(key, ctx->round_key);

	return 0;
}

static int lpc3143_handle_req(int idx, struct ablkcipher_request *req)
{
	unsigned int *iv = req->info;
	unsigned int *k = 
		((struct lpc3143_aes_instance *)crypto_tfm_ctx(req->base.tfm))->key;
	long l;

	if (req->nbytes != CHUNK)
	{
		printk(KERN_CRIT "lpc3143_aes: data size != chunk size!!\n");
		return -EFAULT;
	}

	mutex_lock(&aeshw_mtx);

	lpc3143_dmacpy(aeshw.phys_sram[idx], sg_phys(req->src), CHUNK);

	writel(k[0], &aeshw.regs[KEY+0]);
	writel(k[1], &aeshw.regs[KEY+1]);
	writel(k[2], &aeshw.regs[KEY+2]);
	writel(k[3], &aeshw.regs[KEY+3]);
	
	writel(iv[0], &aeshw.regs[IV+0]);
	writel(iv[1], &aeshw.regs[IV+1]);
	writel(iv[2], &aeshw.regs[IV+2]);
	writel(iv[3], &aeshw.regs[IV+3]);
		
	writel(NAND_AES_AHB_EN |
		(idx ? NAND_AES_AHB_DCRYPT_RAM1 : NAND_AES_AHB_DCRYPT_RAM0),
		&aeshw.regs[AES_FROM_AHB]);
	
#ifdef ASYNC
	l = wait_for_completion_timeout(&aescomplete, HZ/20);
#else
	for (l = 50; l && !try_wait_for_completion(&aescomplete); --l);
#endif		
	INIT_COMPLETION(aescomplete);	
		
	if (!l)
	{
		// timed out
		printk(KERN_CRIT "lpc3143_aes: IRQ never arrived!\n");
		mutex_unlock(&aeshw_mtx);
		return -EFAULT;
	}
	
	lpc3143_dmacpy(sg_phys(req->dst), aeshw.phys_sram[idx], CHUNK);
	
	mutex_unlock(&aeshw_mtx);

	return 0;
}

#ifdef ASYNC

static int lpc3143_aes_kthread(void *p)
{
	while (!kthread_should_stop())
	{
		struct crypto_async_request *async_req, *backlog;
		int ret;
		
		if (!wait_event_timeout(aesreqq, aeshw.queue.qlen, HZ))
			continue;

		spin_lock_irq(&aeshw.qlock);
		backlog = crypto_get_backlog(&aeshw.queue);
		async_req = crypto_dequeue_request(&aeshw.queue);
		spin_unlock_irq(&aeshw.qlock);
		
		if (backlog)
			backlog->complete(backlog, -EINPROGRESS);

		if (!async_req)
			continue;

		ret = lpc3143_handle_req((int)p, container_of(async_req,
					struct ablkcipher_request, base));

		async_req->complete(async_req, ret);
	}
	return 0;
}

#endif

static int lpc3143_aes_decrypt(struct ablkcipher_request *req)
{
#ifdef ASYNC
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&aeshw.qlock, flags);
	ret = ablkcipher_enqueue_request(&aeshw.queue, req);
	spin_unlock_irqrestore(&aeshw.qlock, flags);

	wake_up(&aesreqq);

	return ret;
#else
	return lpc3143_handle_req(0, req);
#endif
}

/* Enable or disable NAND controller clocks */
static void lpc313x_nand_clocks_disen(int en)
{
	/* Enable or disable clocks for NAND Controller */
	cgu_clk_en_dis(CGU_SB_NANDFLASH_S0_CLK_ID, en);
	cgu_clk_en_dis(CGU_SB_NANDFLASH_NAND_CLK_ID, en);
	cgu_clk_en_dis(CGU_SB_NANDFLASH_PCLK_ID, en);

	/* Needed for LPC3143/54 chips only */
	cgu_clk_en_dis(CGU_SB_NANDFLASH_AES_CLK_ID, en);
}

static int lpc3143_aes_encrypt(struct ablkcipher_request *req)
{
	unsigned int *iv = req->info;
	unsigned int block[] = { iv[0], iv[1], iv[2], iv[3] };
	int i;

	if (req->nbytes != CHUNK)
	{
		printk(KERN_CRIT "lpc3143_aes: data size != chunk size!!\n");
		return -EFAULT;
	}

	for (i = 0; i < CHUNK; i += 16)
	{
		unsigned int *pbuf = (unsigned int*)((unsigned char*)sg_virt(req->src) + i);

		block[0] ^= pbuf[0];
		block[1] ^= pbuf[1];
		block[2] ^= pbuf[2];
		block[3] ^= pbuf[3];
	
		encrypt_block(block, 
			((struct lpc3143_aes_instance *)crypto_tfm_ctx(req->base.tfm))->round_key);

		memcpy((unsigned char*)sg_virt(req->dst) + i, block, sizeof(block));
	}

	return 0;
}

int lpc3143_aes_decrypt_elf(unsigned char *base, unsigned int phys, int size, unsigned char *key)
{
	unsigned int *k = (unsigned int*)key;
	unsigned int *iv = (unsigned int*)&base[0x80];
	int i;
	
	mutex_lock(&aeshw_mtx);

	writel(k[0], &aeshw.regs[KEY+0]);
	writel(k[1], &aeshw.regs[KEY+1]);
	writel(k[2], &aeshw.regs[KEY+2]);
	writel(k[3], &aeshw.regs[KEY+3]);

	writel(iv[0], &aeshw.regs[IV+0]);
	writel(iv[1], &aeshw.regs[IV+1]);
	writel(iv[2], &aeshw.regs[IV+2]);
	writel(iv[3], &aeshw.regs[IV+3]);

	for (i = 1; i < size/CHUNK; i++)
	{
		long l;

		lpc3143_dmacpy(aeshw.phys_sram[0], phys + CHUNK*i, CHUNK);
		
		writel(NAND_AES_AHB_EN | NAND_AES_AHB_DCRYPT_RAM0,
			&aeshw.regs[AES_FROM_AHB]);
	
		for (l = 50; l && !try_wait_for_completion(&aescomplete); --l);
		INIT_COMPLETION(aescomplete);	
			
		if (!l)
		{
			// timed out
			printk(KERN_CRIT "lpc3143_aes: IRQ never arrived!\n");
			mutex_unlock(&aeshw_mtx);
			return -EFAULT;
		}
		
		lpc3143_dmacpy(phys + CHUNK*i, aeshw.phys_sram[0], CHUNK);
	}

	mutex_unlock(&aeshw_mtx);

	return 0;
}

static struct crypto_alg lpc3143_aes_alg_cbc = {
	.cra_name		= "cbc(nxpaes)",
	.cra_driver_name	= "lpc3143_aes",
	.cra_priority	= 300,
	.cra_flags	= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
	.cra_blocksize	= AES_BLOCK_SIZE,
	.cra_alignmask	= 0,
	.cra_type	= &crypto_ablkcipher_type,
	.cra_module	= THIS_MODULE,
	.cra_ctxsize	= sizeof(struct lpc3143_aes_instance),
	.cra_u		= {
		.ablkcipher = {
			.ivsize		= AES_BLOCK_SIZE,
			.min_keysize	= AES_KEYSIZE_128,
			.max_keysize	= AES_KEYSIZE_128,
			.setkey		= lpc3143_aes_setkey,
			.encrypt	= lpc3143_aes_encrypt,
			.decrypt	= lpc3143_aes_decrypt,
		},
	},
};

static int lpc3143_aes_probe(struct platform_device *pdev)
{
	struct lpc3143_aes_priv *p = &aeshw;
	struct resource *res;
	int irq;
	int ret;

	static int probed;
	
	if (probed)
		return -EEXIST;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res)
		return -ENXIO;
	
	p->phys_sram[0] = res->start;
	p->phys_sram[1] = res->start + 0x400;
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENXIO;

	p->regs = ioremap(res->start, res->end - res->start + 1);
	if (!p->regs)
		return -ENOMEM;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0 || irq == NO_IRQ) {
		ret = irq;
		goto err_unmap_regs;
	}
	p->irq = irq;

	platform_set_drvdata(pdev, p);

	ret = request_irq(irq, lpc3143_aes_irq, IRQF_DISABLED,
		dev_name(&pdev->dev), p);

	if (ret)
		goto err_unmap_regs;

#ifdef ASYNC
	p->workers[0] = kthread_run(lpc3143_aes_kthread, (void*)0, "kaeshw-0");
	if (IS_ERR(p->workers[0])) {
		ret = PTR_ERR(p->workers[0]);
		goto err_thread;
	}

	p->workers[1] = kthread_run(lpc3143_aes_kthread, (void*)1, "kaeshw-1");
	if (IS_ERR(p->workers[0])) {
		ret = PTR_ERR(p->workers[0]);
		goto err_thread2;
	}
	printk(KERN_CRIT "lpc3143_aes: asynchronous mode, using 2 worker threads\n");
#endif

	p->dma_ch = dma_request_channel("lpc3143_aes", lpc3143_aes_dma_irq, 0);
	if (p->dma_ch < 0)
	{
		dev_err(&pdev->dev, "error getting DMA channel.\n");
		ret = -ENXIO;
		goto err_thread2;
	}
	printk(KERN_CRIT "lpc3143_aes: using DMA channel %d\n", p->dma_ch);

	crypto_register_alg(&lpc3143_aes_alg_cbc);

	/* Enable clocks for NAND Controller */
	lpc313x_nand_clocks_disen(1);

	/* Reset NAND controller */
	cgu_soft_reset_module(NANDFLASH_CTRL_NAND_RESET_N_SOFT);
	cgu_soft_reset_module(NANDFLASH_CTRL_AES_RESET_N_SOFT);

	// enable access over AHB
	writel(NAND_AES_AHB_EN, &aeshw.regs[AES_FROM_AHB]);

	// unmask only the IRQs we need
	writel(~(NAND_NANDIRQSTATUS1_AES_DONE_RAM0|NAND_NANDIRQSTATUS1_AES_DONE_RAM1),
		&p->regs[NandIRQMask1]);

#ifdef ASYNC
	spin_lock_init(&p->qlock);
	crypto_init_queue(&p->queue, 50);
#endif

	printk(KERN_WARNING "lpc3143_aes: set up and ready to go\n");
	
	probed = 1;
	
	return 0;

err_thread2:
#ifdef ASYNC
	kthread_stop(aeshw.workers[1]);
	kthread_stop(aeshw.workers[0]);
err_thread:
#endif
	free_irq(irq, p);
err_unmap_regs:
	iounmap(p->regs);

	return ret;
}

static int lpc3143_aes_remove(struct platform_device *pdev)
{
	struct lpc3143_aes_priv *p = platform_get_drvdata(pdev);

	writel(0, p->regs[AES_FROM_AHB]);
	
	crypto_unregister_alg(&lpc3143_aes_alg_cbc);

#ifdef ASYNC
	kthread_stop(aeshw.workers[1]);
	kthread_stop(aeshw.workers[0]);
#endif
	free_irq(p->irq, p);
	iounmap(p->regs);
	kfree(p);

        /* Disable clocks for NAND Controller */
        lpc313x_nand_clocks_disen(0);

	return 0;
}

static struct platform_driver lpc3143_aes = {
	.probe		= lpc3143_aes_probe,
	.remove		= lpc3143_aes_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "lpc3143_aes",
	},
};

static int __init lpc3143_aes_init(void)
{
	return platform_driver_register(&lpc3143_aes);
}
module_init(lpc3143_aes_init);

static void __exit lpc3143_aes_exit(void)
{
	platform_driver_unregister(&lpc3143_aes);
}
module_exit(lpc3143_aes_exit);

MODULE_ALIAS("platform:lpc3143_aes");
MODULE_LICENSE("GPL");

#define BYTE0(a)	((unsigned char*)&a)[0]
#define BYTE1(a)	((unsigned char*)&a)[1]
#define BYTE2(a)	((unsigned char*)&a)[2]
#define BYTE3(a)	((unsigned char*)&a)[3]

static unsigned int const rcon[] = {
	0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000,
	0x20000000, 0x40000000, 0x80000000, 0x1b000000, 0x36000000,
};

static unsigned int const sbox[] = {
	0x63636363,0x7C7C7C7C,0x77777777,0x7B7B7B7B,0xF2F2F2F2,0x6B6B6B6B,0x6F6F6F6F,0xC5C5C5C5,
	0x30303030,0x01010101,0x67676767,0x2B2B2B2B,0xFEFEFEFE,0xD7D7D7D7,0xABABABAB,0x76767676,
	0xCACACACA,0x82828282,0xC9C9C9C9,0x7D7D7D7D,0xFAFAFAFA,0x59595959,0x47474747,0xF0F0F0F0,
	0xADADADAD,0xD4D4D4D4,0xA2A2A2A2,0xAFAFAFAF,0x9C9C9C9C,0xA4A4A4A4,0x72727272,0xC0C0C0C0,
	0xB7B7B7B7,0xFDFDFDFD,0x93939393,0x26262626,0x36363636,0x3F3F3F3F,0xF7F7F7F7,0xCCCCCCCC,
	0x34343434,0xA5A5A5A5,0xE5E5E5E5,0xF1F1F1F1,0x71717171,0xD8D8D8D8,0x31313131,0x15151515,
	0x04040404,0xC7C7C7C7,0x23232323,0xC3C3C3C3,0x18181818,0x96969696,0x05050505,0x9A9A9A9A,
	0x07070707,0x12121212,0x80808080,0xE2E2E2E2,0xEBEBEBEB,0x27272727,0xB2B2B2B2,0x75757575,
	0x09090909,0x83838383,0x2C2C2C2C,0x1A1A1A1A,0x1B1B1B1B,0x6E6E6E6E,0x5A5A5A5A,0xA0A0A0A0,
	0x52525252,0x3B3B3B3B,0xD6D6D6D6,0xB3B3B3B3,0x29292929,0xE3E3E3E3,0x2F2F2F2F,0x84848484,
	0x53535353,0xD1D1D1D1,0x00000000,0xEDEDEDED,0x20202020,0xFCFCFCFC,0xB1B1B1B1,0x5B5B5B5B,
	0x6A6A6A6A,0xCBCBCBCB,0xBEBEBEBE,0x39393939,0x4A4A4A4A,0x4C4C4C4C,0x58585858,0xCFCFCFCF,
	0xD0D0D0D0,0xEFEFEFEF,0xAAAAAAAA,0xFBFBFBFB,0x43434343,0x4D4D4D4D,0x33333333,0x85858585,
	0x45454545,0xF9F9F9F9,0x02020202,0x7F7F7F7F,0x50505050,0x3C3C3C3C,0x9F9F9F9F,0xA8A8A8A8,
	0x51515151,0xA3A3A3A3,0x40404040,0x8F8F8F8F,0x92929292,0x9D9D9D9D,0x38383838,0xF5F5F5F5,
	0xBCBCBCBC,0xB6B6B6B6,0xDADADADA,0x21212121,0x10101010,0xFFFFFFFF,0xF3F3F3F3,0xD2D2D2D2,
	0xCDCDCDCD,0x0C0C0C0C,0x13131313,0xECECECEC,0x5F5F5F5F,0x97979797,0x44444444,0x17171717,
	0xC4C4C4C4,0xA7A7A7A7,0x7E7E7E7E,0x3D3D3D3D,0x64646464,0x5D5D5D5D,0x19191919,0x73737373,
	0x60606060,0x81818181,0x4F4F4F4F,0xDCDCDCDC,0x22222222,0x2A2A2A2A,0x90909090,0x88888888,
	0x46464646,0xEEEEEEEE,0xB8B8B8B8,0x14141414,0xDEDEDEDE,0x5E5E5E5E,0x0B0B0B0B,0xDBDBDBDB,
	0xE0E0E0E0,0x32323232,0x3A3A3A3A,0x0A0A0A0A,0x49494949,0x06060606,0x24242424,0x5C5C5C5C,
	0xC2C2C2C2,0xD3D3D3D3,0xACACACAC,0x62626262,0x91919191,0x95959595,0xE4E4E4E4,0x79797979,
	0xE7E7E7E7,0xC8C8C8C8,0x37373737,0x6D6D6D6D,0x8D8D8D8D,0xD5D5D5D5,0x4E4E4E4E,0xA9A9A9A9,
	0x6C6C6C6C,0x56565656,0xF4F4F4F4,0xEAEAEAEA,0x65656565,0x7A7A7A7A,0xAEAEAEAE,0x08080808,
	0xBABABABA,0x78787878,0x25252525,0x2E2E2E2E,0x1C1C1C1C,0xA6A6A6A6,0xB4B4B4B4,0xC6C6C6C6,
	0xE8E8E8E8,0xDDDDDDDD,0x74747474,0x1F1F1F1F,0x4B4B4B4B,0xBDBDBDBD,0x8B8B8B8B,0x8A8A8A8A,
	0x70707070,0x3E3E3E3E,0xB5B5B5B5,0x66666666,0x48484848,0x03030303,0xF6F6F6F6,0x0E0E0E0E,
	0x61616161,0x35353535,0x57575757,0xB9B9B9B9,0x86868686,0xC1C1C1C1,0x1D1D1D1D,0x9E9E9E9E,
	0xE1E1E1E1,0xF8F8F8F8,0x98989898,0x11111111,0x69696969,0xD9D9D9D9,0x8E8E8E8E,0x94949494,
	0x9B9B9B9B,0x1E1E1E1E,0x87878787,0xE9E9E9E9,0xCECECECE,0x55555555,0x28282828,0xDFDFDFDF,
	0x8C8C8C8C,0xA1A1A1A1,0x89898989,0x0D0D0D0D,0xBFBFBFBF,0xE6E6E6E6,0x42424242,0x68686868,
	0x41414141,0x99999999,0x2D2D2D2D,0x0F0F0F0F,0xB0B0B0B0,0x54545454,0xBBBBBBBB,0x16161616,
};

static unsigned int const table[][256] = {{
	0xC66363A5,0xF87C7C84,0xEE777799,0xF67B7B8D,0xFFF2F20D,0xD66B6BBD,0xDE6F6FB1,0x91C5C554,
	0x60303050,0x02010103,0xCE6767A9,0x562B2B7D,0xE7FEFE19,0xB5D7D762,0x4DABABE6,0xEC76769A,
	0x8FCACA45,0x1F82829D,0x89C9C940,0xFA7D7D87,0xEFFAFA15,0xB25959EB,0x8E4747C9,0xFBF0F00B,
	0x41ADADEC,0xB3D4D467,0x5FA2A2FD,0x45AFAFEA,0x239C9CBF,0x53A4A4F7,0xE4727296,0x9BC0C05B,
	0x75B7B7C2,0xE1FDFD1C,0x3D9393AE,0x4C26266A,0x6C36365A,0x7E3F3F41,0xF5F7F702,0x83CCCC4F,
	0x6834345C,0x51A5A5F4,0xD1E5E534,0xF9F1F108,0xE2717193,0xABD8D873,0x62313153,0x2A15153F,
	0x0804040C,0x95C7C752,0x46232365,0x9DC3C35E,0x30181828,0x379696A1,0x0A05050F,0x2F9A9AB5,
	0x0E070709,0x24121236,0x1B80809B,0xDFE2E23D,0xCDEBEB26,0x4E272769,0x7FB2B2CD,0xEA75759F,
	0x1209091B,0x1D83839E,0x582C2C74,0x341A1A2E,0x361B1B2D,0xDC6E6EB2,0xB45A5AEE,0x5BA0A0FB,
	0xA45252F6,0x763B3B4D,0xB7D6D661,0x7DB3B3CE,0x5229297B,0xDDE3E33E,0x5E2F2F71,0x13848497,
	0xA65353F5,0xB9D1D168,0x00000000,0xC1EDED2C,0x40202060,0xE3FCFC1F,0x79B1B1C8,0xB65B5BED,
	0xD46A6ABE,0x8DCBCB46,0x67BEBED9,0x7239394B,0x944A4ADE,0x984C4CD4,0xB05858E8,0x85CFCF4A,
	0xBBD0D06B,0xC5EFEF2A,0x4FAAAAE5,0xEDFBFB16,0x864343C5,0x9A4D4DD7,0x66333355,0x11858594,
	0x8A4545CF,0xE9F9F910,0x04020206,0xFE7F7F81,0xA05050F0,0x783C3C44,0x259F9FBA,0x4BA8A8E3,
	0xA25151F3,0x5DA3A3FE,0x804040C0,0x058F8F8A,0x3F9292AD,0x219D9DBC,0x70383848,0xF1F5F504,
	0x63BCBCDF,0x77B6B6C1,0xAFDADA75,0x42212163,0x20101030,0xE5FFFF1A,0xFDF3F30E,0xBFD2D26D,
	0x81CDCD4C,0x180C0C14,0x26131335,0xC3ECEC2F,0xBE5F5FE1,0x359797A2,0x884444CC,0x2E171739,
	0x93C4C457,0x55A7A7F2,0xFC7E7E82,0x7A3D3D47,0xC86464AC,0xBA5D5DE7,0x3219192B,0xE6737395,
	0xC06060A0,0x19818198,0x9E4F4FD1,0xA3DCDC7F,0x44222266,0x542A2A7E,0x3B9090AB,0x0B888883,
	0x8C4646CA,0xC7EEEE29,0x6BB8B8D3,0x2814143C,0xA7DEDE79,0xBC5E5EE2,0x160B0B1D,0xADDBDB76,
	0xDBE0E03B,0x64323256,0x743A3A4E,0x140A0A1E,0x924949DB,0x0C06060A,0x4824246C,0xB85C5CE4,
	0x9FC2C25D,0xBDD3D36E,0x43ACACEF,0xC46262A6,0x399191A8,0x319595A4,0xD3E4E437,0xF279798B,
	0xD5E7E732,0x8BC8C843,0x6E373759,0xDA6D6DB7,0x018D8D8C,0xB1D5D564,0x9C4E4ED2,0x49A9A9E0,
	0xD86C6CB4,0xAC5656FA,0xF3F4F407,0xCFEAEA25,0xCA6565AF,0xF47A7A8E,0x47AEAEE9,0x10080818,
	0x6FBABAD5,0xF0787888,0x4A25256F,0x5C2E2E72,0x381C1C24,0x57A6A6F1,0x73B4B4C7,0x97C6C651,
	0xCBE8E823,0xA1DDDD7C,0xE874749C,0x3E1F1F21,0x964B4BDD,0x61BDBDDC,0x0D8B8B86,0x0F8A8A85,
	0xE0707090,0x7C3E3E42,0x71B5B5C4,0xCC6666AA,0x904848D8,0x06030305,0xF7F6F601,0x1C0E0E12,
	0xC26161A3,0x6A35355F,0xAE5757F9,0x69B9B9D0,0x17868691,0x99C1C158,0x3A1D1D27,0x279E9EB9,
	0xD9E1E138,0xEBF8F813,0x2B9898B3,0x22111133,0xD26969BB,0xA9D9D970,0x078E8E89,0x339494A7,
	0x2D9B9BB6,0x3C1E1E22,0x15878792,0xC9E9E920,0x87CECE49,0xAA5555FF,0x50282878,0xA5DFDF7A,
	0x038C8C8F,0x59A1A1F8,0x09898980,0x1A0D0D17,0x65BFBFDA,0xD7E6E631,0x844242C6,0xD06868B8,
	0x824141C3,0x299999B0,0x5A2D2D77,0x1E0F0F11,0x7BB0B0CB,0xA85454FC,0x6DBBBBD6,0x2C16163A,
}, {
	0xA5C66363,0x84F87C7C,0x99EE7777,0x8DF67B7B,0x0DFFF2F2,0xBDD66B6B,0xB1DE6F6F,0x5491C5C5,
	0x50603030,0x03020101,0xA9CE6767,0x7D562B2B,0x19E7FEFE,0x62B5D7D7,0xE64DABAB,0x9AEC7676,
	0x458FCACA,0x9D1F8282,0x4089C9C9,0x87FA7D7D,0x15EFFAFA,0xEBB25959,0xC98E4747,0x0BFBF0F0,
	0xEC41ADAD,0x67B3D4D4,0xFD5FA2A2,0xEA45AFAF,0xBF239C9C,0xF753A4A4,0x96E47272,0x5B9BC0C0,
	0xC275B7B7,0x1CE1FDFD,0xAE3D9393,0x6A4C2626,0x5A6C3636,0x417E3F3F,0x02F5F7F7,0x4F83CCCC,
	0x5C683434,0xF451A5A5,0x34D1E5E5,0x08F9F1F1,0x93E27171,0x73ABD8D8,0x53623131,0x3F2A1515,
	0x0C080404,0x5295C7C7,0x65462323,0x5E9DC3C3,0x28301818,0xA1379696,0x0F0A0505,0xB52F9A9A,
	0x090E0707,0x36241212,0x9B1B8080,0x3DDFE2E2,0x26CDEBEB,0x694E2727,0xCD7FB2B2,0x9FEA7575,
	0x1B120909,0x9E1D8383,0x74582C2C,0x2E341A1A,0x2D361B1B,0xB2DC6E6E,0xEEB45A5A,0xFB5BA0A0,
	0xF6A45252,0x4D763B3B,0x61B7D6D6,0xCE7DB3B3,0x7B522929,0x3EDDE3E3,0x715E2F2F,0x97138484,
	0xF5A65353,0x68B9D1D1,0x00000000,0x2CC1EDED,0x60402020,0x1FE3FCFC,0xC879B1B1,0xEDB65B5B,
	0xBED46A6A,0x468DCBCB,0xD967BEBE,0x4B723939,0xDE944A4A,0xD4984C4C,0xE8B05858,0x4A85CFCF,
	0x6BBBD0D0,0x2AC5EFEF,0xE54FAAAA,0x16EDFBFB,0xC5864343,0xD79A4D4D,0x55663333,0x94118585,
	0xCF8A4545,0x10E9F9F9,0x06040202,0x81FE7F7F,0xF0A05050,0x44783C3C,0xBA259F9F,0xE34BA8A8,
	0xF3A25151,0xFE5DA3A3,0xC0804040,0x8A058F8F,0xAD3F9292,0xBC219D9D,0x48703838,0x04F1F5F5,
	0xDF63BCBC,0xC177B6B6,0x75AFDADA,0x63422121,0x30201010,0x1AE5FFFF,0x0EFDF3F3,0x6DBFD2D2,
	0x4C81CDCD,0x14180C0C,0x35261313,0x2FC3ECEC,0xE1BE5F5F,0xA2359797,0xCC884444,0x392E1717,
	0x5793C4C4,0xF255A7A7,0x82FC7E7E,0x477A3D3D,0xACC86464,0xE7BA5D5D,0x2B321919,0x95E67373,
	0xA0C06060,0x98198181,0xD19E4F4F,0x7FA3DCDC,0x66442222,0x7E542A2A,0xAB3B9090,0x830B8888,
	0xCA8C4646,0x29C7EEEE,0xD36BB8B8,0x3C281414,0x79A7DEDE,0xE2BC5E5E,0x1D160B0B,0x76ADDBDB,
	0x3BDBE0E0,0x56643232,0x4E743A3A,0x1E140A0A,0xDB924949,0x0A0C0606,0x6C482424,0xE4B85C5C,
	0x5D9FC2C2,0x6EBDD3D3,0xEF43ACAC,0xA6C46262,0xA8399191,0xA4319595,0x37D3E4E4,0x8BF27979,
	0x32D5E7E7,0x438BC8C8,0x596E3737,0xB7DA6D6D,0x8C018D8D,0x64B1D5D5,0xD29C4E4E,0xE049A9A9,
	0xB4D86C6C,0xFAAC5656,0x07F3F4F4,0x25CFEAEA,0xAFCA6565,0x8EF47A7A,0xE947AEAE,0x18100808,
	0xD56FBABA,0x88F07878,0x6F4A2525,0x725C2E2E,0x24381C1C,0xF157A6A6,0xC773B4B4,0x5197C6C6,
	0x23CBE8E8,0x7CA1DDDD,0x9CE87474,0x213E1F1F,0xDD964B4B,0xDC61BDBD,0x860D8B8B,0x850F8A8A,
	0x90E07070,0x427C3E3E,0xC471B5B5,0xAACC6666,0xD8904848,0x05060303,0x01F7F6F6,0x121C0E0E,
	0xA3C26161,0x5F6A3535,0xF9AE5757,0xD069B9B9,0x91178686,0x5899C1C1,0x273A1D1D,0xB9279E9E,
	0x38D9E1E1,0x13EBF8F8,0xB32B9898,0x33221111,0xBBD26969,0x70A9D9D9,0x89078E8E,0xA7339494,
	0xB62D9B9B,0x223C1E1E,0x92158787,0x20C9E9E9,0x4987CECE,0xFFAA5555,0x78502828,0x7AA5DFDF,
	0x8F038C8C,0xF859A1A1,0x80098989,0x171A0D0D,0xDA65BFBF,0x31D7E6E6,0xC6844242,0xB8D06868,
	0xC3824141,0xB0299999,0x775A2D2D,0x111E0F0F,0xCB7BB0B0,0xFCA85454,0xD66DBBBB,0x3A2C1616,
}, {
	0x63A5C663,0x7C84F87C,0x7799EE77,0x7B8DF67B,0xF20DFFF2,0x6BBDD66B,0x6FB1DE6F,0xC55491C5,
	0x30506030,0x01030201,0x67A9CE67,0x2B7D562B,0xFE19E7FE,0xD762B5D7,0xABE64DAB,0x769AEC76,
	0xCA458FCA,0x829D1F82,0xC94089C9,0x7D87FA7D,0xFA15EFFA,0x59EBB259,0x47C98E47,0xF00BFBF0,
	0xADEC41AD,0xD467B3D4,0xA2FD5FA2,0xAFEA45AF,0x9CBF239C,0xA4F753A4,0x7296E472,0xC05B9BC0,
	0xB7C275B7,0xFD1CE1FD,0x93AE3D93,0x266A4C26,0x365A6C36,0x3F417E3F,0xF702F5F7,0xCC4F83CC,
	0x345C6834,0xA5F451A5,0xE534D1E5,0xF108F9F1,0x7193E271,0xD873ABD8,0x31536231,0x153F2A15,
	0x040C0804,0xC75295C7,0x23654623,0xC35E9DC3,0x18283018,0x96A13796,0x050F0A05,0x9AB52F9A,
	0x07090E07,0x12362412,0x809B1B80,0xE23DDFE2,0xEB26CDEB,0x27694E27,0xB2CD7FB2,0x759FEA75,
	0x091B1209,0x839E1D83,0x2C74582C,0x1A2E341A,0x1B2D361B,0x6EB2DC6E,0x5AEEB45A,0xA0FB5BA0,
	0x52F6A452,0x3B4D763B,0xD661B7D6,0xB3CE7DB3,0x297B5229,0xE33EDDE3,0x2F715E2F,0x84971384,
	0x53F5A653,0xD168B9D1,0x00000000,0xED2CC1ED,0x20604020,0xFC1FE3FC,0xB1C879B1,0x5BEDB65B,
	0x6ABED46A,0xCB468DCB,0xBED967BE,0x394B7239,0x4ADE944A,0x4CD4984C,0x58E8B058,0xCF4A85CF,
	0xD06BBBD0,0xEF2AC5EF,0xAAE54FAA,0xFB16EDFB,0x43C58643,0x4DD79A4D,0x33556633,0x85941185,
	0x45CF8A45,0xF910E9F9,0x02060402,0x7F81FE7F,0x50F0A050,0x3C44783C,0x9FBA259F,0xA8E34BA8,
	0x51F3A251,0xA3FE5DA3,0x40C08040,0x8F8A058F,0x92AD3F92,0x9DBC219D,0x38487038,0xF504F1F5,
	0xBCDF63BC,0xB6C177B6,0xDA75AFDA,0x21634221,0x10302010,0xFF1AE5FF,0xF30EFDF3,0xD26DBFD2,
	0xCD4C81CD,0x0C14180C,0x13352613,0xEC2FC3EC,0x5FE1BE5F,0x97A23597,0x44CC8844,0x17392E17,
	0xC45793C4,0xA7F255A7,0x7E82FC7E,0x3D477A3D,0x64ACC864,0x5DE7BA5D,0x192B3219,0x7395E673,
	0x60A0C060,0x81981981,0x4FD19E4F,0xDC7FA3DC,0x22664422,0x2A7E542A,0x90AB3B90,0x88830B88,
	0x46CA8C46,0xEE29C7EE,0xB8D36BB8,0x143C2814,0xDE79A7DE,0x5EE2BC5E,0x0B1D160B,0xDB76ADDB,
	0xE03BDBE0,0x32566432,0x3A4E743A,0x0A1E140A,0x49DB9249,0x060A0C06,0x246C4824,0x5CE4B85C,
	0xC25D9FC2,0xD36EBDD3,0xACEF43AC,0x62A6C462,0x91A83991,0x95A43195,0xE437D3E4,0x798BF279,
	0xE732D5E7,0xC8438BC8,0x37596E37,0x6DB7DA6D,0x8D8C018D,0xD564B1D5,0x4ED29C4E,0xA9E049A9,
	0x6CB4D86C,0x56FAAC56,0xF407F3F4,0xEA25CFEA,0x65AFCA65,0x7A8EF47A,0xAEE947AE,0x08181008,
	0xBAD56FBA,0x7888F078,0x256F4A25,0x2E725C2E,0x1C24381C,0xA6F157A6,0xB4C773B4,0xC65197C6,
	0xE823CBE8,0xDD7CA1DD,0x749CE874,0x1F213E1F,0x4BDD964B,0xBDDC61BD,0x8B860D8B,0x8A850F8A,
	0x7090E070,0x3E427C3E,0xB5C471B5,0x66AACC66,0x48D89048,0x03050603,0xF601F7F6,0x0E121C0E,
	0x61A3C261,0x355F6A35,0x57F9AE57,0xB9D069B9,0x86911786,0xC15899C1,0x1D273A1D,0x9EB9279E,
	0xE138D9E1,0xF813EBF8,0x98B32B98,0x11332211,0x69BBD269,0xD970A9D9,0x8E89078E,0x94A73394,
	0x9BB62D9B,0x1E223C1E,0x87921587,0xE920C9E9,0xCE4987CE,0x55FFAA55,0x28785028,0xDF7AA5DF,
	0x8C8F038C,0xA1F859A1,0x89800989,0x0D171A0D,0xBFDA65BF,0xE631D7E6,0x42C68442,0x68B8D068,
	0x41C38241,0x99B02999,0x2D775A2D,0x0F111E0F,0xB0CB7BB0,0x54FCA854,0xBBD66DBB,0x163A2C16,
}, {
	0x6363A5C6,0x7C7C84F8,0x777799EE,0x7B7B8DF6,0xF2F20DFF,0x6B6BBDD6,0x6F6FB1DE,0xC5C55491,
	0x30305060,0x01010302,0x6767A9CE,0x2B2B7D56,0xFEFE19E7,0xD7D762B5,0xABABE64D,0x76769AEC,
	0xCACA458F,0x82829D1F,0xC9C94089,0x7D7D87FA,0xFAFA15EF,0x5959EBB2,0x4747C98E,0xF0F00BFB,
	0xADADEC41,0xD4D467B3,0xA2A2FD5F,0xAFAFEA45,0x9C9CBF23,0xA4A4F753,0x727296E4,0xC0C05B9B,
	0xB7B7C275,0xFDFD1CE1,0x9393AE3D,0x26266A4C,0x36365A6C,0x3F3F417E,0xF7F702F5,0xCCCC4F83,
	0x34345C68,0xA5A5F451,0xE5E534D1,0xF1F108F9,0x717193E2,0xD8D873AB,0x31315362,0x15153F2A,
	0x04040C08,0xC7C75295,0x23236546,0xC3C35E9D,0x18182830,0x9696A137,0x05050F0A,0x9A9AB52F,
	0x0707090E,0x12123624,0x80809B1B,0xE2E23DDF,0xEBEB26CD,0x2727694E,0xB2B2CD7F,0x75759FEA,
	0x09091B12,0x83839E1D,0x2C2C7458,0x1A1A2E34,0x1B1B2D36,0x6E6EB2DC,0x5A5AEEB4,0xA0A0FB5B,
	0x5252F6A4,0x3B3B4D76,0xD6D661B7,0xB3B3CE7D,0x29297B52,0xE3E33EDD,0x2F2F715E,0x84849713,
	0x5353F5A6,0xD1D168B9,0x00000000,0xEDED2CC1,0x20206040,0xFCFC1FE3,0xB1B1C879,0x5B5BEDB6,
	0x6A6ABED4,0xCBCB468D,0xBEBED967,0x39394B72,0x4A4ADE94,0x4C4CD498,0x5858E8B0,0xCFCF4A85,
	0xD0D06BBB,0xEFEF2AC5,0xAAAAE54F,0xFBFB16ED,0x4343C586,0x4D4DD79A,0x33335566,0x85859411,
	0x4545CF8A,0xF9F910E9,0x02020604,0x7F7F81FE,0x5050F0A0,0x3C3C4478,0x9F9FBA25,0xA8A8E34B,
	0x5151F3A2,0xA3A3FE5D,0x4040C080,0x8F8F8A05,0x9292AD3F,0x9D9DBC21,0x38384870,0xF5F504F1,
	0xBCBCDF63,0xB6B6C177,0xDADA75AF,0x21216342,0x10103020,0xFFFF1AE5,0xF3F30EFD,0xD2D26DBF,
	0xCDCD4C81,0x0C0C1418,0x13133526,0xECEC2FC3,0x5F5FE1BE,0x9797A235,0x4444CC88,0x1717392E,
	0xC4C45793,0xA7A7F255,0x7E7E82FC,0x3D3D477A,0x6464ACC8,0x5D5DE7BA,0x19192B32,0x737395E6,
	0x6060A0C0,0x81819819,0x4F4FD19E,0xDCDC7FA3,0x22226644,0x2A2A7E54,0x9090AB3B,0x8888830B,
	0x4646CA8C,0xEEEE29C7,0xB8B8D36B,0x14143C28,0xDEDE79A7,0x5E5EE2BC,0x0B0B1D16,0xDBDB76AD,
	0xE0E03BDB,0x32325664,0x3A3A4E74,0x0A0A1E14,0x4949DB92,0x06060A0C,0x24246C48,0x5C5CE4B8,
	0xC2C25D9F,0xD3D36EBD,0xACACEF43,0x6262A6C4,0x9191A839,0x9595A431,0xE4E437D3,0x79798BF2,
	0xE7E732D5,0xC8C8438B,0x3737596E,0x6D6DB7DA,0x8D8D8C01,0xD5D564B1,0x4E4ED29C,0xA9A9E049,
	0x6C6CB4D8,0x5656FAAC,0xF4F407F3,0xEAEA25CF,0x6565AFCA,0x7A7A8EF4,0xAEAEE947,0x08081810,
	0xBABAD56F,0x787888F0,0x25256F4A,0x2E2E725C,0x1C1C2438,0xA6A6F157,0xB4B4C773,0xC6C65197,
	0xE8E823CB,0xDDDD7CA1,0x74749CE8,0x1F1F213E,0x4B4BDD96,0xBDBDDC61,0x8B8B860D,0x8A8A850F,
	0x707090E0,0x3E3E427C,0xB5B5C471,0x6666AACC,0x4848D890,0x03030506,0xF6F601F7,0x0E0E121C,
	0x6161A3C2,0x35355F6A,0x5757F9AE,0xB9B9D069,0x86869117,0xC1C15899,0x1D1D273A,0x9E9EB927,
	0xE1E138D9,0xF8F813EB,0x9898B32B,0x11113322,0x6969BBD2,0xD9D970A9,0x8E8E8907,0x9494A733,
	0x9B9BB62D,0x1E1E223C,0x87879215,0xE9E920C9,0xCECE4987,0x5555FFAA,0x28287850,0xDFDF7AA5,
	0x8C8C8F03,0xA1A1F859,0x89898009,0x0D0D171A,0xBFBFDA65,0xE6E631D7,0x4242C684,0x6868B8D0,
	0x4141C382,0x9999B029,0x2D2D775A,0x0F0F111E,0xB0B0CB7B,0x5454FCA8,0xBBBBD66D,0x16163A2C,
}};

static void expand_key(unsigned char const *key, unsigned int round_key[])
{
	unsigned int *k = (unsigned int*)key, *rk;
	int i;
	
	round_key[0] = k[3];
	round_key[1] = k[2];
	round_key[2] = k[1];
	round_key[3] = k[0];
	
	for (rk = round_key, i = 0; i < sizeof(rcon)/4; i += 1, rk += 4)
	{
		rk[4] = rcon[i] ^ rk[0]
			^ (sbox[BYTE3(rk[3])] & 0xFF)
			^ (sbox[BYTE2(rk[3])] & 0xFF000000)
			^ (sbox[BYTE1(rk[3])] & 0xFF0000)
			^ (sbox[BYTE0(rk[3])] & 0xFF00);

		rk[5] = rk[4] ^ rk[1];
		rk[6] = rk[5] ^ rk[2];
		rk[7] = rk[6] ^ rk[3];
	}
}

static void encrypt_block(unsigned int block[], unsigned int const rk[])
{
	unsigned int x[8];
	int i;
	
	x[0] = rk[0] ^ block[3];
	x[1] = rk[1] ^ block[2];
	x[2] = rk[2] ^ block[1];
	x[3] = rk[3] ^ block[0];

	x[4] = rk[4]
		^ table[0][BYTE3(x[0])]
		^ table[1][BYTE2(x[1])]
		^ table[2][BYTE1(x[2])]
		^ table[3][BYTE0(x[3])];

	x[5] = rk[5]
		^ table[0][BYTE3(x[1])]
		^ table[1][BYTE2(x[2])]
		^ table[2][BYTE1(x[3])]
		^ table[3][BYTE0(x[0])];
		
	x[6] = rk[6]
		^ table[0][BYTE3(x[2])]
		^ table[1][BYTE2(x[3])]
		^ table[2][BYTE1(x[0])]
		^ table[3][BYTE0(x[1])];

	x[7] = rk[7]
		^ table[0][BYTE3(x[3])]
		^ table[1][BYTE2(x[0])]
		^ table[2][BYTE1(x[1])]
		^ table[3][BYTE0(x[2])];

	rk += 8;
	
	for (i = 0; i < 4; i++)
	{
		x[0] = rk[0]
			^ table[0][BYTE3(x[4])]
			^ table[1][BYTE2(x[5])]
			^ table[2][BYTE1(x[6])]
			^ table[3][BYTE0(x[7])];
			
		x[1] = rk[1]
			^ table[0][BYTE3(x[5])]
			^ table[1][BYTE2(x[6])]
			^ table[2][BYTE1(x[7])]
			^ table[3][BYTE0(x[4])];
			
		x[2] = rk[2]
			^ table[0][BYTE3(x[6])]
			^ table[1][BYTE2(x[7])]
			^ table[2][BYTE1(x[4])]
			^ table[3][BYTE0(x[5])];
			
		x[3] = rk[3]
			^ table[0][BYTE3(x[7])]
			^ table[1][BYTE2(x[4])]
			^ table[2][BYTE1(x[5])]
			^ table[3][BYTE0(x[6])];
			
		x[4] = rk[4]
			^ table[0][BYTE3(x[0])]
			^ table[1][BYTE2(x[1])]
			^ table[2][BYTE1(x[2])]
			^ table[3][BYTE0(x[3])];

		x[5] = rk[5]
			^ table[0][BYTE3(x[1])]
			^ table[1][BYTE2(x[2])]
			^ table[2][BYTE1(x[3])]
			^ table[3][BYTE0(x[0])];
			
		x[6] = rk[6]
			^ table[0][BYTE3(x[2])]
			^ table[1][BYTE2(x[3])]
			^ table[2][BYTE1(x[0])]
			^ table[3][BYTE0(x[1])];

		x[7] = rk[7]
			^ table[0][BYTE3(x[3])]
			^ table[1][BYTE2(x[0])]
			^ table[2][BYTE1(x[1])]
			^ table[3][BYTE0(x[2])];

		rk += 8;
	}

	block[3] = rk[0] 
		^ (sbox[BYTE3(x[4])] & 0xFF000000) 
		^ (sbox[BYTE2(x[5])] & 0xFF0000)
		^ (sbox[BYTE1(x[6])] & 0xFF00) 
		^ (sbox[BYTE0(x[7])] & 0xFF);
	
	block[2] = rk[1]
		^ (sbox[BYTE3(x[5])] & 0xFF000000)
		^ (sbox[BYTE2(x[6])] & 0xFF0000)
		^ (sbox[BYTE1(x[7])] & 0xFF00)
		^ (sbox[BYTE0(x[4])] & 0xFF);
	
	block[1] = rk[2]
		^ (sbox[BYTE3(x[6])] & 0xFF000000)
		^ (sbox[BYTE2(x[7])] & 0xFF0000)
		^ (sbox[BYTE1(x[4])] & 0xFF00)
		^ (sbox[BYTE0(x[5])] & 0xFF);
	
	block[0] = rk[3]
		^ (sbox[BYTE3(x[7])] & 0xFF000000)
		^ (sbox[BYTE2(x[4])] & 0xFF0000)
		^ (sbox[BYTE1(x[5])] & 0xFF00)
		^ (sbox[BYTE0(x[6])] & 0xFF);
}
