/*
 ============================================================================
 Name        : libleiodchw.c
 Author      : AK
 Version     : V2.00
 Copyright   : Property of Londelec UK Ltd
 Description : LEIODC CPU pin control and serial interface configuration library

  Change log :

  *********V2.00 31/10/2022**************
  Compatible with new GPIO interface in linux, now using /dev/gpio

  *********V1.01 02/07/2015**************
  Library initialization constructor function created
  Set pin to input function created
  gpio direction and value file 0666 permission setting function created

  *********V1.00 05/03/2015**************
  Initial revision

 ============================================================================
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>			// strcat, strcpy
#include <stdarg.h>
#include <errno.h>			// Error number
#include <fcntl.h>			// File controls
#include <unistd.h>			// getcwd, access
#include <sys/ioctl.h>
#include <sys/stat.h>		// Open function constants
#include <linux/gpio.h>		// cdev GPIO UAPI
#include <linux/serial.h>	// serial port UAPI

#include "libleiodchw.h"


#define	LIBVERSION_MAJOR		2			// Library version number major
#define	LIBVERSION_MINOR		0			// Library version number minor
#if LIBVERSION_MINOR < 10
#define	LIBVERSION_10TH_ZERO	"0"
#else
#define	LIBVERSION_10TH_ZERO	""
#endif
const lechar *libleiodcVersion = " libleiodcVersion="\
								STRINGIFY(LIBVERSION_MAJOR)\
								"."\
								LIBVERSION_10TH_ZERO\
								STRINGIFY(LIBVERSION_MINOR)\
								" ";
#include "builddate.txt"



#ifdef GLOBAL_DEBUG
//#define DEBUG_RS485_AS_232
#define DEBUG_VERBOSE_CDEV_FD
//#define DEBUG_ENABLE_HB_OUTPUT
//#define DEBUG_LINUX_SERIAL
#endif	// GLOBAL_DEBUG

/*
 * GPIO char dev constants
 */
#define	GPIO_CDEV_CHIP		"/dev/gpiochip"		// Path of the gpio chip device

/*
 * GPIO sysfs constants
 */
#define GPIO_PATH_LENGTH	256					// Length of the gpio directory name
#define	GPIO_SYSFS_DIR		"/sys/class/gpio/"	// Path of the gpio sysfs directory
#define	GPIO_DIR_PREF		"gpio"				// gpio name prefix
#define	GPIO_EXPORT			"export"			// gpio export file
#define	GPIO_DIRECTION		"/direction"		// gpio direction file
#define	GPIO_VALUE			"/value"			// gpio value file
#define	GPIO_OUT			"out"				// gpio out value
#define	GPIO_IN				"in"				// gpio in value
#define	GPIO_HIGH			"high"				// gpio direction out and high value
#define	GPIO_LOW			"low"				// gpio direction out and low value

#define __EXPORT_SYMBOL(sym, sec) \
	extern typeof(sym) sym;

#define EXPORT_SYMBOL(sym)		__EXPORT_SYMBOL(sym, "")


/*
 * Linux kernel structure
 */
#ifdef DEBUG_LINUX_SERIAL
#ifndef _LINUX_SERIAL_H
typedef struct serial_rs485 {
	uint32_t	flags;			/* RS485 feature flags */
#define SER_RS485_ENABLED			(1 << 0)	/* If enabled */
#define SER_RS485_RTS_ON_SEND		(1 << 1)	/* Logical level for
						   RTS pin when
						   sending */
#define SER_RS485_RTS_AFTER_SEND	(1 << 2)	/* Logical level for
						   RTS pin after sent*/
#define SER_RS485_RX_DURING_TX		(1 << 4)
	uint32_t	delay_rts_before_send;	/* Delay before send (milliseconds) */
	uint32_t	delay_rts_after_send;	/* Delay after send (milliseconds) */
	uint32_t	padding[5];		/* Memory is cheap, new structs are a royal PITA .. */
} linux_serial_rs485;
#endif // _LINUX_SERIAL_H
#endif // DEBUG_LINUX_SERIAL


static const lechar *gpiodirpref = GPIO_SYSFS_DIR GPIO_DIR_PREF;
static const lechar *gpioexport = GPIO_SYSFS_DIR GPIO_EXPORT;
static const lechar *gpiodirection = GPIO_DIRECTION;
static const lechar *sloginvalidpin = "GPIO pad is not mapped for the requested lepin[%u]";
static const lechar *sloginvalidmode = "Internal error GPIO access mode=%u is not implemented";
lechar LibErrorString[512];


/*
 * GPIO access modes
 */
typedef enum {
	mode_none = 0,
	mode_sysfs,
	mode_cdev,
} libmode_e;
static libmode_e libmode;


/*
 * i.MX28 CPU pads
 */
typedef enum {
	mx28pad_LCD_RD_E__GPIO_1_24		= 56,				// COM1 RS232 enabled = 1
	mx28pad_LCD_WR_RWN__GPIO_1_25	= 57,				// COM1 1st RS485_P[12] (U11) RX enabled = 0
	mx28pad_LCD_CS__GPIO_1_27		= 59,				// COM1 1st RS485_P[12] (U11) TX enabled = 1
	mx28pad_LCD_VSYNC__GPIO_1_28	= 60,				// COM1 2nd RS485_P[34] (U12) RX enabled = 0
	mx28pad_LCD_HSYNC__GPIO_1_29	= 61,				// COM1 2nd RS485_P[34] (U12) TX enabled = 1

	mx28pad_LCD_D07__GPIO_1_7		= 39,				// COM2 RS232 enabled = 1
	mx28pad_LCD_D08__GPIO_1_8		= 40,				// COM2 1st RS485_P[12] (U14) RX enabled = 0
	mx28pad_LCD_D09__GPIO_1_9		= 41,				// COM2 1st RS485_P[12] (U14) TX enabled = 1
	mx28pad_LCD_D10__GPIO_1_10		= 42,				// COM2 2nd RS485_P[34] (U15) RX enabled = 0
	mx28pad_LCD_D11__GPIO_1_11		= 43,				// COM2 2nd RS485_P[34] (U15) TX enabled = 1

	mx28pad_LCD_D12__GPIO_1_12		= 44,				// COM3 RS232 enabled = 1
	mx28pad_LCD_D13__GPIO_1_13		= 45,				// COM3 1st RS485_P[12] (U17) RX enabled = 0
	mx28pad_LCD_D14__GPIO_1_14		= 46,				// COM3 1st RS485_P[12] (U17) TX enabled = 1
	mx28pad_LCD_D15__GPIO_1_15		= 47,				// COM3 2nd RS485_P[34] (U18) RX enabled = 0
	mx28pad_LCD_D16__GPIO_1_16		= 48,				// COM3 2nd RS485_P[34] (U18) TX enabled = 1

	mx28pad_SSP3_MISO__GPIO_2_26	= 90,				// Heartbeat
	mx28pad_SAIF1_SDATA0__GPIO_3_26	= 122,				// MU609 reset (no longer used)
	mx28pad_PWM4__GPIO_3_29			= 125,				// MU609 power
} cpupad_e;


static const cpupad_e PinMapTable[] = {
	[lepin_COM1_RS232] 		= mx28pad_LCD_RD_E__GPIO_1_24,
	[lepin_COM1_RS422_RX1]	= mx28pad_LCD_WR_RWN__GPIO_1_25,
	[lepin_COM1_RS422_TX1]	= mx28pad_LCD_CS__GPIO_1_27,
	[lepin_COM1_RS422_RX2]	= mx28pad_LCD_VSYNC__GPIO_1_28,
	[lepin_COM1_RS422_TX2]	= mx28pad_LCD_HSYNC__GPIO_1_29,

	[lepin_COM2_RS232]		= mx28pad_LCD_D07__GPIO_1_7,
	[lepin_COM2_RS422_RX1]	= mx28pad_LCD_D08__GPIO_1_8,
	[lepin_COM2_RS422_TX1]	= mx28pad_LCD_D09__GPIO_1_9,
	[lepin_COM2_RS422_RX2]	= mx28pad_LCD_D10__GPIO_1_10,
	[lepin_COM2_RS422_TX2]	= mx28pad_LCD_D11__GPIO_1_11,

	[lepin_COM3_RS232]		= mx28pad_LCD_D12__GPIO_1_12,
	[lepin_COM3_RS422_RX1]	= mx28pad_LCD_D13__GPIO_1_13,
	[lepin_COM3_RS422_TX1]	= mx28pad_LCD_D14__GPIO_1_14,
	[lepin_COM3_RS422_RX2]	= mx28pad_LCD_D15__GPIO_1_15,
	[lepin_COM3_RS422_TX2]	= mx28pad_LCD_D16__GPIO_1_16,

	[lepin_heartbeat]		= mx28pad_SSP3_MISO__GPIO_2_26,
	[lepin_modem_reset]		= mx28pad_SAIF1_SDATA0__GPIO_3_26,
	[lepin_modem_power]		= mx28pad_PWM4__GPIO_3_29,
};


#define UART_CTRL_PIN_COUNT		5
static const struct {
	leiodcpin_e		lepin[UART_CTRL_PIN_COUNT];
} UartpinTable[] = {
	{{lepin_COM1_RS232, lepin_COM1_RS422_RX1, lepin_COM1_RS422_TX1, lepin_COM1_RS422_RX2, lepin_COM1_RS422_TX2}},
	{{lepin_COM2_RS232, lepin_COM2_RS422_RX1, lepin_COM2_RS422_TX1, lepin_COM2_RS422_RX2, lepin_COM2_RS422_TX2}},
	{{lepin_COM3_RS232, lepin_COM3_RS422_RX1, lepin_COM3_RS422_TX1, lepin_COM3_RS422_RX2, lepin_COM3_RS422_TX2}},
};

static const struct {
	int				value[UART_CTRL_PIN_COUNT];
} UartoeTable[] = {
	[leuart_RS232] = 	{{1, 1, 0, 1, 0}},
#ifdef DEBUG_RS485_AS_232
	[leuart_RS485def] =	{{1, 1, 0, 1, 0}},
#else
	[leuart_RS485def] =	{{0, 0, 0, 1, 0}},
#endif
	[leuart_RS485rev] =	{{0, 1, 0, 0, 0}},
	[leuart_RS422def] = {{0, 1, 1, 0, 0}},
	[leuart_RS422rev] = {{0, 0, 0, 1, 1}},
};


/*
 * cdev GPIO handle types
 */
enum {
	handle_uart = 0,
	handle_modem,
	handle_heartbeat,
	handle_count		/* Number of handles, must be the last */
};

struct handle_s {
	fddef			fd;
	const lechar	*name;
	leiodcpin_e		minp;
	leiodcpin_e		maxp;
};
static struct handle_s glrhandles[handle_count] = {
	[handle_uart]		= {0, "uart-gpio", lepin_COM1_RS232, lepin_COM3_RS422_TX2},
	[handle_modem]		= {0, "modem-gpio", lepin_modem_reset, lepin_modem_power},
	[handle_heartbeat]	= {0, "hb-gpio", lepin_heartbeat, lepin_heartbeat},
};


/*
 * Error logger macros
 */
#define ERROR_LOGGER(...) _error_logger(__func__, __LINE__, NULL, __VA_ARGS__);
#define ERROR_STD_LOGGER(...) _error_logger(__func__, __LINE__, strerror(errno), __VA_ARGS__);
#define GPIO_CHIP_FROM_PAD(mpad) (mpad >> 5)
#define GPIO_CHIP_MASK 0x1f




/*
 * Log error messages
 * [05/03/2015]
 * Variable argument used
 * [31/10/2022]
 */
static void _error_logger(const lechar *cfunc, int lineno, const lechar *errstr, const lechar *format, ...) {
	va_list 		ap;
	lechar 			argbuf[256];
	int 			retstat;


	va_start(ap, format);
	retstat = vsnprintf(argbuf, sizeof(argbuf) - 1, format, ap); // vnsprintf returns what it would have written, even if truncated
	va_end(ap);

	if (retstat > sizeof(argbuf) - 1)
		retstat = sizeof(argbuf) - 1;

	argbuf[retstat] = 0;
	strcpy(LibErrorString, "libleiodc ");
	strcat(LibErrorString, cfunc);
	strcat(LibErrorString, "(): ");
	strcat(LibErrorString, argbuf);

	if (errstr) {
		strcat(LibErrorString, ": ");
		strcat(LibErrorString, errstr);
	}

#ifdef DEBUG_VERBOSE_CDEV_FD
	printf("DEBUG: %s\n", LibErrorString);
#endif
}


/*
 * Library initialization constructor
 * [03/07/2015]
 */
static void LELIBCONSTRUCTOR leiodc_init(void) {

	LibErrorString[0] = '\0';
}


/*
 * Get CPU pad number from the table
 * [05/03/2015]
 */
static cpupad_e _cpu_pad_get(leiodcpin_e lepin) {

	if (lepin < ARRAY_SIZE(PinMapTable))
		return PinMapTable[lepin];
	return 0;
}



/*
 * Initialize GPIO access mode:
 * /sys/class/gpio/	=> Legacy sysfs
 * /dev/gpiochipN	=> Linux CDEV API
 * [26/12/2022]
 */
static int _lib_mode(void) {
	const lechar	*syserr, *cdeverr;


	if (access(GPIO_CDEV_CHIP "0", F_OK) == 0) {
		/*
		 * /dev/gpiochip0 found, using Linux CDEV API
		 */
		libmode = mode_cdev;
		return EXIT_SUCCESS;
	}
	cdeverr = strerror(errno);

	if (access(GPIO_SYSFS_DIR, X_OK) == 0) {
		/*
		 * Legacy /sys/class/gpio/ found
		 */
		libmode = mode_sysfs;
		return EXIT_SUCCESS;
	}
	syserr = strerror(errno);

	ERROR_LOGGER("'" GPIO_CDEV_CHIP "0' : %s; '" GPIO_SYSFS_DIR "' : %s", cdeverr, syserr)
	return EXIT_FAILURE;
}


/*
 * close() function wrapper
 * [05/03/2015]
 */
static int _close(fddef *fdptr, const lechar *filename, int verbose) {
	int retstat;


	if ((retstat = close(*fdptr))) {
		if (verbose) {
			ERROR_STD_LOGGER("close(%s)", filename)
		}
	}
	*fdptr = 0;

	return (retstat) ? EXIT_FAILURE : EXIT_SUCCESS;
}


/*
 * Write string to the specified file
 * [05/03/2015]
 */
static int _sysfile_write(fddef *fdptr, const lechar *filename, const lechar *wrstring, int closefl) {
	int retstat = EXIT_SUCCESS;


	if (!(*fdptr)) {
		*fdptr = open(filename, O_WRONLY);
		if (*fdptr < 1) {
			ERROR_STD_LOGGER("open(%s)", filename)
			*fdptr = 0;
			return EXIT_FAILURE;
		}
	}

	if (write(*fdptr, wrstring, strlen(wrstring)) < 0) {
		ERROR_STD_LOGGER("write(%s)", filename)
		retstat = EXIT_FAILURE;
	}

	if (closefl) {
		if (_close(fdptr, filename, BOOL_CHECK(retstat == EXIT_SUCCESS)) == EXIT_FAILURE)
			retstat = EXIT_FAILURE;
	}

	return retstat;
}


/*
 * Perform an action on sysfs GPIO
 * [05/03/2015]
 */
static int _sysfs_action(leiodcpin lepin, const lechar *pathpostfix, const lechar *statec) {
	lechar			gpiopath[GPIO_PATH_LENGTH];
	size_t			len;
	cpupad_e		mxpad;
	fddef			tmpfd = 0;


	mxpad = _cpu_pad_get(lepin);	// This is CPU gpio number
	if (!mxpad) {
		ERROR_LOGGER(sloginvalidpin, lepin)
		return EXIT_FAILURE;
	}

	len = strlen(gpiodirpref);
	strcpy(gpiopath, gpiodirpref);			// Individual pin directory name without a gpio number
	sprintf(&gpiopath[len], "%u", mxpad);	// Append gpio number to the directory
	strcat(gpiopath, pathpostfix);			// Append file name e.g. '/direction' or '/value'


	if (access(gpiopath, W_OK) != 0) {
		ERROR_STD_LOGGER("Can't write '%s'", gpiopath)
		return EXIT_FAILURE;
	}
	return _sysfile_write(&tmpfd, gpiopath, statec, 1);
}


/*
 * ioctl() of the cdev GPIO line handle
 * [26/12/2022]
 */
static int _cdev_line_ioctl(struct handle_s *lrhandle, __u32 pinmask, __u32 valmask, enum gpio_v2_line_flag gflag) {
	struct gpio_v2_line_config linecfg;


	if (!lrhandle->fd) {
		ERROR_LOGGER("GPIO line handle is not initialized for '%s'", lrhandle->name)
		return EXIT_FAILURE;
	}

	memset(&linecfg, 0, sizeof(linecfg));
	/*
	 * Don't use global flag as it applies to all pins,
	 * even those not selected by pin mask.
	 * linecfg.flags = gflag;
	 */
	linecfg.num_attrs = 1;
	linecfg.attrs[0].mask = pinmask;
	linecfg.attrs[0].attr.id = GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES;
	linecfg.attrs[0].attr.values = valmask;

	if (gflag) {
		linecfg.num_attrs++;
		linecfg.attrs[1].mask = pinmask;
		linecfg.attrs[1].attr.id = GPIO_V2_LINE_ATTR_ID_FLAGS;
		linecfg.attrs[1].attr.flags = gflag;
	}

	if (ioctl(lrhandle->fd, GPIO_V2_LINE_SET_CONFIG_IOCTL, &linecfg)) {
		ERROR_STD_LOGGER("GPIO ioctl(%s, %s)",
				lrhandle->name, STRINGIFY_(GPIO_V2_LINE_SET_CONFIG_IOCTL))
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


/*
 * Perform an action on cdev GPIO
 * [25/12/2022]
 */
static int _cdev_action(leiodcpin lepin, int state, enum gpio_v2_line_flag gflag) {
	__u32	pbit;
	struct handle_s *handle = NULL;
	int 	i;


	for (i = 0; i < ARRAY_SIZE(glrhandles); i++) {
		if ((lepin >= glrhandles[i].minp) && (lepin <= glrhandles[i].maxp)) {
			handle = &glrhandles[i];
			break;
		}
	}

	if (!handle) {
		ERROR_LOGGER("GPIO line handle for lepin (%u) doesn't exist (contact support)", lepin)
		return EXIT_FAILURE;
	}

	pbit = 1 << (lepin - handle->minp);
	return _cdev_line_ioctl(handle, pbit, state ? pbit : 0, gflag);
}


/*
 * Check and set file permissions
 * [09/07/2015]
 */
static int _permission_set(const lechar *filepath) {
	struct stat 	fstat;
	mode_t			fperms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |  S_IROTH | S_IWOTH;


	if (stat(filepath, &fstat) < 0) {			// Check if file exists and read permissions
		ERROR_STD_LOGGER("stat(%s)", filepath)
		return EXIT_FAILURE;
	} else {
		if ((fstat.st_mode & fperms) != fperms) {	// Different permissions required
			if (chmod(filepath, fperms) < 0) {		// Set new permissions
				ERROR_STD_LOGGER("chmod(%s)", filepath)
				return EXIT_FAILURE;
			}
		}
	}
	return EXIT_SUCCESS;
}


/*
 * Initialize sysfs GPIOs
 * [31/10/2022]
 */
static int _init_sysfs_file(lechar *gpiopath, cpupad_e cpupad, fddef *fdptr, int dirlen) {

	*fdptr = 0;

	sprintf(&gpiopath[dirlen], "%u", cpupad);	// Append gpio number to the directory string
	if (access(gpiopath, X_OK) != 0) {			// Check if gpio directory already exists
		if (_sysfile_write(fdptr, gpioexport, &gpiopath[dirlen], 0) == EXIT_FAILURE)
			return EXIT_FAILURE;

		if (access(gpiopath, X_OK) != 0) {	// Check if gpio directory created
			ERROR_STD_LOGGER("GPIO export failed to create '%s'", gpiopath)
			return EXIT_FAILURE;
		}
	}

	sprintf(&gpiopath[dirlen], "%u%s", cpupad, gpiodirection);	// Set direction file permissions
	if (_permission_set(gpiopath) == EXIT_FAILURE)
		return EXIT_FAILURE;

	sprintf(&gpiopath[dirlen], "%u%s", cpupad, GPIO_VALUE);		// Set value file permissions
	if (_permission_set(gpiopath) == EXIT_FAILURE)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}


/*
 * Initialize cdev GPIOs
 * [25/12/2022]
 */
static int _init_cdev_chip(lechar *gpiopath, struct handle_s *lrhandle, int dirlen) {
	fddef			fd;
	int				i;
	int				chip = GPIO_CHIP_FROM_PAD(_cpu_pad_get(lrhandle->minp));
	struct gpio_v2_line_request linereq;


	memset(&linereq, 0, sizeof(linereq));

	sprintf(&gpiopath[dirlen], "%u", chip);		// Append chip number to the directory string
	if (access(gpiopath, R_OK) != 0) {			// Check if gpio chip exists
		ERROR_STD_LOGGER("GPIO chip '%s' doesn't exist", gpiopath)
		return EXIT_FAILURE;
	}


	for (i = lrhandle->minp; i <= lrhandle->maxp; i++) {
		linereq.offsets[linereq.num_lines] = _cpu_pad_get(i) & GPIO_CHIP_MASK;
		linereq.num_lines++;
	}

	strcpy(linereq.consumer, lrhandle->name);
	/*
	 * Don't use global flag as it applies to all pins,
	 * even those not selected by pin mask.
	 * linereq.config.flags = 0;
	 */

#ifdef DEBUG_ENABLE_HB_OUTPUT
	if (lrhandle->minp == lepin_heartbeat) {
		linereq.config.num_attrs = 1;
		linereq.config.attrs[0].mask = (1 << linereq.num_lines) - 1;
		linereq.config.attrs[0].attr.id = GPIO_V2_LINE_ATTR_ID_FLAGS;
		linereq.config.attrs[0].attr.flags = GPIO_V2_LINE_FLAG_OUTPUT;
		printf("DEBUG: %s enabling OUTPUT: chip=%u pinmask=0x%llx\n", lrhandle->name, chip, linereq.config.attrs[0].mask);
	}
#endif

	fd = open(gpiopath, O_RDONLY);
	if (fd < 1) {
		ERROR_STD_LOGGER("open(%s)", gpiopath)
		return EXIT_FAILURE;
	}

	if (ioctl(fd, GPIO_V2_GET_LINE_IOCTL, &linereq)) {
		ERROR_STD_LOGGER( "GPIO ioctl(%s, %s)",
				gpiopath, STRINGIFY_(GPIO_V2_GET_LINE_IOCTL))
	}
	else {
		if (linereq.fd > 0)
			lrhandle->fd = linereq.fd;
		else {
			ERROR_LOGGER("GPIO chip '%s' handle fd=%i", gpiopath, linereq.fd)
		}
	}

	_close(&fd, gpiopath, BOOL_CHECK(linereq.fd > 0));

#ifdef DEBUG_VERBOSE_CDEV_FD
	printf("DEBUG: %s handle fd=%i \n", lrhandle->name, linereq.fd);
#endif
	return (linereq.fd > 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


/*
 * Initialize required pins
 * return failure if initialization failed
 * [11/03/2015]
 * File permission setting added
 * [09/07/2015]
 * Char device support added
 * [26/12/2022]
 */
int leiodc_pininit(LIBARGDEF_INIT) {
	int				i, h, retstat = EXIT_SUCCESS;
	lechar			gpiopath[GPIO_PATH_LENGTH];
	size_t			len = 0;
	leiodcpin		first = 0;
	cpupad_e		cpupad;
	fddef			fd = 0;


	if (!libmode) {
		if (_lib_mode() == EXIT_FAILURE)
			return EXIT_FAILURE;
	}

	switch (libmode) {
	case mode_cdev:
		len = strlen(GPIO_CDEV_CHIP);
		strcpy(gpiopath, GPIO_CDEV_CHIP);
		if (!pintable) {
			ERROR_LOGGER("Can't initialize 'all' pins, need to pass pintable[] argument to %s()", __func__)
			return EXIT_FAILURE;
		}
		break;

	case mode_sysfs:
 		len = strlen(gpiodirpref);
 		strcpy(gpiopath, gpiodirpref);
 		break;

	default:
		ERROR_LOGGER(sloginvalidmode, libmode)
		return EXIT_FAILURE;
	}


	if (!pintable) {
		/*
		 * Initialization starts from the first pin if table argument is not passed
		 * (only used for sysfs)
		 */
		first = 1;
	}


	for (i = first; i < pincount; i++) {
		switch (libmode) {
		case mode_cdev:
			for (h = 0; h < ARRAY_SIZE(glrhandles); h++) {
				if ((pintable[i] >= glrhandles[h].minp) && (pintable[i] <= glrhandles[h].maxp)) {
					if (!glrhandles[h].fd) {
						if (_init_cdev_chip(gpiopath, &glrhandles[h], len) == EXIT_FAILURE) {
							return EXIT_FAILURE;
						}
					}
					break;
				}
			}
			break;


		case mode_sysfs:
			/*
			 * CPU gpio number from user table or calculate based on a counter
			 */
			cpupad = _cpu_pad_get((pintable) ? pintable[i] : i);

			if (!cpupad) {
				ERROR_LOGGER(sloginvalidpin, (pintable) ? pintable[i] : i)
				retstat = EXIT_FAILURE;
				goto failed;
			}

			if (_init_sysfs_file(gpiopath, cpupad, &fd, len) == EXIT_FAILURE) {
				retstat = EXIT_FAILURE;
				goto failed;
			}
			break;


		default:
			ERROR_LOGGER(sloginvalidmode, libmode)
			return EXIT_FAILURE;
		}
	}


	failed:
	if (fd) {
		if (_close(&fd, gpioexport, BOOL_CHECK(retstat == EXIT_SUCCESS)) == EXIT_FAILURE)
			retstat = EXIT_FAILURE;
	}
	return retstat;
}
EXPORT_SYMBOL(leiodc_pininit)


/*
 * Make pin output and set state
 * return failure if failed
 * [06/12/2022]
 */
int leiodc_pinoutstate(LIBARGDEF_PINS) {

	switch (libmode) {
	case mode_cdev:
		return _cdev_action(lepin, state, GPIO_V2_LINE_FLAG_OUTPUT);

	case mode_sysfs:
		return _sysfs_action(lepin, gpiodirection, (state) ? GPIO_HIGH : GPIO_LOW);

	default:
		ERROR_LOGGER(sloginvalidmode, libmode)
		break;
	}
	return EXIT_FAILURE;
}
EXPORT_SYMBOL(leiodc_pinoutstate)


/*
 * Set state of the specified pin
 * return failure if failed
 * [06/03/2015]
 */
int leiodc_pinstate(LIBARGDEF_PINS) {

	switch (libmode) {
	case mode_cdev:
		return _cdev_action(lepin, state, GPIO_V2_LINE_FLAG_OUTPUT);

	case mode_sysfs:
		return _sysfs_action(lepin, GPIO_VALUE, (state) ? "1" : "0");

	default:
		ERROR_LOGGER(sloginvalidmode, libmode)
		break;
	}
	return EXIT_FAILURE;
}
EXPORT_SYMBOL(leiodc_pinstate)


/*
 * Make pin input
 * return failure if failed
 * [06/12/2022]
 */
int leiodc_pininput(LIBARGDEF_PINS) {

	switch (libmode) {
	case mode_cdev:
		return _cdev_action(lepin, 0, GPIO_V2_LINE_FLAG_INPUT);

	case mode_sysfs:
		return _sysfs_action(lepin, gpiodirection, GPIO_IN);

	default:
		ERROR_LOGGER(sloginvalidmode, libmode)
		break;
	}
	return EXIT_FAILURE;
}
EXPORT_SYMBOL(leiodc_pininput)


/*
 * Set interface mode of the UART
 * return failure if failed
 * [14/04/2015]
 * Pin table created
 * [26/12/2022]
 */
int leiodc_uartint(LIBARGDEF_UART) {
	int i;
	 struct serial_rs485 rs485conf;
	__u32 pbit, pinmask = 0, valmask = 0;


	memset(&rs485conf, 0, sizeof(rs485conf));

	if ((uartno > 2) || (interface >= ARRAY_SIZE(UartoeTable))) {
		//ERROR_LOGGER("UART number '%u' is to high, must be between 0...2", uartno)
		return EXIT_SUCCESS;		// Don't do anything if UART number argument is greater than 2
	}

	if (!libmode) {
		if (_lib_mode() == EXIT_FAILURE)
			return EXIT_FAILURE;
	}


	switch (libmode) {
	case mode_cdev:
		if (!glrhandles[handle_uart].fd) {
			/*
			 * One pin is sufficient to initialize UART GPIOs
			 */
			const leiodcpin pintable[] = {lepin_COM1_RS232};

			if (leiodc_pininit(pintable, ARRAY_SIZE(pintable)) == EXIT_FAILURE)
				return EXIT_FAILURE;
		}
		break;

	case mode_sysfs:
		if (leiodc_pininit(NULL, lepin_count) == EXIT_FAILURE)
			return EXIT_FAILURE;
		break;

	default:
		ERROR_LOGGER(sloginvalidmode, libmode)
		return EXIT_FAILURE;
	}


	for (i = 0; i < UART_CTRL_PIN_COUNT; i++) {
		switch (libmode) {
		case mode_cdev:
			pbit = 1 << (UartpinTable[uartno].lepin[i] - lepin_COM1_RS232);
			pinmask |= pbit;
			if (UartoeTable[interface].value[i])
				valmask |= pbit;
			break;

		case mode_sysfs:
			if (leiodc_pinoutstate(UartpinTable[uartno].lepin[i],
					UartoeTable[interface].value[i]) == EXIT_FAILURE)
				return EXIT_FAILURE;
			break;

		default:
			ERROR_LOGGER(sloginvalidmode, libmode)
			return EXIT_FAILURE;
		}
	}


	if (libmode == mode_cdev) {
		if (_cdev_line_ioctl(&glrhandles[handle_uart], pinmask, valmask, GPIO_V2_LINE_FLAG_OUTPUT) == EXIT_FAILURE)
			return EXIT_FAILURE;
	}


	switch (interface) {
	case leuart_RS485def:
#ifdef DEBUG_RS485_AS_232
		rs485conf.flags = (SER_RS485_ENABLED | SER_RS485_RTS_AFTER_SEND);
		switch (uartno) {
		case 0:
			rs485conf.padding[0] = 99;	// COM1 AUART RTS pin
			break;
		case 1:
			rs485conf.padding[0] = 103;	// COM2 AUART RTS pin
			break;
		case 2:
			rs485conf.padding[0] = 107;	// COM3 AUART RTS pin
			break;
		default:
			break;
		}
#else
		rs485conf.flags = (SER_RS485_ENABLED | SER_RS485_RTS_ON_SEND);
		// FIXME check if these flags are backward compatible
		rs485conf.flags |= (SER_RS485_ADDRB | SER_RS485_ADDR_RECV);
		rs485conf.padding[0] = _cpu_pad_get(UartpinTable[uartno].lepin[2]);
#endif
		break;

	case leuart_RS485rev:
		rs485conf.flags = (SER_RS485_ENABLED | SER_RS485_RTS_ON_SEND);
		rs485conf.flags |= (SER_RS485_ADDRB | SER_RS485_ADDR_RECV);
		rs485conf.padding[0] = _cpu_pad_get(UartpinTable[uartno].lepin[4]);
		break;

	default:
		break;
	}


	if (fdptr && rs485conf.flags) {
		if (ioctl(*fdptr, TIOCSRS485, &rs485conf)) {
			ERROR_STD_LOGGER( "UART ioctl(%u, %s, rs485.flags=0x%x rs485.padding[0]=%u)",
					*fdptr, STRINGIFY_(TIOCSRS485), rs485conf.flags, rs485conf.padding[0])
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}
EXPORT_SYMBOL(leiodc_uartint)


/*
 * Check library version
 * return failure if initialization failed
 * [11/03/2015]
*/
int leiodc_libverchk(LIBARGDEF_VERCHK) {
	float fval = minvers;


	if (minvers > ((LIBVERSION_MAJOR * 100) + LIBVERSION_MINOR)) {
		fval /= 100;
		ERROR_LOGGER("libleiodc.so library version %u.%02u is too old, version %.2f or newer is required.",
				LIBVERSION_MAJOR, LIBVERSION_MINOR, fval);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
EXPORT_SYMBOL(leiodc_libverchk)
