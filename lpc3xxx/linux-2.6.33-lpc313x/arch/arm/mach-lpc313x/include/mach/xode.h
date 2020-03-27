
#ifndef __PLAT_XODE_H__
#define __PLAT_XODE_H__

#include <mach/hardware.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct xode_info {
	const char * name;
	struct xode_device	*xode_dev;

};

#define GPIO_EJECT		GPIO_MI2STX_CLK0
#define GPIO_TRAY_STATE	GPIO_MI2STX_WS0

#endif

