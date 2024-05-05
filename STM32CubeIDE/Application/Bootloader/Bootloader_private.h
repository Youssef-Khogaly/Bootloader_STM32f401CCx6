/*
 ******************************************************************************
 * @file           : Bootloader_private.h
 * @author         : Youssef Ibrahem
 * @brief          : Bootloader_private.h
 ******************************************************************************
 */
#ifndef APPLICATION_BOOTLOADER_BOOTLOADER_PRIVATE_H_
#define APPLICATION_BOOTLOADER_BOOTLOADER_PRIVATE_H_

/*----------------------- Include Start ---------------------- */

/* ----------------------- Include END ----------------------- */


/* ----------------------- MACROS Start ---------------------- */
#define BL_NUMBER_OF_COMMAND		(8U)
	/* 			BL Commands  start 		*/
/*command is used to  read bootloader version*/
#define CBL_GET_VER_CMD					(0x10U)

/* Command is used to know what commands are supported by bootloader*/
#define CBL_GET_HELP_CMD				(0x11U)

/*Command is used to read  MCU chip identifier number*/
#define CBL_GET_CID_CMD					(0x12U)

/*Command is used to read  the flash Read Protection level*/
#define CBL_GET_RDP_STATUS_CMD			(0x13U)

/*Command is used to jump  bootloader to sepcific address*/
#define CBL_GO_TO_ADDR_CMD				(0x14U)

 /*Command is used  this command use to  mass erase or sector erase of the user flash*/
#define CBL_FLASH_ERASE_CMD				(0x15U)

/*Command is used to write data   to different  memories of mcu*/
#define CBL_MEM_WRITE_CMD				(0x16U)

/* Change Read Out Protection Level */
#define CBL_CHANGE_ROP_Level_CMD        (0x17U)

		/*       BL Commands end */
#define CBL_VENDOR_ID			(100U)
#define CBL_SW_MAJOR_VERSION	(1U)
#define CBL_SW_MINOR_PATCH		(0U)
#define CBL_SW_PATCH_VERSION	(0U)

#define CRC_TYPE_SIZE			(4U)
#define CRC_VERIFICATION_FAILED		(0U)
#define CRC_VERIFICATION_PASSED		(1U)


#define CBL_SEND_ACK	(0xCDU)
#define CBL_SEND_NACK	(0xABU)


#define CBL_ACK_REPLY_MSG_LENGTH  		(0x02UL)

#define FLASH_SECTOR2_BASE_ADDRESS		(0x8008000UL)

#define ADDRESS_IS_VALID			(0x01UL)
#define ADDRESS_IS_INVALID			(0x00UL)


#define BL_STM32F401_SRAM_SIZE		(64UL * 1024UL)
#define BL_STM32F401_FLASH_SIZE		(256UL * 1024UL)

#define BL_STM32401_FLASH_END		(FLASH_BASE + BL_STM32F401_FLASH_SIZE)
#define BL_STM32F401_SRAM_END		(SRAM1_BASE + BL_STM32F401_SRAM_SIZE)

#define BL_STM32401_MAX_FLASH_SECTORS	(0x6UL)

#define BL_FLASH_MASS_ERASE					(0xffU)

#define BL_INVALID_SECTOR_NUMBER			(0x00U)
#define BL_VALID_SECTOR_NUMBER				(0x01U)
#define BL_UNSUCCESSFUL_ERASE				(0x02U)
#define BL_SUCCESSFUL_ERASE					(0x03U)

#define BL_HAL_SUCCESSFUL_ERASE				(0xFFFFFFFFUL)

#define BL_FLASH_WRITE_FAILED				(0x00U)
#define BL_FLASH_WRITE_PASSED				(0x01U)


#define BL_ROP_LEVEL_0							(0U)
#define BL_ROP_LEVEL_1							(1U)
#define BL_ROP_LEVEL_2							(2U)

#define ROP_LEVEL_CHANGE_INVALID			(0x00U)
#define ROP_LEVEL_CHANGE_VALID				(0x01U)
/* ----------------------- MACROS END ------------------------ */

/* ----------------------- Macro Functions Start -------------- */
#define BL_COMMAND_TO_ARR_IDX(_COMMAND)	((uint8_t)((_COMMAND) - 0x10U))

#define IS_BL_COMMAND(_COMMAND)			((CBL_GET_VER_CMD <=  (_COMMAND)) && (CBL_CHANGE_ROP_Level_CMD >=  (_COMMAND)))

/* ----------------------- Macro Function End ----------------- */

/* ----------------------- User Data Types Start -------------- */
typedef void(*pMainApp)(void);
typedef void(*BL_HelperCommandpFunc)(void);
typedef void(*pJumpAddressFunc)(void);
/* ----------------------- User Data Types End ---------------- */

/* ----------------------- Software Interfaces Start ---------- */

/* ----------------------- Software Interfaces end ------------ */

#endif /* APPLICATION_BOOTLOADER_BOOTLOADER_PRIVATE_H_ */
