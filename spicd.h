/* 
 * Sony Programmable I/O Control Device (SPIC) driver for VAIO. 
 * Userspace Control Utility
 *
 * Copyright 2001 Stelian Pop, Alcove
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _SPICCTRL_H_ 
#define _SPICCTRL_H

/* Have our own definition of ioctls... */

/* get/set brightness */
#define SONYPI_IOCGBRT		_IOR('v', 0, __u8)
#define SONYPI_IOCSBRT		_IOW('v', 0, __u8)

/* get battery full capacity/remaining capacity */
#define SONYPI_IOCGBAT1CAP	_IOR('v', 2, __u16)
#define SONYPI_IOCGBAT1REM	_IOR('v', 3, __u16)
#define SONYPI_IOCGBAT2CAP	_IOR('v', 4, __u16)
#define SONYPI_IOCGBAT2REM	_IOR('v', 5, __u16)

/* get battery flags: battery1/battery2/ac adapter present */
#define SONYPI_BFLAGS_B1	0x01
#define SONYPI_BFLAGS_B2	0x02
#define SONYPI_BFLAGS_AC	0x04
#define SONYPI_IOCGBATFLAGS	_IOR('v', 7, __u8)

/* get/set bluetooth subsystem state on/off */
#define SONYPI_IOCGBLUE         _IOR('v', 8, __u8)
#define SONYPI_IOCSBLUE         _IOW('v', 9, __u8)

#define SONYPI_DEVICE		"/dev/sonypi"
#define CPUFREQ_DEVICE		"/proc/sys/cpu/0/speed"

#define CPUFREQ_MIN_DEVICE	"/proc/sys/cpu/0/speed-min"
#define CPUFREQ_MAX_DEVICE	"/proc/sys/cpu/0/speed-max"

#define SONYPI_DEFAULT_AC_BN	255
#define SONYPI_DEFAULT_DC_BN	0

#define SONYPI_MIN_BRIGHTNESS	0
#define SONYPI_MAX_BRIGHTNESS	255

#define PID_FILE _PATH_VARRUN	"spicd.pid"

#define MAX_FREQ_LENGTH		32

#endif /* _SPICCTRL_H_ */
