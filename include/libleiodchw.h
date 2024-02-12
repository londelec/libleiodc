/*
 ============================================================================
 Name        : libleiodchw.h
 Author      : AK
 Version     : V3.00
 Copyright   : Property of Londelec UK Ltd
 Description : Header file for LEIODC CPU pin manipulation library

  Change log :

  *********V3.00 26/01/2024**************
  M.2 card config pins defined, new API to read card config as a byte
  MB board version pins defined, new API to read Board version as a byte
  All API functions renamed

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
	lepin_modem_reset,			/* M.2 reset (no longer used for MU609 reset) */
	lepin_modem_power,			/* M.2/MU609 power */
	lepin_M2_cfg0,				/* M.2 CONFIG0 */
	lepin_M2_cfg1,				/* M.2 CONFIG1 */
	lepin_M2_cfg2,				/* M.2 CONFIG2 */
	lepin_M2_cfg3,				/* M.2 CONFIG3 */

	lepin_board_ver0,			/* Board version[0] */
	lepin_board_ver1,			/* Board version[1] */
	lepin_board_ver2,			/* Board version[2] */
	lepin_board_ver3,			/* Board version[3] */
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
extern int leiodc_pin_init(LIBARGDEF_INIT);
extern int leiodc_pin_dir_out_state_set(LIBARGDEF_PINS);
extern int leiodc_pin_state_set(LIBARGDEF_PINS);
extern int leiodc_pin_state_get(LIBARGDEF_PINS);
extern int leiodc_pin_dir_in_set(LIBARGDEF_PINS);
extern int leiodc_uart_int(LIBARGDEF_UART);
extern int leiodc_m2_init(void);
extern int leiodc_m2_config_get(void);
extern int leiodc_board_ver_get(void);
extern int leiodc_libverchk(LIBARGDEF_VERCHK);
#endif /* LIBLEIODCHW_H_ */
