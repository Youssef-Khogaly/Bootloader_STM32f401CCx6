/*
 ******************************************************************************
 * @file           : Bootloader_CFG.h
 * @author         : Youssef Ibrahem
 * @brief          : Bootloader_CFG.h
 ******************************************************************************
 */
#ifndef APPLICATION_BOOTLOADER_BOOTLOADER_CFG_H_
#define APPLICATION_BOOTLOADER_BOOTLOADER_CFG_H_

/*----------------------- Include Start ---------------------- */

/* ----------------------- Include END ----------------------- */

/* ----------------------- MACROS Start ---------------------- */
#define BL_DEBUG_UART			(&huart2)
#define BL_PRINT_BUFFER_SIZE	((uint8_t)64)
#define BL_PRINT_NEWLINE		((char*)"\r\n")

#define BL_ENABLE_UART_DEBUG_MSG 	(0x00U)
#define BL_ENABLE_SPI_DEBUG_MSG		(0x01U)
#define BL_ENABLE_CAN_DEBUG_MSG		(0x02U)

/**
 * options:
 * 			BL_ENABLE_UART_DEBUG_MSG
 * 			BL_ENABLE_SPI_DEBUG_MSG
 * 			BL_ENABLE_CAN_DEBUG_MSG
 * */
#define BL_DEBUG_METHOD	(BL_ENABLE_UART_DEBUG_MSG)

/**
 * 	comment it to stop printing debugging info
 * */
//#define BL_ENABLE_DEBUG


#define BL_HOST_BUFFER_RX_MAX_SIZE		((uint16_t)272UL)

/*
 * 		Address of @ref UART_HandleTypeDef
 * */
#define BL_HOST_COMMUNICATION_UART		(&(huart2))

/**/
#define BL_CRC_ENGINE_OBJ				(&(hcrc))

/* Comment it if you need to disable changing  Read protection  to level 2
 * Note:
 * 			If you changed read out protection level to level 2
 * 			YOU CANT NEVER GO BACK TO LEVEL 0 or LEVEL 1 !!!
 * */
//#define BL_ENABLE_ROP_LEVEL_2



/* ----------------------- MACROS END ------------------------ */

/* ----------------------- Macro Functions Start -------------- */


/* ----------------------- Macro Function End ----------------- */

/* ----------------------- User Data Types Start -------------- */

/* ----------------------- User Data Types End ---------------- */

/* ----------------------- Software Interfaces Start ---------- */

/* ----------------------- Software Interfaces end ------------ */

#endif /* APPLICATION_BOOTLOADER_BOOTLOADER_CFG_H_ */
