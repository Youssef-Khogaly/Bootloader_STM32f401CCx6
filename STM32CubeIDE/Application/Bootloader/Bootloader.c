/*
 ******************************************************************************
 * @file           : Bootloader.c
 * @author         : Youssef Ibrahem
 * @brief          : Bootloader.c
 ******************************************************************************
 */
#include "Bootloader.h"
#include "../../Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_crc.h"
#include "../../Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_flash_ex.h"
#include "../../Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_flash.h"
#include "Bootloader_private.h"


static void Bootloader_Jump_to_user_main(void);
static void Bootloader_Get_Version(void);
static void Bootloader_Get_Help(void);
static void Bootloader_Get_Chip_ID(void);
static void Bootloader_Read_Protection_Level(void);
static void Bootloader_Jump_to_Address(void);
static void Bootloader_Erase_Flash(void);
static void Bootloader_Memory_Write(void);
static uint8_t Bootloader_CRC_Verifiy(uint32_t dataLen ,uint32_t HostCRC);
static void Bootloader_SendAck(uint8_t ReplyMessageLength);
static void Bootloader_SendNAck(void);
static void BootLoader_SendData(uint8_t* pData , uint32_t DataLen);
static uint8_t Bootloader_Host_Jump_Address_verification(uint32_t JumpAdress);
static uint8_t Perfrom_Flash_Erase(uint8_t SectorNum , uint8_t NumberOfSectors);
static uint8_t Perfrom_Memory_Write(uint8_t* pDataBuffer , uint8_t DataLen , uint32_t StartMemAddress);
static uint8_t BL_Read_Flash_Protection_Level(void);
static void Bootloader_ChangeReadProtection(void);
static uint8_t BL_Change_ROP_Level(uint8_t RDP_Level);
static BL_Stat_t BL_PrintMsg(const char* format , ... );

/* Array of pointer to helper functions of bootloader commands*/
static BL_HelperCommandpFunc BL_HelperFunc[BL_NUMBER_OF_COMMAND] = {
		Bootloader_Get_Version,
		Bootloader_Get_Help,
		Bootloader_Get_Chip_ID,
		Bootloader_Read_Protection_Level,
		Bootloader_Jump_to_Address,
		Bootloader_Erase_Flash,
		Bootloader_Memory_Write,
		Bootloader_ChangeReadProtection
};
/*****************************************/


/* private Global Variable*/
static uint8_t BL_HOST_BUFFER[BL_HOST_BUFFER_RX_MAX_SIZE];
static uint8_t BL_Commands[8U] = {CBL_GET_VER_CMD,CBL_GET_HELP_CMD,CBL_GET_CID_CMD,CBL_GET_RDP_STATUS_CMD,CBL_GO_TO_ADDR_CMD,CBL_FLASH_ERASE_CMD,CBL_MEM_WRITE_CMD,CBL_CHANGE_ROP_Level_CMD};
/* ----------------------- Software Interfaces Start ---------- */


BL_Stat_t BL_UART_Featch_Host_Command(void)
{
	BL_Stat_t RetStat = BL_ACK;
	HAL_StatusTypeDef UART_Stat = HAL_OK;
	uint16_t DataLen = 0;
	/* Clear Rx Buffer */
	memset(BL_HOST_BUFFER,(uint8_t)0U , BL_HOST_BUFFER_RX_MAX_SIZE);
	/*Receive first byte aka number of byte to be received from host
	 * Min data len = 5 : 1byte command + 4 byte CRC
	 * Max data len = 5 + N : 1 byte command + N byte details + 4 byte CRC
	 * */
#ifdef  BL_ENABLE_DEBUG
	BL_PrintMsg("Send length of Data %s" , BL_PRINT_NEWLINE);
#endif
	UART_Stat |= HAL_UART_Receive(BL_DEBUG_UART, BL_HOST_BUFFER, (uint16_t)1U, HAL_MAX_DELAY);
	DataLen = BL_HOST_BUFFER[0];

#ifdef  BL_ENABLE_DEBUG
	BL_PrintMsg("data len = : %i , Send command %s" ,BL_HOST_BUFFER[0] ,BL_PRINT_NEWLINE);
#endif
	UART_Stat |= HAL_UART_Receive(BL_DEBUG_UART, BL_HOST_BUFFER+1UL , DataLen, HAL_MAX_DELAY);
	if(UART_Stat == HAL_OK)
	{
		if(IS_BL_COMMAND(BL_HOST_BUFFER[1U]))
		{
			BL_HelperFunc[BL_COMMAND_TO_ARR_IDX(BL_HOST_BUFFER[1U])]();
		}
		else
		{
			RetStat = BL_NACK;
		}
	}
#ifdef  BL_ENABLE_DEBUG
	BL_PrintMsg("BL_UART_Featch_Host_Command: return status -> %i %s" ,RetStat,BL_PRINT_NEWLINE);
#endif
	return RetStat;
}



/*Static private functions Declarations*/
BL_Stat_t BL_PrintMsg(const char* format , ... )
{
	BL_Stat_t BlStatRet = BL_ACK;
	HAL_StatusTypeDef HalStat = HAL_OK;
	uint32_t msg_len = 0U;
	if(strlen(format)+1UL <= BL_PRINT_BUFFER_SIZE )
	{
		/* create buffer for sending msg*/
		char message[BL_PRINT_BUFFER_SIZE] = {0U};
		/*create a list of args*/
		va_list args;
		/*This enables access to variadic function arguments.*/
		va_start(args , format);
		/*sends formatted output to a buffer using an argument list passed to it.*/
		vsprintf(message , format , args);
		/*lenth of the msg need to send it */
		msg_len = strlen(message)+1U;
		BootLoader_SendData((uint8_t*)message , msg_len);
		va_end(args);

		BlStatRet = (HalStat == HAL_OK) ? BL_ACK : BL_NACK;
	}
	else
	{
		BlStatRet = BL_NACK;
	}
	return BlStatRet;
}


static uint8_t Bootloader_CRC_Verifiy(uint32_t dataLen ,uint32_t HostCRC)
{
	uint8_t crcStat = CRC_VERIFICATION_FAILED;
	uint32_t MCU_CRC_Calculated = 0;
	uint8_t DataCounter = 0;
	uint32_t DataBuffer = 0;
	/*Calculate my CRC*/
	for( ; DataCounter < dataLen ; ++DataCounter)
	{
		DataBuffer = (uint32_t)BL_HOST_BUFFER[DataCounter];
		MCU_CRC_Calculated = HAL_CRC_Accumulate(BL_CRC_ENGINE_OBJ , &DataBuffer , (uint32_t)1);
	}

	/*Reset CRC data REG*/

	__HAL_CRC_DR_RESET(BL_CRC_ENGINE_OBJ);
	if(HostCRC == MCU_CRC_Calculated)
	{
		crcStat = CRC_VERIFICATION_PASSED;
	}
	else {}

	return crcStat;
}


static void Bootloader_SendAck(uint8_t ReplyMessageLength)
{
	uint8_t Ack_Value[CBL_ACK_REPLY_MSG_LENGTH] = {CBL_SEND_ACK , ReplyMessageLength};
	BootLoader_SendData(Ack_Value , CBL_ACK_REPLY_MSG_LENGTH);
}


static void Bootloader_SendNAck(void)
{
	uint8_t Ack_Value[CBL_ACK_REPLY_MSG_LENGTH] = {CBL_SEND_NACK, } ;
	BootLoader_SendData(Ack_Value , CBL_ACK_REPLY_MSG_LENGTH);
}


static void Bootloader_Get_Version(void)
{
	uint8_t BL_Version[4U] = {CBL_VENDOR_ID , CBL_SW_MAJOR_VERSION , CBL_SW_MINOR_PATCH ,CBL_SW_PATCH_VERSION};
	uint16_t Host_PacketLen = BL_HOST_BUFFER[0U] + 1U;
	uint32_t Host_CRC32 = 0UL;

	/*extract CRC from buffer */
	Host_CRC32 = *((uint32_t*)(BL_HOST_BUFFER + (Host_PacketLen - CRC_TYPE_SIZE)));
	/*Calcualte my crc and verification of crc */
	if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verifiy((uint32_t)(Host_PacketLen-CRC_TYPE_SIZE) , Host_CRC32) )
	{
#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("CRC Verification Passed %s" , BL_PRINT_NEWLINE);
#endif
		/*Send Ack +  Reply message length*/
		Bootloader_SendAck((uint8_t)4);
		/*Send message with version info*/
		BootLoader_SendData(BL_Version , 4U);
	}
	else
	{
#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("CRC Verification Failed %s" , BL_PRINT_NEWLINE);
#endif
		/*Send NACK */
		Bootloader_SendNAck();
	}

}
static void Bootloader_Get_Help(void)
{
	uint16_t Host_PacketLen = BL_HOST_BUFFER[0U] + 1U;
	uint32_t Host_CRC32 = 0UL;

	/*extract CRC from buffer */
	Host_CRC32 = *((uint32_t*)(BL_HOST_BUFFER + (Host_PacketLen - CRC_TYPE_SIZE)));

	/*Calcualte my crc and verify  crc */
	if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verifiy((uint32_t)(Host_PacketLen-CRC_TYPE_SIZE) , Host_CRC32) )
	{
#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("CRC Verification Passed %s" , BL_PRINT_NEWLINE);
#endif
		/*Send Ack +  Reply message length*/
		Bootloader_SendAck((uint8_t)BL_NUMBER_OF_COMMAND);
		/*Send BL commands*/
		BootLoader_SendData(BL_Commands , BL_NUMBER_OF_COMMAND);
	}
	else
	{
#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("CRC Verification Failed %s" , BL_PRINT_NEWLINE);
#endif
		/*Send NACK */
		Bootloader_SendNAck();
	}
}
static void Bootloader_Get_Chip_ID(void)
{
	uint16_t Host_PacketLen = BL_HOST_BUFFER[0U] + 1U;
	uint32_t Host_CRC32 = 0UL;
	uint16_t MCU_ID = 0 ;
	/*extract CRC from buffer */
	Host_CRC32 = *((uint32_t*)(BL_HOST_BUFFER + (Host_PacketLen - CRC_TYPE_SIZE)));

	/*Calcualte my crc and verify  crc */
	if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verifiy((uint32_t)(Host_PacketLen-CRC_TYPE_SIZE) , Host_CRC32) )
	{
#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("CRC Verification Passed %s" , BL_PRINT_NEWLINE);
#endif
		/* Get MCU ID */
		MCU_ID = (uint16_t)((DBGMCU->IDCODE) & ((uint32_t)0x0007FFUL));
		/*Send Ack +  Reply message length*/
		Bootloader_SendAck((uint8_t)2U);
		/*Send MCU ID*/
		BootLoader_SendData( (uint8_t*)(&MCU_ID), (uint32_t)2UL);
	}
	else
	{
#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("CRC Verification Failed %s" , BL_PRINT_NEWLINE);
#endif
		/*Send NACK */
		Bootloader_SendNAck();
	}
}
static void Bootloader_Read_Protection_Level(void)
{
	uint16_t Host_PacketLen = BL_HOST_BUFFER[0U] + 1U;
	uint32_t Host_CRC32 = 0UL;
	uint8_t RDP_level = 0;
	/*extract CRC from buffer */
	Host_CRC32 = *((uint32_t*)(BL_HOST_BUFFER + (Host_PacketLen - CRC_TYPE_SIZE)));

	/*Calcualte my crc and verify  crc */
	if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verifiy((uint32_t)(Host_PacketLen-CRC_TYPE_SIZE) , Host_CRC32) )
	{
#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("CRC Verification Passed %s" , BL_PRINT_NEWLINE);
#endif
		/*	Send Ack +  Reply message length	*/
		Bootloader_SendAck((uint8_t)1U);
		/*	Read Protection level */
		RDP_level = BL_Read_Flash_Protection_Level();
		/* Report Read Protection level  */
		BootLoader_SendData((uint8_t*)(&RDP_level), (uint32_t)1UL);
#ifdef  BL_ENABLE_DEBUG
		BL_PrintMsg("Flash Read Protection level -> %i %s" ,RDP_level , BL_PRINT_NEWLINE);
#endif

	}
	else
	{
#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("CRC Verification Failed %s" , BL_PRINT_NEWLINE);
#endif
		/*Send NACK */
		Bootloader_SendNAck();
	}
}
static void Bootloader_Jump_to_Address(void)
{
		uint16_t Host_PacketLen = BL_HOST_BUFFER[0U] + 1U;
		uint32_t Host_CRC32 = 0UL;
		pJumpAddressFunc pJumpAddress = NULL;
		uint32_t HostJumpAdress = 0;
		uint8_t Address_Verification = ADDRESS_IS_INVALID;
		/*extract CRC from buffer */
		Host_CRC32 = *((uint32_t*)(BL_HOST_BUFFER + (Host_PacketLen - CRC_TYPE_SIZE)));
		/*Calcualte my crc and verify  crc */
		if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verifiy((uint32_t)(Host_PacketLen-CRC_TYPE_SIZE) , Host_CRC32) )
		{
	#ifdef  BL_ENABLE_DEBUG
				BL_PrintMsg("CRC Verification Passed %s" , BL_PRINT_NEWLINE);
	#endif
			/*Send Ack +  Reply message length*/
			Bootloader_SendAck((uint8_t)1U);
			/* Parse  Address from host buffer */
			HostJumpAdress = *((uint32_t*)(&BL_HOST_BUFFER[2U]));

				/* Address Verification */
			Address_Verification = Bootloader_Host_Jump_Address_verification(HostJumpAdress);
			if ( ADDRESS_IS_VALID == Address_Verification )
			{
#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("Jump Address is Valid %s" , BL_PRINT_NEWLINE);
#endif
				/* Access address using pointer */
				pJumpAddress = (pJumpAddressFunc)(HostJumpAdress | 0x01UL); /* Make sure that bit 0 in the address is 0(T-BIT) */
#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("Jump to : 0x%X  %s" , pJumpAddress , BL_PRINT_NEWLINE);
#endif
			BootLoader_SendData( (uint8_t*)(&Address_Verification), (uint32_t)1UL);
				pJumpAddress();
			}
			else
			{
		#ifdef  BL_ENABLE_DEBUG
					BL_PrintMsg("Jump Address is invalid %s" , BL_PRINT_NEWLINE);
		#endif
				/*Send NACK */
				BootLoader_SendData( (uint8_t*)(&Address_Verification), (uint32_t)1UL);
			}

		}
		else
		{
	#ifdef  BL_ENABLE_DEBUG
				BL_PrintMsg("CRC Verification Failed %s" , BL_PRINT_NEWLINE);
	#endif
			/*Send NACK */
			Bootloader_SendNAck();
		}
}
__STATIC_FORCEINLINE uint8_t Bootloader_Host_Jump_Address_verification(uint32_t JumpAdress)
{
	if( (SRAM1_BASE  <=  JumpAdress)  && (BL_STM32F401_SRAM_END  >=  JumpAdress) )
	{
		return ADDRESS_IS_VALID;
	}
	else if ( (FLASH_BASE  <=  JumpAdress)  && (BL_STM32401_FLASH_END	  >=  JumpAdress) )
	{
		return ADDRESS_IS_VALID;
	}
	else
	{ /*nothing */}
	return ADDRESS_IS_INVALID;
}

static uint8_t Perfrom_Flash_Erase(uint8_t SectorNum , uint8_t NumberOfSectors)
{
	uint8_t FlashEraseStat = BL_INVALID_SECTOR_NUMBER;
	uint32_t HAL_FLASH_STAT = 0;
	uint8_t remainingSectors = BL_STM32401_MAX_FLASH_SECTORS - SectorNum;
	HAL_StatusTypeDef HAL_STAT = HAL_OK;
	FLASH_EraseInitTypeDef  EraseInit = {
			.Banks = FLASH_BANK_1,
			.Sector = (uint32_t)SectorNum,
			.VoltageRange = FLASH_VOLTAGE_RANGE_3
	};
	/*Check if start sector number and number of sectors are valid */
	if(((BL_STM32401_MAX_FLASH_SECTORS-1U) >= SectorNum || BL_FLASH_MASS_ERASE == SectorNum) )
	{
		/* Check if SectorNumber  + number of sectors to be erased is valid if not valid erase from start sector to last  sector */
		if(NumberOfSectors >  remainingSectors)
		{
			EraseInit.NbSectors = (uint32_t)remainingSectors;
		}
		else
		{
			EraseInit.NbSectors = (uint32_t)NumberOfSectors;
		}
		/*	Erase Flash sector or mass erase */
		if(BL_FLASH_MASS_ERASE == SectorNum)
		{
			/*Mass erase*/
			EraseInit.TypeErase = FLASH_TYPEERASE_MASSERASE;
		}
		else
		{
			/* Sector erase */
			EraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
		}
#ifdef  BL_ENABLE_DEBUG
		BL_PrintMsg("Start Erasing Flash %s" , BL_PRINT_NEWLINE);
#endif
		/*UnLock Flash*/
		HAL_STAT |= HAL_FLASH_Unlock();
		/*	Start flash erase */
		HAL_STAT |= HAL_FLASHEx_Erase(&EraseInit , &HAL_FLASH_STAT);

		/* Check if Flash Erase Success */
		if(BL_HAL_SUCCESSFUL_ERASE == HAL_FLASH_STAT && HAL_OK == HAL_STAT)
		{
			FlashEraseStat = BL_SUCCESSFUL_ERASE;
		}
		else
		{
			FlashEraseStat = BL_UNSUCCESSFUL_ERASE;
		}
		/*Lock Flash*/
		HAL_STAT |= HAL_FLASH_Lock();
	}
	else
	{
		/*nothing*/
	}
	return FlashEraseStat;
}
static void Bootloader_Erase_Flash(void)
{
	uint16_t Host_PacketLen = BL_HOST_BUFFER[0U] + 1U;
	uint32_t Host_CRC32 = 0UL;
	uint8_t NumberOfStartSector = 0;
	uint8_t NumberOfSectors_Erease = 0;
	uint8_t FlashEraseStat = BL_UNSUCCESSFUL_ERASE;
	/*extract CRC from buffer */
	Host_CRC32 = *((uint32_t*)(BL_HOST_BUFFER + (Host_PacketLen - CRC_TYPE_SIZE)));

	/*Calcualte my crc and verify  crc */
	if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verifiy((uint32_t)(Host_PacketLen-CRC_TYPE_SIZE) , Host_CRC32) )
	{
#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("CRC Verification Passed %s" , BL_PRINT_NEWLINE);
#endif
		/*Send Ack +  Reply message length*/
		Bootloader_SendAck((uint8_t)1U);
		/*Extracts start sector number and number of sectors to erase */
		NumberOfStartSector = BL_HOST_BUFFER[2];
		NumberOfSectors_Erease = BL_HOST_BUFFER[3];
		/* Perfrom Flash erasing */
		FlashEraseStat = Perfrom_Flash_Erase(NumberOfStartSector , NumberOfSectors_Erease);
#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("Flsah Erase Stat -> %i %s" , FlashEraseStat , BL_PRINT_NEWLINE);
#endif
		 /* Report Erase operation status */
		BootLoader_SendData((uint8_t*)(&FlashEraseStat), (uint32_t)1UL);

	}
	else
	{
#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("CRC Verification Failed %s" , BL_PRINT_NEWLINE);
#endif
		/*Send NACK */
		Bootloader_SendNAck();
	}
}
static uint8_t Perfrom_Memory_Write(uint8_t* pDataBuffer , uint8_t DataLen , uint32_t StartMemAddress)
{
		HAL_StatusTypeDef HAL_stat = HAL_OK;
		uint8_t l_dataCounter = 0U;
		uint8_t WriteStat = BL_FLASH_WRITE_FAILED;
		/*UnLock Flash*/
		HAL_stat = HAL_FLASH_Unlock();

		for( ; (l_dataCounter < DataLen) && (HAL_OK == HAL_stat) ; ++l_dataCounter)
		{
			HAL_stat = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE , (StartMemAddress+l_dataCounter) , pDataBuffer[l_dataCounter]);
		}
		if((HAL_OK == HAL_stat))
		{
			WriteStat = BL_FLASH_WRITE_PASSED;
		}
		else{/*nothing*/}
		/*Lock Flash*/
		HAL_stat = HAL_FLASH_Lock();
		return WriteStat;
}
static void Bootloader_Memory_Write(void)
{
		uint16_t Host_PacketLen = BL_HOST_BUFFER[0U] + 1U;
		uint32_t Host_CRC32 = 0UL;
		uint32_t BaseMemeoryAddress = 0;
		uint8_t PayloadLen = 0;
		uint8_t MemoryWriteStat = 0;
		uint8_t Address_Verification = ADDRESS_IS_INVALID;
		/*extract CRC from buffer */
		Host_CRC32 = *((uint32_t*)(BL_HOST_BUFFER + (Host_PacketLen - CRC_TYPE_SIZE)));

		/*Calcualte my crc and verify  crc */
		if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verifiy((uint32_t)(Host_PacketLen-CRC_TYPE_SIZE) , Host_CRC32) )
		{
	#ifdef  BL_ENABLE_DEBUG
				BL_PrintMsg("CRC Verification Passed %s" , BL_PRINT_NEWLINE);
	#endif
			/*Send Ack +  Reply message length*/
			Bootloader_SendAck((uint8_t)1U);
			/* Extract base memory address and payload Len */
			BaseMemeoryAddress = *((uint32_t*)(&BL_HOST_BUFFER[2U]));
			PayloadLen  = BL_HOST_BUFFER[6U];
			/*Verifiy Memeory address access */
			Address_Verification = Bootloader_Host_Jump_Address_verification(BaseMemeoryAddress);
			if( ADDRESS_IS_VALID == Address_Verification  )
			{
				/* Perfrom Memory write */
				MemoryWriteStat = Perfrom_Memory_Write((BL_HOST_BUFFER+7UL) , PayloadLen , BaseMemeoryAddress);
				/* Report write operation status */
				BootLoader_SendData((uint8_t*)(&MemoryWriteStat), (uint32_t)1UL);
#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("Memory write Stat -> %i %s" , MemoryWriteStat , BL_PRINT_NEWLINE);
#endif

			}
			else
			{
				BootLoader_SendData((uint8_t*)(&Address_Verification), (uint32_t)1UL);
#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("Memory write Stat -> %i %s" , Address_Verification , BL_PRINT_NEWLINE);
#endif
			}
		}
		else
		{
	#ifdef  BL_ENABLE_DEBUG
				BL_PrintMsg("CRC Verification Failed %s" , BL_PRINT_NEWLINE);
	#endif
			/*Send NACK */
			Bootloader_SendNAck();
		}
}
static uint8_t BL_Change_ROP_Level(uint8_t RDP_Level)
{
	HAL_StatusTypeDef HAL_stat = HAL_OK;
	uint8_t ChangeROP_Stat = ROP_LEVEL_CHANGE_INVALID;
	FLASH_OBProgramInitTypeDef OBInit;
	/*Change RDP level to value to be written in reg*/
	switch(RDP_Level)
	{
	case BL_ROP_LEVEL_0:  RDP_Level = OB_RDP_LEVEL_0;  break;
	case BL_ROP_LEVEL_1:  RDP_Level = OB_RDP_LEVEL_1;  break;
	case BL_ROP_LEVEL_2:  RDP_Level = OB_RDP_LEVEL_2;  break;
	default:
		return ChangeROP_Stat;
		break;

	}
	/*	Flash option bytes unlock	*/
	HAL_stat |= HAL_FLASH_OB_Unlock();
	/* change rdp level */
	OBInit.Banks = FLASH_BANK_1;
	OBInit.OptionType = OPTIONBYTE_RDP;
	OBInit.RDPLevel = (uint32_t)RDP_Level;
	HAL_stat |= HAL_FLASHEx_OBProgram(&OBInit);
	if(HAL_OK == HAL_stat)
	{
		/* Lunch option bytes*/
		HAL_stat |= HAL_FLASH_OB_Launch();
	}
	else
	{	/*nothing*/	}

	/*	Flash  option bytes lock	*/
	HAL_stat |= HAL_FLASH_OB_Lock();

	if(HAL_OK == HAL_stat)
	{
		ChangeROP_Stat = ROP_LEVEL_CHANGE_VALID;
	}
	else {/*nothing*/}
	return ChangeROP_Stat;
}
static void Bootloader_ChangeReadProtection(void)
{
		uint16_t Host_PacketLen = BL_HOST_BUFFER[0U] + 1U;
		uint32_t Host_CRC32 = 0UL;
		uint8_t RDP_ChangeStat = ROP_LEVEL_CHANGE_INVALID;
		/*extract CRC from buffer */
		Host_CRC32 = *((uint32_t*)(BL_HOST_BUFFER + (Host_PacketLen - CRC_TYPE_SIZE)));

		/*Calcualte my crc and verify  crc */
		if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verifiy((uint32_t)(Host_PacketLen-CRC_TYPE_SIZE) , Host_CRC32) )
		{
	#ifdef  BL_ENABLE_DEBUG
				BL_PrintMsg("CRC Verification Passed %s" , BL_PRINT_NEWLINE);
	#endif
			/*	Send Ack +  Reply message length	*/
			Bootloader_SendAck((uint8_t)1U);
			/*	Change Read Protection level */
#ifdef BL_ENABLE_ROP_LEVEL_2

			RDP_ChangeStat = BL_Change_ROP_Level(BL_HOST_BUFFER[2U]);
#else
			if( BL_ROP_LEVEL_2 != BL_HOST_BUFFER[2U])
			{
				RDP_ChangeStat = BL_Change_ROP_Level(BL_HOST_BUFFER[2U]);
			}
			else
			{
				RDP_ChangeStat = ROP_LEVEL_CHANGE_INVALID;
			}

#endif

			/* Report Changing RDP  level status   */
			BootLoader_SendData((uint8_t*)(&RDP_ChangeStat), (uint32_t)1UL);
	#ifdef  BL_ENABLE_DEBUG
			BL_PrintMsg("Changing RDP  level status -> %i %s" ,RDP_ChangeStat , BL_PRINT_NEWLINE);
	#endif

		}
		else
		{
	#ifdef  BL_ENABLE_DEBUG
				BL_PrintMsg("CRC Verification Failed %s" , BL_PRINT_NEWLINE);
	#endif
			/*Send NACK */
			Bootloader_SendNAck();
		}
}
static uint8_t BL_Read_Flash_Protection_Level(void)
{

	 FLASH_OBProgramInitTypeDef pOBInit;
	 HAL_FLASHEx_OBGetConfig(&pOBInit);

	 return ((uint8_t)(pOBInit.RDPLevel));
}
static void Bootloader_Jump_to_user_main(void)
{
	HAL_StatusTypeDef HalStat = HAL_OK;
	/* Value of main stack pointer in our main application*/
	volatile uint32_t MSP_Val = *((volatile uint32_t*)FLASH_SECTOR2_BASE_ADDRESS);
	/* Reset handler definiation of our main application address and save it  in pointer to function  */
	volatile uint32_t ResetHanlderAddress = *((volatile uint32_t*)(FLASH_SECTOR2_BASE_ADDRESS+4UL));
	pMainApp MainAppFunc = (pMainApp)(ResetHanlderAddress);

	/*Set main stack pointer to Value of main stack pointer in our main application*/
	__set_MSP(MSP_Val);

	/* DeInitialization of Modules*/
	HalStat |= HAL_CRC_DeInit(BL_CRC_ENGINE_OBJ); 			 	/*	De init CRC*/

	HalStat |= HAL_UART_DeInit(BL_HOST_COMMUNICATION_UART); 	/*	De Init Host communication UART*/
	HalStat |= HAL_UART_DeInit(BL_DEBUG_UART);					 /*	De init DEBUG UART*/
	__HAL_RCC_GPIOA_CLK_DISABLE();		/*Disable Clocks */
	HalStat |= HAL_RCC_DeInit();
	/*Check if DeInitialization of Modules didnt success */
	if(HalStat != HAL_OK)
	{
		Error_Handler();
	}
	else
	{
		/*Call User Main Reset Handler Funcation*/
		MainAppFunc();
	}
}


static void BootLoader_SendData(uint8_t* pData , uint32_t DataLen)
{
	HAL_StatusTypeDef HalStat = HAL_ERROR;
#if  BL_ENABLE_UART_DEBUG_MSG == BL_DEBUG_METHOD
		/*Transmit msg using uart */
		HalStat = HAL_UART_Transmit(BL_DEBUG_UART , pData , DataLen , HAL_MAX_DELAY);
#elif  BL_ENABLE_SPI_DEBUG_MSG == BL_DEBUG_METHOD
		/*Transmit msg using spi */

#elif  BL_ENABLE_CAN_DEBUG_MSG == BL_DEBUG_METHOD
		/*Transmit msg using can*/
#endif

	/*to remove compiler warning*/
	UNUSED(HalStat);

}
/*****************************************/
