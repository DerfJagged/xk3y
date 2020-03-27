
extern struct otp
{
	/* the control registers */
	volatile unsigned int con;
	volatile unsigned int rprot;
	volatile unsigned int wprot;
	
	/* bits 0 - 127: the unique id supposedly pre-programmed by NXP */
	unsigned int _reserved0[4];
	
	/* bits 128 - 255: the key we use to encrypt u-boot */
	unsigned int bootkey[4];
	
	/* bits 256 - 282: minimal u-boot version we require */
	unsigned revocount:27;

	unsigned _reserved1:5;
	
	/* bits 288 - 289: development or production class */
	unsigned devclass:2;
	
	/* bits 290 - 294: which flavor of hardware are we running on? */
	unsigned hwversion:5;

	/* bit 295: the peoples paradise flag */
	unsigned chineselanguageonly:1;
	
	unsigned _spare1:19;
	unsigned _reserved2:5;

	/* bits 320 - 447: anti-clone key */
	unsigned int acdata[4];
	
	/* bits 448 - 479: serial */
	unsigned int serial[1];
	
	/* bits 480 - 485: chip ID */
	unsigned chip_id:6;
	
	unsigned _reserved3:16;
	
	/* bit 502: whether to fallback to DFU boot if no valid image found */
	unsigned dfu_boot:1;
	
	/* bit 503: whether programmed PID & VID are valid */
	unsigned pid_vid_valid:1;
	
	/* bit 504: whether AES key has been programmed */
	unsigned aes_key_valid:1;
	
	unsigned _reserved4:4;
	
	/* bits 509 - 511: JTAG security level */
	unsigned jtag_sec:3;
} OTP;

#define OTP_DEVCLASS_DEVELOPMENT	0x1
#define OTP_DEVCLASS_PRODUCTION		0x2

#define OTP_CHINESE_LANG_ONLY		0x01

void otp_init(void);
int otp_update_revocount(void);
int otp_program(struct otp const *otp);
