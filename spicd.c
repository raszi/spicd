/*
 * spicd: Simple Power Management for Sony VAIO notebooks
 *
 * Modified from spicctrl utility by Stelian Pop, Alcove and sources from
 * apmd.c by Rickard E. Faith <faith@acm.org>
 *
 * Copyright 2002 KARASZI Istvan <sonyd@spam.raszi.hu>
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/types.h>

#include <paths.h>
#include <syslog.h>
#include <sys/stat.h>
#include <signal.h>

#include "spicd.h"

#define VERSION "0.2, Nov 3, 2002"

/* Initialization */
static uid_t spicd_uid = 0;
static int sonypi_fd = -1;
static int cpufreq_fd = -1;
static int debug = 0;

/* Function prototypes */
int main(int, char *[]);
int sonypi_open(void);
int cpufreq_open(void);

unsigned int get_freq(const char *device);

static int cpufreq_ioctl(int, int);
static int spic_ioctl(int, int, void *);
static void usage(void);
static void version(void);
static void error(char *str, int doexit);

static void sig_handler(int sig) {
    syslog(LOG_INFO, "Exiting");
    unlink(PID_FILE);
    exit(0);
}

/*
 * Main function.
 */
int main(int argc, char **argv) {
	__u8 value8, current, last = 0;
	int first = 1;
	int pid;
	int fd;
	int c;
	int ac_brightness = SONYPI_DEFAULT_AC_BN;
	int dc_brightness = SONYPI_DEFAULT_DC_BN;

	int cpufreq = 0;
	unsigned int freq;
	unsigned int minimal_frequency = get_freq(CPUFREQ_MIN_DEVICE);
	unsigned int maximal_frequency = get_freq(CPUFREQ_MAX_DEVICE);

	unsigned int dc_freq = minimal_frequency;
	unsigned int ac_freq = maximal_frequency;

	FILE *str;

	static struct option longopts[] = {
		{ "debug", 0, 0, 'd' },
		{ "dc-brightness", 1, 0, 'D' },
		{ "ac-brightness", 1, 0, 'A' },
		{ "version", 0, 0, 'V' },
		{ "help", 0, 0, '?' },
		{ "dc-frequency", 1, 0, 'm' },
		{ "ac-frequency", 1, 0, 'M' },
		{ "disable-cpufreq", 0, 0, 'C' },
		{ NULL, 0, 0, 0 }
	};

	while((c = getopt_long(argc, argv, "?dVCD:A:m:M:", longopts, NULL)) != -1) {
		switch(c) {
			case 'd':
				debug = 1;
				break;
			case 'D':
				dc_brightness = atoi(optarg);
				break;
			case 'A':
				ac_brightness = atoi(optarg);
				break;
			case 'V':
				version();
				break;
			case 'm':
				dc_freq = strtoul(optarg, NULL, 10);
				break;
			case 'M':
				ac_freq = strtoul(optarg, NULL, 10);
				break;
			case 'C':
				cpufreq = 0;
				break;
			case '?':
			default:
				usage();
		}
	}

	if ((dc_brightness < SONYPI_MIN_BRIGHTNESS) || (dc_brightness > SONYPI_MAX_BRIGHTNESS)) {
		if (debug) {
			char *errorstr;
			asprintf(&errorstr, "DC brigthness is out of range. Setting to default value (%d).", SONYPI_DEFAULT_DC_BN);
			error(errorstr, 0);
		}

		dc_brightness = SONYPI_DEFAULT_DC_BN;
	}

	if ((ac_brightness < SONYPI_MIN_BRIGHTNESS) || (ac_brightness > SONYPI_MAX_BRIGHTNESS)) {
		if (debug) {
			char *errorstr;
			asprintf(&errorstr, "spicd: AC brigthness is out of range. Setting to default value (%d).", SONYPI_DEFAULT_AC_BN);
			error(errorstr, 0);
		}

		ac_brightness = SONYPI_DEFAULT_DC_BN;
	}

	if ((dc_freq > maximal_frequency) || (dc_freq < minimal_frequency)) {
		if (debug) {
			char *errorstr;
			asprintf(&errorstr, "DC frequency is out of range. Setting to default value (%d).", minimal_frequency);
			error(errorstr, 0);
		}

		dc_freq =  minimal_frequency;
	}
	if ((ac_freq > maximal_frequency) || (ac_freq < minimal_frequency)) {
		if (debug) {
			char *errorstr;
			asprintf(&errorstr, "AC frequency is out of range. Setting to default value (%d).", maximal_frequency);
			error(errorstr, 0);
		}

		ac_freq =  maximal_frequency;
	}

	if (!access(PID_FILE, R_OK)) {
		if ((str = fopen(PID_FILE, "r"))) {
			fscanf(str, "%d", &pid);
			fclose(str);

			if (!kill(pid, 0) || errno == EPERM) {
				fprintf(stderr,
					"A spicd already appears to be running as process %d\n"
					"If in reality no spicd is running, remove %s\n",
					pid, PID_FILE);
				exit(1);
			}
		}
	}

	if ((spicd_uid = getuid())) {
		fprintf(stderr, "spicd: must be run as root\n");
		exit(1);
	}

	openlog("spicd", LOG_PID | LOG_CONS, LOG_DAEMON);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN) signal(SIGINT, sig_handler);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN) signal(SIGQUIT, sig_handler);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN) signal(SIGTERM, sig_handler);

	if ((pid = fork())) { /* parent */
		if (pid < 0) {
			syslog(LOG_INFO, "fork() failed: %m");
			unlink(PID_FILE);
			exit(1);
		}

		if ((str = fopen(PID_FILE, "w"))) {
			fprintf(str, "%d\n", pid);
			fclose(str);
		}
		exit(0);
	} /* endif parent */

	/* Child.  Follow the daemon rules in
	 * W. Richard Stevens. Advanced Programming
	 * in the UNIX Environment (Addison-Wesley
	 * Publishing Co., 1992). Page 417.).
	 */
	if (setsid() < 0) {
		syslog(LOG_INFO, "setsid() failed: %m");
		unlink(PID_FILE);
		exit(1);
	}
	chdir("/");
	close(0);
	close(1);
	close(2);
	umask(0);

	syslog(LOG_INFO, "daemon starting");

	if ((cpufreq = cpufreq_open()) < 0) {
		if (debug) syslog(LOG_INFO, "cpufreq does not suppported");
	} else {
		cpufreq_fd = cpufreq;
	}

	if ((fd = sonypi_open()) < 0) {
		syslog(LOG_INFO, "sonypi_open() failed: %m");
		unlink(PID_FILE);
		exit(1);
	} else {
		sonypi_fd = fd;
	}

	while(1) {
		if (spic_ioctl(sonypi_fd, SONYPI_IOCGBATFLAGS, &value8) < 0) exit(2);
		current = value8 & SONYPI_BFLAGS_AC;
		if ((current != last) && !first) {
			last = current;

			if (debug) syslog(LOG_DEBUG, "AC status changed");

			if (current) {
				/* AC adaptor plugged in */
				value8 = ac_brightness;
				freq = ac_freq;
			} else {
				value8 = dc_brightness;
				freq = dc_freq;
			}

			if (debug) {
				char *debugmsg;

				asprintf(&debugmsg, "Setting LCD brightness to %d", value8);
				syslog(LOG_DEBUG, debugmsg);

				if (cpufreq_fd != -1) {
					asprintf(&debugmsg, "Setting CPU speed to %dKHz", freq);
					syslog(LOG_DEBUG, debugmsg);
				}
			}

			spic_ioctl(sonypi_fd, SONYPI_IOCSBRT, &value8);

			if (cpufreq_fd != -1) cpufreq_ioctl(cpufreq_fd, dc_freq);
		} else if (first) {
			first = 0;
			last = current;
		}

		/* sleep 2 seconds */
		sleep(2);
	}
}

/* print error message and exit if needed */
static void error(char *str, int doexit) {
	fprintf(stderr, "spicd: %s\n", str);
	if (doexit) exit(1);
}

/*
 * Open cpufreq device.
 *
 * Return: descriptor if success, -1 if an error occured.
 */
int cpufreq_open(void) {
	int fd;

	if ((fd = open(CPUFREQ_DEVICE, O_RDWR)) < 0) {
		/* failed to open */
		syslog(LOG_INFO, "Failed to open cpufreq device!");
		return(-1);
	}

	return(fd);
}

unsigned int get_freq(const char *device) {
	unsigned int freq = 0;
	FILE *frq;

	if ((frq = fopen(device, "r"))) {
		fscanf(frq, "%d", &freq);
		fclose(frq);
	} else {
		/* failed to open */
		syslog(LOG_INFO, "Failed to open cpufreq device!");
	}

	return(freq);
}

/*
 * Open Sony SPIC device.
 *
 * Return: descriptor if success, -1 if an error occured.
 */
int sonypi_open(void) {
	int fd;

	if ((fd = open(SONYPI_DEVICE, O_RDWR)) < 0) {
		/* failed to open */
		syslog(LOG_INFO, "Failed to open sonypi device!");
		return(-1);
		/* it could be an mknod here, but I don't want to write it :) */
	}

	return(fd);
}

/*
 * Sends a ioctl command to the cpufreq driver.
 *
 * Return: 0 if success, -1 if an error occured.
 */
static int cpufreq_ioctl(int fd, int freq) {
	char *buf;
	asprintf(&buf, "%d", freq);

	if (write(fd, buf, strlen(buf)) == -1) {
		syslog(LOG_INFO, "write() failed!");
		return(-1);
	}

	return(0);
}

/*
 * Sends a ioctl command to the SPIC driver.
 *
 * Return: 0 if success, -1 if an error occured.
 */
static int spic_ioctl(int fd, int ioctlno, void *param) {
	if (ioctl(fd, ioctlno, param) < 0) {
		syslog(LOG_INFO, "ioctl() failed!");
		return(-1);
	}

	return(0);
}

/*
 * Print usage information
 */
static void usage(void) {
	fprintf(stderr, "Usage: spicd [OPTIONS]...\n\n");
	fprintf(stderr, "Commands:\n");
	fprintf(stderr, "\t-d, --debug\t\t\tdebug mode\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "\t-D, --dc-brightness=NUM\t\tLCD brightness value without AC adaptor (from 0..255, default: %d)\n", SONYPI_DEFAULT_DC_BN);
	fprintf(stderr, "\t-A, --ac-brightness=NUM\t\tLCD brightness value with AC adaptor (default: %d)\n", SONYPI_DEFAULT_AC_BN);
	fprintf(stderr, "\n");
	fprintf(stderr, "\t-C, --disable-cpufreq\t\tdisable CPU frequency support\n");
	fprintf(stderr, "\t-m, --dc-frequency=NUM\t\tCPU frequency without AC adaptor\n");
	fprintf(stderr, "\t-M, --ac-frequency=NUM\t\tCPU frequency with AC adaptor\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "\t-V, --version\t\t\tprint version information\n");
	fprintf(stderr, "\t-?, --help\t\t\tprint this usage\n");

	exit(1);
}

/*
 * Print version information.
 */
static void version(void) {
	char *versionstr;

	asprintf(&versionstr, "spicctrld %s", VERSION);
	error(versionstr, 1);
}
