/**
 * TODO:
 * - Реализовать проверку на правильность данных при чтении настроек с Flash
 * - Переделать char на uint8_t
 * - Цикл реализовать в зависимости от выравнивания по адресу или по данным DeviceConfiguration.AlignMode = 0x0;
 * - Реализовать функцию FD
*/

/**
 * \mainpage
 * \author Атапин А.В. tlt-andrew@yandex.ru 
 * \date
 *  
 * Используемый микроконтроллер STM32F103VE (high-density)
 * тестовая память toshiba g80477 tc551664 bji-15
 *
 *	PA00-PA07 - OUT - ADDR_H A16-A23
 *	PA08 - OUT - ALE
 *	PA09 - UART_TX
 *	PA10 - UART_RX
 *	PA11 - OUT - LDS
 *	PA12 - OUT - UDS
 *
 *	PC00 - OUT - BHE
 *	PC01 - OUT - AS
 *	PC02 - OUT - WR
 *	PC03 - OUT - RD
 *	PC08 - OUT - MRQ
 *	PC09 - OUT - DT/R
 *
 *	PD00-PD15 - OUT - ADDR_L A00-A15
 *	PE00-PE15 - INOUT - DATA_16b D00-D15
 *
 *	PC06 - IN - SYNC
 *	PC07 - IN - RDY
 *
 *	PB5 - IN - TIM3_CH2 (alternate function)
 *	Вход тактирования
*/
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver_flash.h"
#include "gpio_stm32f1.h"
#include "hwIndependentFunctions.h"
#include "init_stm32f103.h"
#include "main.h"
#include "stm32f10x.h"

/*Global*/
Message UART_Message;
Command CurrentCommand = {EMPTY, BLANK, 0, 0, 0, 0, 0, 0};
Command LastCommand = {EMPTY, BLANK, 0, 0, 0, 0, 0, 0};
flash FlashControl;
Config DeviceConfiguration;

/*UART buffer*/
char recieved[UART_RECIEVE_BUFFER];
uint32_t rec_len = 0;

/*Functions*/
void resetCC(void)
{
	CurrentCommand.Command = EMPTY;
	CurrentCommand.AttributeSpaceAddress = BLANK;
	CurrentCommand.Address = 0x0;
	CurrentCommand.Start_Address = 0x0;
	CurrentCommand.Stop_Address = 0x0;
	CurrentCommand.IncrementAddress = 0x0;
	CurrentCommand.Data = 0x0;
	CurrentCommand.RepeatNumber = 0x0;
}

void copyCCtoLC(void)
{
	LastCommand = CurrentCommand;
}

void taskExecCommand(void)
{
	char testRamAddress[UART_RECIEVE_BUFFER];
	char testWritedData[UART_RECIEVE_BUFFER];
	char testReadedData[UART_RECIEVE_BUFFER];

	if (CurrentCommand.Command != EMPTY)
	{
		if (CurrentCommand.Command == WRITE)
		{

			uint16_t readedData = 0;
			SendMessage("<--------------------------------------------------------");
			for (uint16_t i = 0; i <= CurrentCommand.RepeatNumber; i++)
			{
				WriteCommand();
				readedData = ReadCommand();
				snprintf(testReadedData, UART_RECIEVE_BUFFER, "%X", readedData);
				SendMessage(testReadedData);
				SendMessage("<-------write complete---\n\n");
				SendMessage("<--------------------------------------------------------");
			}
		}
		else if (CurrentCommand.Command == READ)
		{

			uint16_t readedData = 0;
			SendMessage("<--------------------------------------------------------");
			for (uint16_t i = 0; i <= CurrentCommand.RepeatNumber; i++)
			{
				readedData = ReadCommand();
				snprintf(testReadedData, UART_RECIEVE_BUFFER, "%X", readedData);
				SendMessage(testReadedData);
				SendMessage("<-------read complete---\n\n");
				SendMessage("<--------------------------------------------------------");
			}
		}
		else if ((CurrentCommand.Command == RAM) || (CurrentCommand.Command == ROM))
		{
			/*Проверка RAM или ROM*/
			uint16_t testedData = 0;
			SendMessage("Test RAM address:");
			for (uint16_t i = 0; i <= CurrentCommand.RepeatNumber; i++)
			{
				SendMessage("<--------------------------------------------------------");
				snprintf(testRamAddress, UART_RECIEVE_BUFFER, "%X", CurrentCommand.Address);
				snprintf(testWritedData, UART_RECIEVE_BUFFER, "%X", CurrentCommand.Data);

				char rWordAdd[11] = "Address=0x";
				SendMessage(strcat(rWordAdd, testRamAddress));
				char rWordData1[11] = "Writed=0x";
				SendMessage(strcat(rWordData1, testWritedData));

				WriteCommand();
				testedData = ReadCommand();
				snprintf(testReadedData, UART_RECIEVE_BUFFER, "%X", testedData);
				char rWordData2[11] = "Readed=0x";
				SendMessage(strcat(rWordData2, testReadedData));

				CurrentCommand.Address = CurrentCommand.Address + CurrentCommand.IncrementAddress;

				if (CurrentCommand.Data == testedData)
				{
					SendMessage("OK");
				}
				else
				{
					SendMessage("ERROR!!!");
				}

				SendMessage("<--------------------------------------------------------\n");
				delay_ms(150);
			}
		}
		else if (CurrentCommand.Command == FD)
		{
			/*Цикл поиска L level*/
		}
		copyCCtoLC();
		resetCC();
		CurrentCommand.RepeatNumber = 0;
	}
	else
	{
		/*code*/
	}
}

void delay_ms(uint16_t Count_ms)
{
	TIM1->ARR = Count_ms + Count_ms; //Указывает значения достигнув которого таймер остановится
	TIM1->EGR |= TIM_EGR_UG;
	TIM1->CNT = 0;						 //Обнуляем счётный регист
	TIM1->CR1 |= TIM_CR1_CEN;			 //Запускаем таймер, разрешаем его работу.
	while ((TIM1->SR & TIM_SR_UIF) == 0) //Ждём установки флага UIF = счёт закончен
	{
	}
	TIM1->SR &= ~TIM_SR_UIF; //Сброс флага
}

//Перевод шины данных в режим чтения (вход)
void DataBusRead(void)
{
	GPIOE->CRL |= PORTE_CNF1_IN;
	GPIOE->CRH |= PORTE_CNF1_IN;
	GPIOE->CRL &= ~GPIO_CRL_MODE;
	GPIOE->CRH &= ~GPIO_CRH_MODE;
}

//Перевод шины данных в режим записи (выход)
void DataBusWrite(void)
{
	GPIOE->CRL &= ~GPIO_CRL_CNF;
	GPIOE->CRH &= ~GPIO_CRH_CNF;
	GPIOE->CRL |= GPIO_CRL_MODE;
	GPIOE->CRH |= GPIO_CRH_MODE;
}

void SendMessage(char *str)
{
	uint8_t data = 0;
	do
	{
		data = *str++;
		USART1->DR = data;					//Send byte for test
		while (!(USART1->SR & USART_SR_TC)) //Wait tx end current data byte
		{
		};
		USART1->SR &= ~USART_SR_TC; //clear TX empty flag
	} while (*str);

	USART1->DR = 0x00A;
	while (!(USART1->SR & USART_SR_TC)) //wait tx end current data byte
	{
	};
	USART1->SR &= ~USART_SR_TC; //clear TX empty flag
}

void DefaultCommand(void)
{
	DeviceConfiguration.AlignMode = 0x01;
	DeviceConfiguration.DataBusSize = 0xFFFF;
	SendMessage("Loaded default device configuration: 16 bit, sync on address");
}

void HelpCommand(void)
{
	SendMessage("<---------------HELP--------------->");
	SendMessage("WRM, WRP - write command");
	SendMessage("RDM, RDP - read command");
	SendMessage(" ");
	SendMessage("format command@address=data");
	SendMessage("SAVE - command that save current device configuration");
	SendMessage("<-------------END-HELP------------->");
}

void WriteCommand(void)
{
	//Исходное состояние (T1)
	IO_SetLine(o_RD, HIGH); //Во время записи должен быть всегда в HIGH
	IO_SetLine(o_WR, HIGH);
	IO_SetLine(o_LDS, HIGH);
	IO_SetLine(o_UDS, HIGH);
	DataBusWrite();
	GPIOD->ODR = CurrentCommand.Address; //адрес
	GPIOE->ODR = CurrentCommand.Data;	 //данные
	TIM3->CNT = 0;

	//Ожидание 2 в таймере
	while (TIM3->CNT != 2)
	{
	}

	//Цикл записи (T2, T3, T4)
	IO_SetLine(o_Address_A16, LOW); //CE = LOW
	IO_SetLine(o_Address_A17, LOW); //CE = LOW
	IO_SetLine(o_LDS, LOW);
	IO_SetLine(o_UDS, LOW);
	IO_SetLine(o_WR, LOW);

	//Ожидание 7 в таймере
	while (TIM3->CNT != 7)
	{
	}

	//Завершение цикла записи, переход в исходное состояние (T4.2)
	IO_SetLine(o_WR, HIGH);
	IO_SetLine(o_Address_A16, HIGH); //CE = LOW
	IO_SetLine(o_Address_A17, HIGH); //CE = LOW
	IO_SetLine(o_LDS, HIGH);
	IO_SetLine(o_UDS, HIGH);
}

uint16_t ReadCommand(void)
{
	uint16_t rData = 0;

	//Исходное состояние (T1)
	IO_SetLine(o_WR, HIGH); //Во время чтения должен быть всегда в HIGH
	IO_SetLine(o_RD, HIGH);
	IO_SetLine(o_Address_A16, HIGH); //CE
	IO_SetLine(o_Address_A17, HIGH); //CE
	IO_SetLine(o_LDS, HIGH);
	IO_SetLine(o_UDS, HIGH);
	DataBusRead();
	GPIOD->ODR = CurrentCommand.Address;
	//Ожидание 2 в таймере
	while (TIM3->CNT != 2)
	{
	}
	//Цикл чтения (T2, T3, T4)
	IO_SetLine(o_Address_A16, LOW); //CE = LOW
	IO_SetLine(o_Address_A17, LOW); //CE = LOW
	IO_SetLine(o_RD, LOW);
	IO_SetLine(o_LDS, LOW);
	IO_SetLine(o_UDS, LOW);

	rData = GPIOE->IDR; //чтение данных

	//Ожидание 7 в таймере
	while (TIM3->CNT != 7)
	{
	}

	//Завершение цикла чтения, переход в исходное состояние (T4)
	IO_SetLine(o_Address_A16, HIGH); //CE
	IO_SetLine(o_Address_A17, HIGH); //CE
	IO_SetLine(o_LDS, HIGH);
	IO_SetLine(o_UDS, HIGH);
	IO_SetLine(o_RD, HIGH);

	return rData;
}

void errorType(uint32_t err_number)
{
	switch (err_number)
	{
	case 0:
		SendMessage(strcat(recieved, " have no errors"));
		break;
	case 1:
		SendMessage("DATA out of range");
		break;
	case 2:
		SendMessage("ADDRESS out of range");
		break;
	case 3:
		SendMessage("ADDRESS is not even number\nSet DATA size 8 bit\nCommand - SB");
		break;
	case 4:
		SendMessage("ADDRESS or DATA have no hex format");
		break;
	case 5:
		SendMessage("START ADDRESS more than STOP ADDRESS");
		break;
	case 6:
		SendMessage("ADDRESS have no hex format");
		break;
	case 7:
		SendMessage("Unable to loop last command");
		break;
	case 8:
		SendMessage("Too much repeat counter (32bit)");
		break;
	case 9:
		SendMessage("You tried calculate lenght from NULL pointer");
		break;
	case 10:
		SendMessage("Unknow command");
		break;
	default:
		printf("Unknow type error");
	}
}

void SaveCommand(void)
{
	uint32_t flash_wr_count = 0;
	flash_wr_count = FlashControl.write_count;
	flash_wr_count++;

	flash_unlock();
	flash_erase_page(FLASH_CONFIG_START_ADDRESS);

	flash_write(FLASH_CONFIG_START_ADDRESS, FLASH_KEY_WORD);										  //Запись ключевого слова
	flash_write(FLASH_CONFIG_START_ADDRESS + FLASH_BYTE_STEP, flash_wr_count);						  //Запись количества сохранений конфигурации
	flash_write(FLASH_CONFIG_START_ADDRESS + 2 * FLASH_BYTE_STEP, *(uint32_t *)&DeviceConfiguration); //Запись конфигурации устройства

	flash_lock();
	SendMessage("Current configuration saved");
}

void initDeviceConfig(void)
{
	FlashControl.hash = *(__IO uint32_t *)FLASH_CONFIG_START_ADDRESS;
	FlashControl.write_count = *(__IO uint32_t *)(FLASH_CONFIG_START_ADDRESS + FLASH_BYTE_STEP);
	if ((FlashControl.write_count != 0xFFFFFFFF) && (FlashControl.hash == FLASH_KEY_WORD))
	{
		DeviceConfiguration = *(__IO Config *)(FLASH_CONFIG_START_ADDRESS + 2 * FLASH_BYTE_STEP);
		if (DeviceConfiguration.DataBusSize == 0xFF)
		{
			SendMessage("8-bit mode");
		}
		else
		{
			SendMessage("16-bit mode");
		}

		if (DeviceConfiguration.AlignMode == 0x0)
		{
			SendMessage("Sync on DATA");
		}
		else
		{
			SendMessage("Sync on ADDRESS");
		}
		SendMessage("Configuration loaded from FLASH");
	}
	else
	{
		DefaultCommand();
	}
}

void parseUARTMessage(char *uart_message)
{

	uint32_t err_code = 0;
	uint32_t cmd_len = 0;
	uint32_t address_len = 0;
	uint32_t data_len = 0;
	uint32_t start_address_len = 0;
	uint32_t stop_address_len = 0;
	uint32_t repeat_number_len = 0;
	err_code = 0;

	/*Выделяем команду из принятого по UART сообщению*/
	char *cmd = strtok(uart_message, "@");
	UART_Message.Command = cmd;

	if (UART_Message.Command != NULL)
	{
		cmd_len = (uint32_t)strlen(UART_Message.Command);
		/*Проходим перебор по пришедшей команде*/
		/*Команда записи*/
		if (((strncmp(UART_Message.Command, "WRM", cmd_len) == 0) ||
			 (strncmp(UART_Message.Command, "WRP", cmd_len) == 0)) &&
			(strlen(UART_Message.Command) == strlen("WRP")))
		{
			char *address = strtok(NULL, "=");
			char *data = strtok(NULL, "");
			UART_Message.Address = address;
			UART_Message.Data = data;

			if ((UART_Message.Address != 0) && (UART_Message.Data != 0))
			{
				address_len = (uint32_t)strlen(UART_Message.Address);
				data_len = (uint32_t)strlen(UART_Message.Data);
				if ((checkHexFormat(UART_Message.Address, address_len) == 0) &&
					(checkHexFormat(UART_Message.Data, data_len) == 0))
				{
					if (checkRange(strtol(UART_Message.Address, 0, 16), 0xFFFFFF) == 0)
					{
						if (checkRange(strtol(UART_Message.Data, 0, 16), DeviceConfiguration.DataBusSize) == 0)
						{
							if ((DeviceConfiguration.DataBusSize == 0xFFFF) &&
								(checkParity(strtol(UART_Message.Address, 0, 16)) != 0))
							{
								err_code = 3;
							}
							else
							{
								CurrentCommand.Command = WRITE;
								CurrentCommand.Address = strtol(UART_Message.Address, 0, 16);
								CurrentCommand.Data = strtol(UART_Message.Data, 0, 16);
								CurrentCommand.Start_Address = 0x0;
								CurrentCommand.Stop_Address = 0x0;
								CurrentCommand.IncrementAddress = 0x0;
								CurrentCommand.RepeatNumber = 0x0;

								if (UART_Message.Command == "WRM")
								{
									CurrentCommand.AttributeSpaceAddress = MEM;
								}
								else
								{
									CurrentCommand.AttributeSpaceAddress = IO;
								}

								err_code = 0;
							}
						}
						else
						{
							err_code = 1;
						}
					}
					else
					{
						err_code = 2;
					}
				}
				else
				{
					err_code = 4;
				}
			}
			else
			{
				err_code = 9;
			}
		}

		/*Команда чтения (Read Command)*/
		else if (((strncmp(UART_Message.Command, "RDM", strlen("RDM")) == 0) ||
				  (strncmp(UART_Message.Command, "RDP", strlen("RDP")) == 0)) &&
				 (strlen(UART_Message.Command) == strlen("RDP")))
		{
			char *address = strtok(NULL, "");
			UART_Message.Address = address;
			if (UART_Message.Address != 0)
			{
				address_len = (uint32_t)strlen(UART_Message.Address);

				if (checkHexFormat(UART_Message.Address, address_len) == 0)
				{
					if (checkRange(strtol(UART_Message.Address, 0, 16), 0xFFFFFF) == 0)
					{

						if ((DeviceConfiguration.DataBusSize == 0xFFFF) &&
							(checkParity(strtol(UART_Message.Address, 0, 16)) != 0))
						{
							err_code = 3;
						}
						else
						{
							CurrentCommand.Command = READ;
							CurrentCommand.Address = strtol(UART_Message.Address, 0, 16);
							CurrentCommand.Data = 0x0;
							CurrentCommand.Start_Address = 0x0;
							CurrentCommand.Stop_Address = 0x0;
							CurrentCommand.IncrementAddress = 0x0;
							CurrentCommand.RepeatNumber = 0x0;

							if (UART_Message.Command == "RDM")
							{
								CurrentCommand.AttributeSpaceAddress = MEM;
							}
							else
							{
								CurrentCommand.AttributeSpaceAddress = IO;
							}

							err_code = 0;
						}
					}
					else
					{
						err_code = 2;
					}
				}
				else
				{
					err_code = 6;
				}
			}
			else
			{
				err_code = 9;
			}
		}

		/*Тест поиска (Test FIND)*/
		else if (((strncmp(UART_Message.Command, "FDM", cmd_len) == 0) ||
				  (strncmp(UART_Message.Command, "FDP", cmd_len) == 0)) &&
				 (strlen(UART_Message.Command) == strlen("FDP")))
		{

			char *start_address = strtok(NULL, "-");
			char *stop_address = strtok(NULL, "=");
			char *repeat = strtok(NULL, "");
			UART_Message.Start_Address = start_address;
			UART_Message.Stop_Address = stop_address;
			UART_Message.RepeatNumber = repeat;

			if ((UART_Message.Start_Address != 0) && (UART_Message.Stop_Address != 0) && (UART_Message.RepeatNumber != 0))
			{
				start_address_len = (uint32_t)strlen(UART_Message.Start_Address);
				stop_address_len = (uint32_t)strlen(UART_Message.Stop_Address);
				repeat_number_len = (uint32_t)strlen(UART_Message.RepeatNumber);

				if ((checkHexFormat(UART_Message.Start_Address, start_address_len) == 0) &&
					(checkHexFormat(UART_Message.Stop_Address, stop_address_len) == 0) &&
					(checkHexFormat(UART_Message.RepeatNumber, repeat_number_len) == 0))
				{

					if ((checkRange(strtol(UART_Message.Start_Address, 0, 16), strtol(UART_Message.Stop_Address, 0, 16)) == 0) &&
						(checkRange(strtol(UART_Message.Stop_Address, 0, 16), 0xFFFFFF) == 0))
					{

						if ((DeviceConfiguration.DataBusSize == 0xFFFF) &&
							((checkParity(strtol(UART_Message.Start_Address, 0, 16)) != 0) ||
							 (checkParity(strtol(UART_Message.Stop_Address, 0, 16)) != 0)))
						{
							err_code = 3;
						}
						else
						{
							CurrentCommand.Command = FD;
							CurrentCommand.Address = 0x0;
							CurrentCommand.Data = 0x0;
							CurrentCommand.Start_Address = strtol(UART_Message.Start_Address, 0, 16);
							CurrentCommand.Stop_Address = strtol(UART_Message.Stop_Address, 0, 16);
							CurrentCommand.IncrementAddress = 0x0;
							CurrentCommand.RepeatNumber = strtol(UART_Message.RepeatNumber, 0, 16);

							if (UART_Message.Command == "FDM")
							{
								CurrentCommand.AttributeSpaceAddress = MEM;
							}
							else
							{
								CurrentCommand.AttributeSpaceAddress = IO;
							}

							err_code = 0;
						}
					}
					else
					{
						err_code = 2;
					}
				}
				else
				{
					err_code = 6;
				}
			}
			else
			{
				err_code = 9;
			}
		}

		/*Команда RAM или ROM*/
		else if (((strncmp(UART_Message.Command, "RAM", cmd_len) == 0) |
				  (strncmp(UART_Message.Command, "ROM", cmd_len) == 0)) &&
				 (strlen(UART_Message.Command) == strlen("ROM")))
		{
			char *start_address = strtok(NULL, "-");
			char *stop_address = strtok(NULL, "");
			UART_Message.Start_Address = start_address;
			UART_Message.Stop_Address = stop_address;

			if ((UART_Message.Start_Address != 0) && (UART_Message.Stop_Address != 0))
			{
				start_address_len = (uint32_t)strlen(UART_Message.Start_Address);
				stop_address_len = (uint32_t)strlen(UART_Message.Stop_Address);

				if ((checkHexFormat(UART_Message.Start_Address, start_address_len) == 0) &&
					checkHexFormat(UART_Message.Stop_Address, stop_address_len) == 0)
				{
					if ((checkRange(strtol(UART_Message.Start_Address, 0, 16), strtol(UART_Message.Stop_Address, 0, 16)) == 0) &&
						(checkRange(strtol(UART_Message.Stop_Address, 0, 16), 0xFFFFFF) == 0))
					{

						if ((DeviceConfiguration.DataBusSize == 0xFFFF) &&
							((checkParity(strtol(UART_Message.Start_Address, 0, 16)) != 0) ||
							 (checkParity(strtol(UART_Message.Stop_Address, 0, 16)) != 0)))
						{
							err_code = 3;
						}
						else
						{
							if (UART_Message.Command == "RAM")
							{
								CurrentCommand.Command = RAM;
							}
							else
							{
								CurrentCommand.Command = ROM;
							}
							CurrentCommand.Start_Address = strtol(UART_Message.Start_Address, 0, 16);
							CurrentCommand.Stop_Address = strtol(UART_Message.Stop_Address, 0, 16);
							CurrentCommand.Address = CurrentCommand.Start_Address;
							CurrentCommand.AttributeSpaceAddress = BLANK;

							if (DeviceConfiguration.DataBusSize == 0xFFFF)
							{
								CurrentCommand.Data = 0xBDBD;
								CurrentCommand.IncrementAddress = 0x2;
								CurrentCommand.RepeatNumber = (CurrentCommand.Stop_Address - CurrentCommand.Start_Address) >> 1;
							}
							else
							{
								CurrentCommand.Data = 0xBD;
								CurrentCommand.IncrementAddress = 0x1;
								CurrentCommand.RepeatNumber = CurrentCommand.Stop_Address - CurrentCommand.Start_Address;
							}

							err_code = 0;
						}
					}
					else
					{
						err_code = 2;
					}
				}
				else
				{
					err_code = 4;
				}
			}
			else
			{
				err_code = 9;
			}
		}

		/*Команда повтора*/
		/** суть в том, чтобы CurrentComman сделать из LastCommand с атрибутом повтора*/
		else if ((strncmp(UART_Message.Command, "LOOP", strlen(UART_Message.Command)) == 0) &&
				 (strlen(UART_Message.Command) == strlen("LOOP")))
		{
			char *repeatNumber = strtok(NULL, "");
			UART_Message.RepeatNumber = repeatNumber;
			if (UART_Message.RepeatNumber != 0)
			{
				if (checkRange(strtol(UART_Message.RepeatNumber, 0, 16), 0xFFFFFFFF) == 0)
				{
					if ((LastCommand.Command == WRITE) || (LastCommand.Command == READ))
					{
						CurrentCommand.Command = LastCommand.Command;
						CurrentCommand.Address = LastCommand.Address;
						CurrentCommand.Data = LastCommand.Data;
						CurrentCommand.Start_Address = LastCommand.Start_Address;
						CurrentCommand.Stop_Address = LastCommand.Stop_Address;
						CurrentCommand.IncrementAddress = LastCommand.IncrementAddress;
						CurrentCommand.RepeatNumber = strtol(UART_Message.RepeatNumber, 0, 16);
						err_code = 0;
					}
					else
					{
						err_code = 7;
					}
				}
				else
				{
					err_code = 8;
				}
			}
			else
			{
				err_code = 7;
			}
		}

		/*Команда сохранения текущей конфигурации*/
		else if ((strncmp(UART_Message.Command, "SAVE", strlen(UART_Message.Command)) == 0) &&
				 (strlen(UART_Message.Command) == strlen("SAVE")))
		{
			SaveCommand();
		}

		/*Команда вызова встроенной справки*/
		else if ((strncmp(UART_Message.Command, "HELP", strlen(UART_Message.Command)) == 0) &&
				 (strlen(UART_Message.Command) == strlen("HELP")))
		{
			HelpCommand();
		}

		/*Команда сброса настроек в значение по умолчанию*/
		else if ((strncmp(UART_Message.Command, "DEFAULT", strlen(UART_Message.Command)) == 0) &&
				 (strlen(UART_Message.Command) == strlen("DEFAULT")))
		{
			DefaultCommand();
		}

		/*Команда установки размера данных 8 бит*/
		else if ((strncmp(UART_Message.Command, "SB", strlen(UART_Message.Command)) == 0) &&
				 strlen(UART_Message.Command) == strlen("SB"))
		{
			DeviceConfiguration.DataBusSize = 0xFF;
			SendMessage("8-bit mode");
		}

		/*Команда установки размера данных 16 бит*/
		else if ((strncmp(UART_Message.Command, "SW", strlen(UART_Message.Command)) == 0) &&
				 (strlen(UART_Message.Command) == strlen("SW")))
		{
			DeviceConfiguration.DataBusSize = 0xFFFF;
			SendMessage("16-bit mode");
		}

		/*Команда выравнивания по данным*/
		else if ((strncmp(UART_Message.Command, "SD", strlen(UART_Message.Command)) == 0) &&
				 (strlen(UART_Message.Command) == strlen("SD")))
		{
			DeviceConfiguration.AlignMode = 0x0;
			SendMessage("Sync on DATA");
		}

		/*Команда выравнивания по адресу*/
		else if ((strncmp(UART_Message.Command, "SA", strlen(UART_Message.Command)) == 0) &&
				 (strlen(UART_Message.Command) == strlen("SA")))
		{
			DeviceConfiguration.AlignMode = 0x1;
			SendMessage("Sync on ADDRESS");
		}

		/*Код ошибки при неизвестной команде*/
		else
		{
			err_code = 10;
		}
	}
	else
	{
		err_code = 9;
	}

	errorType(err_code);
	return;
}

void RecieveMessage(char data)
{
	data = toupper(data);				   /*Функция преобразования строчной буквы в прописную*/
	if (rec_len < UART_RECIEVE_BUFFER - 1) /*Проверка переполнения буфера UART*/
	{
		if (data == 13)
		{
			SendMessage(">");
			recieved[rec_len++] = 0;	/*Делаем нуль-терминированную строку*/
			parseUARTMessage(recieved); /*Отправляем принятую команду на обработку*/
			rec_len = 0;				/*Очищаем буфер приёма*/
		}
		else
		{
			recieved[rec_len++] = data; /*Размещаем принятый символ в приёмный буфер*/
		}
	}
	else
	{
		/*При переполнении буфера UART_RECIEVE_BUFFER выдаем сообщение в терминал*/
		SendMessage("Too long command");
		rec_len = 0; /*Обнуляем счетчик принятого сообщения*/
	}
}

void USART1_IRQHandler(void)
{
	char uart_data;
	if (USART1->SR & USART_SR_RXNE)
	{
		uart_data = USART1->DR;
		RecieveMessage(uart_data);
	}
	else
	{
		SendMessage("Exeption #1 - UART");
	}
}

/*MAIN*/
int main()
{
	IO_Init();
	initUSART1_19200();
	initIRQHandler();
	initTIM1_msTimer();
	initTIM3CH2_externalCounter();
	initDeviceConfig();
	//delay_ms(500); //Задержка для обновления всех регистров таймеров
	SendMessage("Ready");

	while (1)
	{
		taskExecCommand();
	}
}
