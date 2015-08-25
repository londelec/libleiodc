/*
 ============================================================================
 Name        : libleiodchw.c
 Author      : AK
 Version     : V1.01
 Copyright   : Property of Londelec UK Ltd
 Description : LEIODC processor pin control and serial interface configuration library

  Change log  :

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
#include <errno.h>			// Error number
#include <fcntl.h>			// File controls
#include <unistd.h>			// getcwd, access
#include <sys/ioctl.h>
#include <sys/stat.h>		// Open function constants


#include "libleiodchw.h"



#define	LIBVERSION_MAJOR		1			// Library version number major
#define	LIBVERSION_MINOR		1			// Library version number minor
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
#define DEBUG_RS485_AS_232
#endif	// GLOBAL_DEBUG


// GPIO definitions
#define	GPIO_ROOT_DIR		"/sys/class/gpio/"	// Path of the gpio directory
#define	GPIO_DIR_PREF		"gpio"				// gpio name prefix
#define	GPIO_EXPORT			"export"			// gpio export file
#define	GPIO_DIRECTION		"/direction"		// gpio direction file
#define	GPIO_VALUE			"/value"			// gpio value file
#define	GPIO_OUT			"out"				// gpio out value
#define	GPIO_IN				"in"				// gpio in value
#define	GPIO_HIGH			"high"				// gpio direction out and high value
#define	GPIO_LOW			"low"				// gpio direction out and low value




uint16_t LibRevNumber = (LIBVERSION_MAJOR * 100) + LIBVERSION_MINOR;
lechar LibErrorString[ERRORSTR_LENGTH] = {
		"libleiodc: \0",
};
lechar *StdErrorPtr;
lechar *ErrStringptr = LibErrorString;


const lechar *gpiorootdir = GPIO_ROOT_DIR;
const lechar *gpiodirpref = GPIO_ROOT_DIR GPIO_DIR_PREF;
const lechar *gpioexport = GPIO_ROOT_DIR GPIO_EXPORT;
const lechar *gpiodirection = GPIO_DIRECTION;
const lechar *gpiovalue = GPIO_VALUE;
//const lechar *gpioout = GPIO_OUT;
const lechar *gpioin = GPIO_IN;
const lechar *gpiohigh = GPIO_HIGH;
const lechar *gpiolow = GPIO_LOW;


// Error message constants
const lechar *slogErrdel = ": ";
const lechar *slogopenerr = "%s open() error";
const lechar *slogwriteerr = "%s write() error";
const lechar *slogcloseerr = "%s close() error";
const lechar *slogchmoderr = "%s chmod() error";
const lechar *slogstaterr = "%s stat() error";
const lechar *slogdirnotfound = "Error directory %s is not found";
const lechar *slogflnotfound = "Error file %s is not found";
const lechar *sloggpionotcreated = "Error gpio export failed, directory %s is not created";

const lechar *sloginvalidpin = "Processor gpio pad is not mapped for the requested lepin enum %u";
const lechar *sloglibold = "libleiodc.so library version %.2f is too old, version %.2f or newer is required.";
const lechar *sloginvaliduart = "UART number '%u' is to high, must be between 0...2";
const lechar *slogioctlerr = "UART ioctl(%u, 0x%x, rs485.flags=0x%x rs485.padding[0]=%u) error";


const pinmapTblStr pinmapTable[] = {
	{lepin_COM1_RS232, 		mx28pad_LCD_RD_E__GPIO_1_24},
	{lepin_COM1_RS422_RX1, 	mx28pad_LCD_WR_RWN__GPIO_1_25},
	{lepin_COM1_RS422_TX1, 	mx28pad_LCD_CS__GPIO_1_27},
	{lepin_COM1_RS422_RX2, 	mx28pad_LCD_VSYNC__GPIO_1_28},
	{lepin_COM1_RS422_TX2,	mx28pad_LCD_HSYNC__GPIO_1_29},

	{lepin_COM2_RS232, 		mx28pad_LCD_D07__GPIO_1_7},
	{lepin_COM2_RS422_RX1,	mx28pad_LCD_D08__GPIO_1_8},
	{lepin_COM2_RS422_TX1,	mx28pad_LCD_D09__GPIO_1_9},
	{lepin_COM2_RS422_RX2, 	mx28pad_LCD_D10__GPIO_1_10},
	{lepin_COM2_RS422_TX2, 	mx28pad_LCD_D11__GPIO_1_11},

	{lepin_COM3_RS232, 		mx28pad_LCD_D12__GPIO_1_12},
	{lepin_COM3_RS422_RX1,	mx28pad_LCD_D13__GPIO_1_13},
	{lepin_COM3_RS422_TX1,	mx28pad_LCD_D14__GPIO_1_14},
	{lepin_COM3_RS422_RX2,	mx28pad_LCD_D15__GPIO_1_15},
	{lepin_COM3_RS422_TX2,	mx28pad_LCD_D16__GPIO_1_16},

	{lepin_heartbeat, 		mx28pad_SSP3_MISO__GPIO_2_26},
	{lepin_modem_reset, 	mx28pad_SAIF1_SDATA0__GPIO_3_26},
	{lepin_modem_power, 	mx28pad_PWM4__GPIO_3_29},
};


// Errors with STD pointer
#define SYSLOG_STDERR _stderrappend(StdErrorPtr);





/***************************************************************************
* Library initialization constructor
* [03/07/2015]
***************************************************************************/
void LELIBCONSTRUCTOR leiodc_init(void) {

	ErrStringptr = LibErrorString + strlen(LibErrorString);
}


/***************************************************************************
* Initialize required pins
* return failure if initialization failed
* [11/03/2015]
* File permission setting added
* [09/07/2015]
***************************************************************************/
uint8_t leiodc_pininit(LIBARGDEF_INIT) {
	lechar					gpiopath[GPIO_PATH_LENGTH];
	uint8_t					dirconstlen;
	leiodcpin				cnt, startc = 0;
	mx28padenum				mxpad;
	fddef					tmpfd = 0;
	uint8_t					erroccurred = 0;


	if (access(gpiorootdir, X_OK) != 0) {		// Root directory must exist
		StdErrorPtr = strerror(errno);			// Capture error if any
		sprintf(ErrStringptr, slogdirnotfound, gpiorootdir);
		SYSLOG_STDERR
		return EXIT_FAILURE;
	}


	dirconstlen = strlen(gpiodirpref);
	strcpy(gpiopath, gpiodirpref);				// Individual pin directory name without a gpio number


	if (!pintable) startc = 1;					// Initialization starts from the first pin if table argument is not passed


	for (cnt = startc; cnt < pincount; cnt++) {
		if (pintable)							// If table argument is passed
			mxpad = _reslovepad(pintable[cnt]);	// Actual MX gpio number is calculated based on user table
		else									// Table argument is not passed
			mxpad = _reslovepad(cnt);			// Actual MX gpio number is calculated based on counter
		if (!mxpad) {
			if (pintable)						// If table argument is passed
				sprintf(ErrStringptr, sloginvalidpin, pintable[cnt]);
			else
				sprintf(ErrStringptr, sloginvalidpin, cnt);
			return EXIT_FAILURE;
		}


		sprintf(&gpiopath[dirconstlen], "%u", mxpad);	// Append gpio number to the directory string
		if (access(gpiopath, X_OK) != 0) {				// Check if gpio directory already exists
			if (_writegpiofile(&tmpfd, (lechar *) gpioexport, &gpiopath[dirconstlen], 0) == EXIT_FAILURE) {
				erroccurred = 1;
				goto stopnow;
			}
			if (access(gpiopath, X_OK) != 0) {			// Check if gpio directory created
				sprintf(ErrStringptr, sloggpionotcreated, gpiopath);
				erroccurred = 1;
				goto stopnow;
			}
		}
		sprintf(&gpiopath[dirconstlen], "%u%s", mxpad, gpiodirection);	// Set direction file permissions
		if (_setperms(gpiopath) == EXIT_FAILURE) {
			erroccurred = 1;
			goto stopnow;
		}
		sprintf(&gpiopath[dirconstlen], "%u%s", mxpad, gpiovalue);		// Set value file permissions
		if (_setperms(gpiopath) == EXIT_FAILURE) {
			erroccurred = 1;
			goto stopnow;
		}
	}


	stopnow:
	if (tmpfd) {
		return _close(&tmpfd, (lechar *) gpioexport, erroccurred);
	}
	if (erroccurred) return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
EXPORT_SYMBOL(leiodc_pininit)


/***************************************************************************
* Make pin output and set state
* return failure if failed
* [10/03/2015]
***************************************************************************/
uint8_t leiodc_pinoutstate(LIBARGDEF_PINS) {

	if (!state)
		return _gpioaction(lepin, gpiodirection, gpiolow);

	return _gpioaction(lepin, gpiodirection, gpiohigh);
}
EXPORT_SYMBOL(leiodc_pinoutstate)


/***************************************************************************
* Set state of the specified pin
* return failure if failed
* [06/03/2015]
***************************************************************************/
uint8_t leiodc_pinstate(LIBARGDEF_PINS) {
	uint16_t	asciistate = state | 0x30;


	return _gpioaction(lepin, gpiovalue, (const lechar *) &asciistate);
}
EXPORT_SYMBOL(leiodc_pinstate)


/***************************************************************************
* Make pin input
* return failure if failed
* [02/07/2015]
***************************************************************************/
uint8_t leiodc_pininput(LIBARGDEF_PINS) {

	return _gpioaction(lepin, gpiodirection, gpioin);
}
EXPORT_SYMBOL(leiodc_pininput)


/***************************************************************************
* Set interface mode of the UART
* return failure if failed
* [14/04/2015]
***************************************************************************/
uint8_t leiodc_uartint(LIBARGDEF_UART) {
	linux_serial_rs485	rs485kernel;


	if (uartno > 2) {
		//sprintf(ErrStringptr, sloginvaliduart, uartno);
		//return EXIT_FAILURE;
		return EXIT_SUCCESS;		// Don't do anything if UART number argument is greater than 2
	}
	if (leiodc_pininit(NULL, lepin_count) == EXIT_FAILURE) return EXIT_FAILURE;


	switch (interface) {
	case leuart_RS485def:
#ifdef DEBUG_RS485_AS_232
		rs485kernel.flags = (SER_RS485_ENABLED | SER_RS485_RTS_AFTER_SEND);
#else
		rs485kernel.flags = (SER_RS485_ENABLED | SER_RS485_RTS_ON_SEND);
#endif
		switch (uartno) {
		case 0:
#ifdef DEBUG_RS485_AS_232
			rs485kernel.padding[0] = 99;						// This is RTS pin
			if (_gpioaction(lepin_COM1_RS232, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_RX1, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
#else
			rs485kernel.padding[0] = mx28pad_LCD_CS__GPIO_1_27;	// This is RS485 OE pin
			if (_gpioaction(lepin_COM1_RS232, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_RX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
#endif
			if (_gpioaction(lepin_COM1_RS422_TX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_RX2, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_TX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;

			break;
		case 1:
#ifdef DEBUG_RS485_AS_232
			rs485kernel.padding[0] = 103;						// This is RTS pin
			if (_gpioaction(lepin_COM2_RS232, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_RX1, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
#else
			rs485kernel.padding[0] = mx28pad_LCD_D09__GPIO_1_9;	// This is RS485 OE pin
			if (_gpioaction(lepin_COM2_RS232, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_RX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
#endif
			if (_gpioaction(lepin_COM2_RS422_TX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_RX2, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_TX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			break;
		case 2:
#ifdef DEBUG_RS485_AS_232
			rs485kernel.padding[0] = 107;						// This is RTS pin
			if (_gpioaction(lepin_COM3_RS232, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_RX1, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
#else
			rs485kernel.padding[0] = mx28pad_LCD_D14__GPIO_1_14;	// This is RS485 OE pin
			if (_gpioaction(lepin_COM3_RS232, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_RX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
#endif
			if (_gpioaction(lepin_COM3_RS422_TX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_RX2, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_TX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			break;
		default:	// Impossible situation
			break;
		}

		if (fdptr) {
			if (ioctl(*fdptr, TIOCSRS485, &rs485kernel)) {
				StdErrorPtr = strerror(errno);
				sprintf(ErrStringptr, slogioctlerr, *fdptr, TIOCSRS485, rs485kernel.flags, rs485kernel.padding[0]);
				SYSLOG_STDERR
				return EXIT_FAILURE;
			}
		}
		break;


	case leuart_RS485rev:
		rs485kernel.flags = (SER_RS485_ENABLED | SER_RS485_RTS_ON_SEND);
		switch (uartno) {
		case 0:
			rs485kernel.padding[0] = mx28pad_LCD_HSYNC__GPIO_1_29;	// This is RS485 OE pin
			if (_gpioaction(lepin_COM1_RS232, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_RX1, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_TX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_RX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_TX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			break;
		case 1:
			rs485kernel.padding[0] = mx28pad_LCD_D11__GPIO_1_11;	// This is RS485 OE pin
			if (_gpioaction(lepin_COM2_RS232, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_RX1, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_TX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_RX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_TX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			break;
		case 2:
			rs485kernel.padding[0] = mx28pad_LCD_D16__GPIO_1_16;	// This is RS485 OE pin
			if (_gpioaction(lepin_COM3_RS232, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_RX1, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_TX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_RX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_TX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			break;
		default:	// Impossible situation
			break;
		}

		if (fdptr) {
			if (ioctl(*fdptr, TIOCSRS485, &rs485kernel)) {
				StdErrorPtr = strerror(errno);
				sprintf(ErrStringptr, slogioctlerr, *fdptr, TIOCSRS485, rs485kernel.flags, rs485kernel.padding[0]);
				SYSLOG_STDERR
				return EXIT_FAILURE;
			}
		}
		break;


	case leuart_RS422def:
		switch (uartno) {
		case 0:
			if (_gpioaction(lepin_COM1_RS232, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_RX1, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_TX1, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_RX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_TX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			break;
		case 1:
			if (_gpioaction(lepin_COM2_RS232, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_RX1, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_TX1, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_RX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_TX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			break;
		case 2:
			if (_gpioaction(lepin_COM3_RS232, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_RX1, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_TX1, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_RX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_TX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			break;
		default:	// Impossible situation
			break;
		}
		break;


	case leuart_RS422rev:
		switch (uartno) {
		case 0:
			if (_gpioaction(lepin_COM1_RS232, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_RX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_TX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_RX2, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_TX2, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			break;
		case 1:
			if (_gpioaction(lepin_COM2_RS232, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_RX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_TX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_RX2, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_TX2, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			break;
		case 2:
			if (_gpioaction(lepin_COM3_RS232, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_RX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_TX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_RX2, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_TX2, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			break;
		default:	// Impossible situation
			break;
		}
		break;


	case leuart_RS232:
	default:
		switch (uartno) {
		case 0:
			if (_gpioaction(lepin_COM1_RS232, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_RX1, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_TX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_RX2, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM1_RS422_TX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			break;
		case 1:
			if (_gpioaction(lepin_COM2_RS232, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_RX1, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_TX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_RX2, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM2_RS422_TX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			break;
		case 2:
			if (_gpioaction(lepin_COM3_RS232, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_RX1, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_TX1, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_RX2, gpiodirection, gpiohigh) == EXIT_FAILURE) return EXIT_FAILURE;
			if (_gpioaction(lepin_COM3_RS422_TX2, gpiodirection, gpiolow) == EXIT_FAILURE) return EXIT_FAILURE;
			break;
		default:	// Impossible situation
			break;
		}
		break;
	}
	return EXIT_SUCCESS;
}
EXPORT_SYMBOL(leiodc_uartint)


/***************************************************************************
* Check library version
* return failure if initialization failed
* [11/03/2015]
***************************************************************************/
uint8_t leiodc_libverchk(LIBARGDEF_VERCHK) {

	if (minvers > LibRevNumber) {
		float		reqver = minvers;
		float		currentver = LibRevNumber;
		reqver /= 100;
		currentver /= 100;
		sprintf(ErrStringptr, sloglibold, currentver, reqver);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
EXPORT_SYMBOL(leiodc_libverchk)




/***************************************************************************
* Perform an action on gpio
* return failure if failed
* [05/03/2015]
***************************************************************************/
uint8_t _gpioaction(leiodcpin lepin, const lechar *pathpostfix, const lechar *statestring) {
	lechar					gpiopath[GPIO_PATH_LENGTH];
	uint8_t					dirconstlen;
	mx28padenum				mxpad;
	fddef					tmpfd = 0;


	mxpad = _reslovepad(lepin);	// This is actual MX gpio number
	if (!mxpad) {
		sprintf(ErrStringptr, sloginvalidpin, lepin);
		return EXIT_FAILURE;
	}

	dirconstlen = strlen(gpiodirpref);
	strcpy(gpiopath, gpiodirpref);					// Individual pin directory name without a gpio number
	sprintf(&gpiopath[dirconstlen], "%u", mxpad);	// Append gpio number to the directory string
	strcat(gpiopath, pathpostfix);					// Append file name e.g. '/direction' or '/value'


	if (access(gpiopath, W_OK) != 0) {				// Check if file name is writable
		sprintf(ErrStringptr, slogflnotfound, gpiopath);
		return EXIT_FAILURE;
	}
	return _writegpiofile(&tmpfd, (lechar *) gpiopath, (lechar *) statestring, 1);
}


/***************************************************************************
* Write string to the specified file
* [05/03/2015]
***************************************************************************/
uint8_t _writegpiofile(fddef *filefd, lechar *filename, lechar *wrstring, uint8_t closefl) {
	uint8_t					erroccurred = 0;


	if (!(*filefd)) {
		*filefd = open(filename, O_WRONLY);
		StdErrorPtr = strerror(errno);			// Capture error if any
		if (*filefd < 1) {
			sprintf(ErrStringptr, slogopenerr, filename);
			SYSLOG_STDERR
			*filefd = 0;
			return EXIT_FAILURE;
		}
	}


	if (write(*filefd, wrstring, strlen(wrstring)) < 0) {
		StdErrorPtr = strerror(errno);			// Capture error if any
		sprintf(ErrStringptr, slogwriteerr, filename);
		SYSLOG_STDERR
		erroccurred = 1;
	}


	if (closefl) {
		return _close(filefd, filename, erroccurred);
	}

	if (erroccurred) return EXIT_FAILURE;
	return EXIT_SUCCESS;
}


/***************************************************************************
* close() function wrapper
* [05/03/2015]
***************************************************************************/
uint8_t _close(fddef *filefd, lechar *filename, uint8_t erralready) {

	if (close(*filefd)) {
		if (!erralready) {
			StdErrorPtr = strerror(errno);		// Capture error if any
			sprintf(ErrStringptr, slogcloseerr, filename);
			SYSLOG_STDERR
		}
		return EXIT_FAILURE;
	}
	*filefd = 0;
	if (erralready) return EXIT_FAILURE;
	return EXIT_SUCCESS;
}


/***************************************************************************
* Resolve MX28 pad number from the table
* [05/03/2015]
***************************************************************************/
mx28padenum _reslovepad(leiodcpinenum lepin) {
	leiodcpin	cnt;


	for (cnt = 0; cnt < (ARRAY_SIZE(pinmapTable)); cnt++) {
		if (pinmapTable[cnt].lepin == lepin)
			return pinmapTable[cnt].cpupad;
	}
	return 0;
}


/***************************************************************************
* Check and set file permissions
* [09/07/2015]
***************************************************************************/
uint8_t _setperms(lechar *filepath) {
	struct stat 	filestat;
	mode_t			fileperms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |  S_IROTH | S_IWOTH;


	if (stat(filepath, &filestat) < 0) {			// Check if file exists and read permissions
		StdErrorPtr = strerror(errno);				// Capture error if any
		sprintf(ErrStringptr, slogstaterr, filepath);
		SYSLOG_STDERR
		return EXIT_FAILURE;
	}
	else {
		if ((filestat.st_mode & fileperms) != fileperms) {	// Different permissions required
			if (chmod(filepath, fileperms) < 0) {			// Set new permissions
				StdErrorPtr = strerror(errno);		// Capture error if any
				sprintf(ErrStringptr, slogchmoderr, filepath);
				SYSLOG_STDERR
				return EXIT_FAILURE;
			}
		}
	}
	return EXIT_SUCCESS;
}


/***************************************************************************
* Log error messages
* [05/03/2015]
***************************************************************************/
void _stderrappend(lechar *errptr) {

	if (errptr) {
		strcat(ErrStringptr, slogErrdel);
		strcat(ErrStringptr, errptr);
	}
}
