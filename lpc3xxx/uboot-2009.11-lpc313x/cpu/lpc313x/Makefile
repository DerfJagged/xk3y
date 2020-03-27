#
# (C) Copyright 2000-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(CPU).a

START	= start.o
COBJS	= cpu.o
COBJS	+= timer.o
COBJS	+= serial.o
COBJS	+= clock.o
COBJS	+= init.o
COBJS	+= i2c.o
COBJS	+= mci.o
COBJS	+= dos_partition.o
COBJS	+= spi.o
COBJS	+= nand_params.o
COBJS	+= nand_format.o
COBJS	+= crc32.o
COBJS   += usbpoll.o
COBJS   += anticlone.o otp.o factory.o


SRCS	:= $(START:.o=.S) $(SOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(SOBJS) $(COBJS))
START	:= $(addprefix $(obj),$(START))

all:	$(obj).depend $(START) $(LIB)

$(LIB):	$(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

anticlone.o: ac_keys.h
factory.o: ac_keys.h boot_key.h
init.o: emu_key.h kernel_key.h
mci.o: kernel_iv.h

ac_keys.h: $(SRCTREE)/../keys/ackey.key $(SRCTREE)/../keys/acdata.key
	echo "static unsigned int const ac_aeskey[] = {" > $@
	hexdump -e '4/4 "0x%02x," "};\n"' < $(SRCTREE)/../keys/ackey.key >> $@
	echo "static unsigned int const ac_plaintext[] = {" >> $@
	hexdump -e '4/4 "0x%x," "};\n"' < $(SRCTREE)/../keys/acdata.key >> $@
	
boot_key.h: $(SRCTREE)/../keys/uboot.key
	hexdump -e '4/4 "0x%x," "\n"' < $^  > $@

kernel_key.h: $(SRCTREE)/../keys/kernel.key
	echo "static unsigned int kernelkey[] = {" > $@
	hexdump -e '4/4 "0x%x," "};\n"' < $^  >> $@

kernel_iv.h: $(SRCTREE)/../keys/kernel.iv
	hexdump -e '4/4 "0x%x," "\n"' < $^  > $@

emu_key.h: $(SRCTREE)/../keys/emu.key
	echo "static char const emukey[] = {" > $@
	hexdump -e '16/1 "0x%02x," "};\n"' < $^  >> $@

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
