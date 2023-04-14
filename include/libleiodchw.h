/*
 ============================================================================
 Name        : libleiodchw.h
 Author      : AK
 Version     : V2.00
 Copyright   : Property of Londelec UK Ltd
 Description : Header file for LEIODC processor pin manipulation library

  Change log :

  *********V2.00 31/10/2022**************
  Compatible with new GPIO interface in linux, now using /dev/gpio

  *********V1.02 03/04/2016**************
  struct serial_rs485 definition disabled if <linux/serial.h> is included

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


/*
 * Exported function arguments
 */
#define LIBARGDEF_INIT const leiodcpin pintable[], uint8_t pincount
#define LIBARGDEF_PINS leiodcpin lepin, uint8_t state
#define LIBARGDEF_UART uint8_t uartno, uint8_t interface, const fddef *fdptr
#define LIBARGDEF_VERCHK uint16_t minvers

typedef	uint8_t		leiodcpin;				/* LEIODC pin size definition */

/*
 * Pin definitions are used by library callers.
 * Warning, don't remove existing enums as this will break
 * binaries which call the library with previous enums.
 */
typedef enum {
	lepin_COM1_RS232 = 1,		/* COM1 RS232 enabled = 1 */
	lepin_COM1_RS422_RX1,		/* COM1 1st RS485_P12 RX enabled = 0 */
	lepin_COM1_RS422_TX1,		/* COM1 1st RS485_P12 TX enabled = 1 */
	lepin_COM1_RS422_RX2,		/* COM1 2nd RS485_P34 RX enabled = 0 */
	lepin_COM1_RS422_TX2,		/* COM1 2nd RS485_P34 TX enabled = 1 */

	lepin_COM2_RS232,			/* COM2 RS232 enabled = 1 */
	lepin_COM2_RS422_RX1,		/* COM2 1st RS485_P12 RX enabled = 0 */
	lepin_COM2_RS422_TX1,		/* COM2 1st RS485_P12 TX enabled = 1 */
	lepin_COM2_RS422_RX2,		/* COM2 2nd RS485_P34 RX enabled = 0 */
	lepin_COM2_RS422_TX2,		/* COM2 2nd RS485_P34 TX enabled = 1 */

	lepin_COM3_RS232,			/* COM3 RS232 enabled = 1 */
	lepin_COM3_RS422_RX1,		/* COM3 1st RS485_P12 RX enabled = 0 */
	lepin_COM3_RS422_TX1,		/* COM3 1st RS485_P12 TX enabled = 1 */
	lepin_COM3_RS422_RX2,		/* COM3 2nd RS485_P34 RX enabled = 0 */
	lepin_COM3_RS422_TX2,		/* COM3 2nd RS485_P34 TX enabled = 1 */

	lepin_heartbeat,			/* Heartbeat LED */
	lepin_modem_reset,			/* MU609 reset (no longer used) */
	lepin_modem_power,			/* MU609 power */
	lepin_count					/* Number of defined pins, must be the last */
} leiodcpin_e;

/*
 * UART interface mode definitions used by library function callers
 */
typedef enum {
	leuart_RS232 = 1,			/* RS232 interface */
	leuart_RS485def,			/* RS485 interface default pinout (DSUB pins 1-2 used) */
	leuart_RS485rev,			/* RS485 interface reversed pinout (DSUB pins 3-4 used) */
	leuart_RS422def,			/* RS422 interface default pinout (Tx 1-2; Rx 3-4) */
	leuart_RS422rev,			/* RS422 interface reversed pinout (Rx 1-2; Tx 3-4) */
} leiodcuartint_e;


extern lechar LibErrorString[];

/*
 * Exported functions
 */
extern int leiodc_pininit(LIBARGDEF_INIT);
extern int leiodc_pinoutstate(LIBARGDEF_PINS);
extern int leiodc_pinstate(LIBARGDEF_PINS);
extern int leiodc_pininput(LIBARGDEF_PINS);
extern int leiodc_uartint(LIBARGDEF_UART);
extern int leiodc_libverchk(LIBARGDEF_VERCHK);
#endif /* LIBLEIODCHW_H_ */
