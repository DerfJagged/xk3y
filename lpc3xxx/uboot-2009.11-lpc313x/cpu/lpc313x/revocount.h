
/*
 * This number is the maximal revocation count burned into the hardware
 * that we tolerate for this build of u-boot, i.e., if the hardware 
 * revocation count is higher, this u-boot is too old and will not run
 * 
 * Note that, due to the nature of OTP, the actual count burned into the
 * ROM will be (1 << MAX_REVO_COUNT)-1, i.e., 0x1, 0x3, 0x7, 0xf
 */

#define MAX_REVO_COUNT		3
#define REVO_BITS		27
