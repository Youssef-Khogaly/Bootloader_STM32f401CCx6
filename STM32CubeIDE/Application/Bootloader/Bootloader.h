/*
 ******************************************************************************
 * @file           : Bootloader.h
 * @author         : Youssef Ibrahem
 * @brief          : Bootloader.h
 ******************************************************************************
 */
#ifndef APPLICATION_BOOTLOADER_BOOTLOADER_H_
#define APPLICATION_BOOTLOADER_BOOTLOADER_H_

/*----------------------- Include Start ---------------------- */
#include "usart.h"
#include "crc.h"
#include "Bootloader_CFG.h"
#include "stdio.h"
#include <strings.h>
#include <string.h>
#include <stdarg.h>
/* ----------------------- Include END ----------------------- */

/* ----------------------- MACROS Start ---------------------- */

/* ----------------------- MACROS END ------------------------ */

/* ----------------------- Macro Functions Start -------------- */


/* ----------------------- Macro Function End ----------------- */

/* ----------------------- User Data Types Start -------------- */
typedef enum {
	BL_ACK = 0U,
	BL_NACK

}BL_Stat_t;

/* ----------------------- User Data Types End ---------------- */

/* ----------------------- Software Interfaces Start ---------- */

BL_Stat_t BL_UART_Featch_Host_Command(void);
/* ----------------------- Software Interfaces end ------------ */

#endif /* APPLICATION_BOOTLOADER_BOOTLOADER_H_ */
