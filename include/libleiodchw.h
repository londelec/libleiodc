/*
 ============================================================================
 Name        : libleiodchw.h
 Author      : AK
 Version     : V1.01
 Copyright   : Property of Londelec UK Ltd
 Description : Header file for LEIODC processor pin manipulation library

  Change log  :

  *********V1.01 02/07/2015**************
  Set pin to input function created
  File permission setting function created

  *********V1.00 05/03/2015**************
  Initial revision

 ============================================================================
 */

#ifndef LIBLEIODCHW_H_
#define LIBLEIODCHW_H_


#include "ledefs.h"


// General constants
#define ERRORSTR_LENGTH				512				// Output string length
#define GPIO_PATH_LENGTH			256				// Length of the gpio directory name


#define LIBARGDEF_INIT	leiodcpin *pintable, uint8_t pincount
#define LIBARGDEF_PINS	leiodcpin lepin, uint8_t state
#define LIBARGDEF_UART	uint8_t uartno, uint8_t interface, fddef *fdptr
#define LIBARGDEF_VERCHK	uint16_t minvers


/* For every exported symbol, place a struct in the __ksymtab section */
#define __EXPORT_SYMBOL(sym, sec)				\
	extern typeof(sym) sym;
	/*static const char __kstrtab_##sym[]			\
	__attribute__((section("__ksymtab_strings"), aligned(1))) \
	= VMLINUX_SYMBOL_STR(sym);				\
	extern const struct kernel_symbol __ksymtab_##sym;	\
	__visible const struct kernel_symbol __ksymtab_##sym	\
	__used							\
	__attribute__((section("___ksymtab" sec "+" #sym), unused))	\
	= { (unsigned long)&sym, __kstrtab_##sym }*/


#define EXPORT_SYMBOL(sym)		__EXPORT_SYMBOL(sym, "")
//#define EXPORT_SYMBOL_GPL(sym)	__EXPORT_SYMBOL(sym, "_gpl")
//#define EXPORT_SYMBOL_GPL_FUTURE(sym)	__EXPORT_SYMBOL(sym, "_gpl_future")


// library variable type definitions
typedef	uint8_t						leiodcpin;				/* LEIODC pin size definition */


// Pin definitions used by library function callers
// Warning, don't remove existing enums as this will break
// binaries which call library with previous enums
typedef enum {
	lepin_COM1_RS232				= 1,				// COM1 RS232 enabled = 1
	lepin_COM1_RS422_RX1,								// COM1 1st RS485 (U11) RX enabled = 0
	lepin_COM1_RS422_TX1,								// COM1 1st RS485 (U11) TX enabled = 1
	lepin_COM1_RS422_RX2,								// COM1 2nd RS485 (U12) RX enabled = 0
	lepin_COM1_RS422_TX2,								// COM1 2nd RS485 (U12) TX enabled = 1

	lepin_COM2_RS232,									// COM2 RS232 enabled = 1
	lepin_COM2_RS422_RX1,								// COM2 1st RS485 (U14) RX enabled = 0
	lepin_COM2_RS422_TX1,								// COM2 1st RS485 (U14) TX enabled = 1
	lepin_COM2_RS422_RX2,								// COM2 2nd RS485 (U15) RX enabled = 0
	lepin_COM2_RS422_TX2,								// COM2 2nd RS485 (U15) TX enabled = 1

	lepin_COM3_RS232,									// COM3 RS232 enabled = 1
	lepin_COM3_RS422_RX1,								// COM3 1st RS485 (U17) RX enabled = 0
	lepin_COM3_RS422_TX1,								// COM3 1st RS485 (U17) TX enabled = 1
	lepin_COM3_RS422_RX2,								// COM3 2nd RS485 (U18) RX enabled = 0
	lepin_COM3_RS422_TX2,								// COM3 2nd RS485 (U18) TX enabled = 1

	lepin_heartbeat,									// Heartbeat
	lepin_modem_reset,									// MU609 reset (no longer used)
	lepin_modem_power,									// MU609 power
	lepin_count											// Number of defined pins, must be the last enum
} LEOPACK leiodcpinenum;


// MX28 pads
typedef enum {
	mx28pad_LCD_RD_E__GPIO_1_24		= 56,				// COM1 RS232 enabled = 1
	mx28pad_LCD_WR_RWN__GPIO_1_25	= 57,				// COM1 1st RS485 (U11) RX enabled = 0
	mx28pad_LCD_CS__GPIO_1_27		= 59,				// COM1 1st RS485 (U11) TX enabled = 1
	mx28pad_LCD_VSYNC__GPIO_1_28	= 60,				// COM1 2nd RS485 (U12) RX enabled = 0
	mx28pad_LCD_HSYNC__GPIO_1_29	= 61,				// COM1 2nd RS485 (U12) TX enabled = 1

	mx28pad_LCD_D07__GPIO_1_7		= 39,				// COM2 RS232 enabled = 1
	mx28pad_LCD_D08__GPIO_1_8		= 40,				// COM2 1st RS485 (U14) RX enabled = 0
	mx28pad_LCD_D09__GPIO_1_9		= 41,				// COM2 1st RS485 (U14) TX enabled = 1
	mx28pad_LCD_D10__GPIO_1_10		= 42,				// COM2 2nd RS485 (U15) RX enabled = 0
	mx28pad_LCD_D11__GPIO_1_11		= 43,				// COM2 2nd RS485 (U15) TX enabled = 1

	mx28pad_LCD_D12__GPIO_1_12		= 44,				// COM3 RS232 enabled = 1
	mx28pad_LCD_D13__GPIO_1_13		= 45,				// COM3 1st RS485 (U17) RX enabled = 0
	mx28pad_LCD_D14__GPIO_1_14		= 46,				// COM3 1st RS485 (U17) TX enabled = 1
	mx28pad_LCD_D15__GPIO_1_15		= 47,				// COM3 2nd RS485 (U18) RX enabled = 0
	mx28pad_LCD_D16__GPIO_1_16		= 48,				// COM3 2nd RS485 (U18) TX enabled = 1

	mx28pad_SSP3_MISO__GPIO_2_26	= 90,				// Heartbeat
	mx28pad_SAIF1_SDATA0__GPIO_3_26	= 122,				// MU609 reset (no longer used)
	mx28pad_PWM4__GPIO_3_29			= 125,				// MU609 power
} LEOPACK mx28padenum;


// UART interface mode definitions used by library function callers
typedef enum {
	leuart_RS232					= 1,				// RS232 interface
	leuart_RS485def,									// RS485 interface default pinout (DSUB pins 1-2 used)
	leuart_RS485rev,									// RS485 interface reversed pinout (DSUB pins 3-4 used)
	leuart_RS422def,									// RS422 interface default pinout (Tx 1-2; Rx 3-4)
	leuart_RS422rev,									// RS422 interface reversed pinout (Rx 1-2; Tx 3-4)
} LEOPACK leiodcuartintenum;




// Pin map structure
typedef struct pinmapTblStr_ {
	leiodcpinenum			lepin;
	mx28padenum				cpupad;
} pinmapTblStr;


// Structure from linux kernel v3.19
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


//extern const lechar *libleiodcVersion;
extern lechar LibErrorString[];


// Exported functions
extern uint8_t leiodc_pininit(LIBARGDEF_INIT);
extern uint8_t leiodc_pinoutstate(LIBARGDEF_PINS);
extern uint8_t leiodc_pinstate(LIBARGDEF_PINS);
extern uint8_t leiodc_pininput(LIBARGDEF_PINS);
uint8_t leiodc_uartint(LIBARGDEF_UART);
uint8_t leiodc_libverchk(LIBARGDEF_VERCHK);


// Internal functions
uint8_t _gpioaction(leiodcpin lepin, const lechar *pathpostfix, const lechar *statestring);
uint8_t _writegpiofile(fddef *filefd, lechar *filename, lechar *wrstring, uint8_t closefl);
uint8_t _close(fddef *filefd, lechar *filename, uint8_t erralready);
mx28padenum _reslovepad(leiodcpinenum lepin);
uint8_t _setperms(lechar *filepath);
void _stderrappend(lechar *errptr);


#endif /* LIBLEIODCHW_H_ */
