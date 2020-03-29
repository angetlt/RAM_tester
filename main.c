/**
 * \mainpage
 * \author Атапин А.В. tlt-andrew@yandex.ru
 * \date
 *   
 */

/*TODO:
-в функции USARTInit() реализовать настройку скорости из FLASH памяти
- если выбран 16-битный режим, то ввести проверку четности адреса
- реализовать ответ типа "ЭХО-ОК"
*	
Используемый микроконтроллер STM32F103VE (high-density)
тестовая память toshiba g80477 tc551664 bji-15
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
*	PB0 - in - TIM3_CH3 (alternate function)
*	Вход тактирования
*/

#include <stm32f10x.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <gpio_stm32f1.h>
#include <driver_flash.h>
#include <init_stm32f103.h>

/*FLASH MACROS*/
#define FLASH_CONFIG_START_ADDRESS ((uint32_t)0x0800F000)
#define FLASH_KEY_WORD ((uint32_t)0x248F135B)
#define FLASH_STEP 0x0F0 //4 bytes
#define FLASH_PAGE 0x080 //128 pages

/*UART RECIEVE BUFFER*/
#define UART_RECIEVE_BUFFER 40

/*Declarations*/
void ReadCommand(void);

typedef enum
{
	WRITE,
	READ,
	RAM,
	ROM
} CommandType;

typedef struct
{
	CommandType Command;
	uint32_t Address;
	uint32_t Start_Address;
	uint32_t Stop_Address;
	uint16_t IncrementAddress;
	uint32_t Data;
	uint32_t Attributes;
} Command;

//Тип после
typedef struct
{
	char *Command;
	char *Address;
	char *Data;
} Message;

//Тип хранения конфигурации внутрисхемного эмулятора
typedef struct
{
	uint8_t AlignMode; ///AlignMode - Синхронизация по адресу или по данным
	uint8_t ModeX;
	/**OSC - частота тактирования процессорной шины */
	uint16_t OSC;
	uint32_t USART_BRR;
} Config;

typedef struct
{
	uint32_t write_count;
	uint32_t hash;
} flash;

/*Global*/
Message UART_Message;
Command CurrentCommand;
Command LastCommand;
flash FlashControl;
Config DeviceConfiguration;

/*Default Messages*/
char *MessageUnknowCommand = "UNKNOW COMMAND";
char *MessageBadData = "BAD DATA";
char *MessageBadAddress = "BAD ADDRESS";
char *MessageOk = "OK";
char *MessageDone = "DONE";

/*UART buffer*/
char received[UART_RECIEVE_BUFFER], rec_len = 0;

/*Functions*/
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
	//TODO: check buffer
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
	SendMessage("Configuration loaded from default");
}

void HelpCommand(void)
{
	SendMessage("help - this help");
}

void WriteCommand(void)
{
	SendMessage("WRITE Command");
	//Исходное состояние (T1)
	IO_SetLine(o_RD, HIGH); //Во время записи должен быть всегда в HIGH
	IO_SetLine(o_WR, HIGH);
	IO_SetLine(o_LDS, HIGH);
	IO_SetLine(o_UDS, HIGH);
	DataBusWrite();
	GPIOD->ODR = CurrentCommand.Address; //адрес
	GPIOE->ODR = CurrentCommand.Data;	//данные

	//Цикл записи (T2, T3, T4)
	IO_SetLine(o_Address_A16, LOW); //CE = LOW
	IO_SetLine(o_Address_A17, LOW); //CE = LOW
	IO_SetLine(o_LDS, LOW);
	IO_SetLine(o_UDS, LOW);
	IO_SetLine(o_WR, LOW);

	//Завершение цикла записи, переход в исходное состояние (T4)
	IO_SetLine(o_WR, HIGH);
	IO_SetLine(o_Address_A16, HIGH); //CE = LOW
	IO_SetLine(o_Address_A17, HIGH); //CE = LOW
	IO_SetLine(o_LDS, HIGH);
	IO_SetLine(o_UDS, HIGH);
	SendMessage("<-------write complete---");

	ReadCommand();
}

void ReadCommand(void)
{
	//TODO:
	//1. Сделать привязку к внешнему тактированию
	//2. Цикл реализовать в зависимости от выравнивания по адресу или по данным DeviceConfiguration.AlignMode = 0x0;

	uint16_t testdata = 0;
	char rMessage[UART_RECIEVE_BUFFER];

	SendMessage("READ Command");

	//Исходное состояние (T1)
	IO_SetLine(o_WR, HIGH); //Во время чтения должен быть всегда в HIGH
	IO_SetLine(o_RD, HIGH);
	IO_SetLine(o_Address_A16, HIGH); //CE
	IO_SetLine(o_Address_A17, HIGH); //CE
	IO_SetLine(o_LDS, HIGH);
	IO_SetLine(o_UDS, HIGH);
	DataBusRead();
	GPIOD->ODR = CurrentCommand.Address;

	//Цикл чтения (T2, T3, T4)
	IO_SetLine(o_Address_A16, LOW); //CE = LOW
	IO_SetLine(o_Address_A17, LOW); //CE = LOW
	IO_SetLine(o_RD, LOW);
	IO_SetLine(o_LDS, LOW);
	IO_SetLine(o_UDS, LOW);

	testdata = GPIOE->IDR; //чтение данных

	//Завершение цикла чтения, переход в исходное состояние (T4)
	IO_SetLine(o_Address_A16, HIGH); //CE
	IO_SetLine(o_Address_A17, HIGH); //CE
	IO_SetLine(o_LDS, HIGH);
	IO_SetLine(o_UDS, HIGH);
	IO_SetLine(o_RD, HIGH);

	snprintf(rMessage, UART_RECIEVE_BUFFER, "%X", testdata);
	SendMessage(rMessage);
	SendMessage("<----read complete---");
}

uint32_t hexCheck(char *str, uint32_t length)
{
	uint32_t err_count = 0;
	for (uint32_t i = 0; i < length; i++)
	{
		if (isxdigit(str[i]) == 0)
		{
			err_count++;
		}
	}
	return err_count;
}

uint32_t addressCheck(uint32_t iAddress)
{
	//Проверка на диапазон - от 0h000000 до 0hFFFFFF‬ (24 бит)
	uint32_t err_count = 0;
	if (iAddress <= 0xFFFFFF)
	{
		err_count = 0;
	}
	else
	{
		err_count++;
	}
	return err_count;
}

uint32_t dataCheck(uint32_t iData)
{
	//Проверка на диапазон
	//от 0x0 до 0xFF‬ (8 бит) DeviceConfiguration.ModeX = 0x0
	//от 0x0 до 0xFFFF (16 бит) DeviceConfiguration.ModeX = 0x1
	uint32_t err_count = 0;

	if (DeviceConfiguration.ModeX == 0)
	{
		if (iData <= 0xFF)
		{
			err_count = 0;
		}
		else
		{
			err_count++;
		}
	}
	else
	{
		if (iData <= 0xFFFF)
		{
			err_count = 0;
		}
		else
		{
			err_count++;
		}
	}

	return err_count;
}

uint32_t Check(void)
{
	uint32_t adr_len = strlen(UART_Message.Address);
	uint32_t data_len = strlen(UART_Message.Data);
	uint32_t check_err_count = 0;
	if ((hexCheck(UART_Message.Address, adr_len) == 0) &
		(hexCheck(UART_Message.Data, data_len) == 0))
	{
		CurrentCommand.Address = strtol(UART_Message.Address, 0, 16);
		CurrentCommand.Data = strtol(UART_Message.Data, 0, 16);
		if (addressCheck(CurrentCommand.Address) == 0)
		{
			if (dataCheck(CurrentCommand.Data) == 0)
			{
				check_err_count = 0;
			}
			else
			{
				check_err_count = 8;
			}
		}
		else
		{
			check_err_count = 16;
		}
	}
	else
	{
		check_err_count = 32;
	}

	return check_err_count;
}

void errorType(uint32_t err_number)
{
	switch (err_number)
	{
	case 0:
		SendMessage("No errors");
		break;
	case 8:
		SendMessage("Data out of range");
		break;
	case 16:
		SendMessage("Address out of range");
		break;
	case 32:
		SendMessage("Data or address have no hex format");
		break;
	default:
		printf("Unknow type error");
	}
}

void SaveCommand(void)
{
	flash_unlock();
	flash_erase_page(FLASH_CONFIG_START_ADDRESS);
	flash_write(FLASH_CONFIG_START_ADDRESS, DeviceConfiguration.ModeX);
	flash_write(FLASH_CONFIG_START_ADDRESS + 0x4, DeviceConfiguration.AlignMode);
	flash_lock();
	SendMessage("Current configuration saved");
}

void ParseUARTMessage(void)
{
	uint32_t cmd_len = strlen(UART_Message.Command);

	uint32_t err_code = 0;

	if ((strncmp(UART_Message.Command, "WR", cmd_len) == 0) |
		(strncmp(UART_Message.Command, "WRM", cmd_len) == 0) |
		(strncmp(UART_Message.Command, "WRP", cmd_len) == 0))
	{
		err_code = Check();
		if (err_code == 0)
		{
			CurrentCommand.Command = WRITE;
			WriteCommand();
		}
		else
		{
			errorType(err_code);
		}
	}
	else if ((strncmp(UART_Message.Command, "RD", cmd_len) == 0) |
			 (strncmp(UART_Message.Command, "RDM", cmd_len) == 0) |
			 (strncmp(UART_Message.Command, "RDP", cmd_len) == 0))
	{
		err_code = Check();
		if (err_code == 0)
		{
			CurrentCommand.Command = READ;
			ReadCommand();
		}
		else
		{
			errorType(err_code);
		}
	}
	else if ((strncmp(UART_Message.Command, "LOOP", 4) == 0))
	{
		//RepeatCommand();
	}
	else if ((strncmp(UART_Message.Command, "SAVE", 4) == 0))
	{
		SaveCommand();
	}
	else if (strncmp(UART_Message.Command, "HELP", 4) == 0)
	{
		HelpCommand();
	}
	else if (strncmp(UART_Message.Command, "DEFAULT", sizeof("DEFAULT")) == 0)
	{
		DefaultCommand();
	}
	else if (strncmp(UART_Message.Command, "SB", cmd_len) == 0)
	{
		DeviceConfiguration.ModeX = 0x0;
		SendMessage("8-bit mode");
	}
	else if (strncmp(UART_Message.Command, "SW", cmd_len) == 0)
	{
		DeviceConfiguration.ModeX = 0x1;
		SendMessage("16-bit mode");
	}
	else if (strncmp(UART_Message.Command, "SD", cmd_len) == 0)
	{
		DeviceConfiguration.AlignMode = 0x0;
		SendMessage("Sync on DATA");
	}
	else if (strncmp(UART_Message.Command, "SA", cmd_len) == 0)
	{
		DeviceConfiguration.AlignMode = 0x1;
		SendMessage("Sync on ADDRESS");
	}
	else
	{
		SendMessage(MessageUnknowCommand);
	}
	return;
}

void Lexem_UART_Message(char *uart_message)
{
	//	UART_Message = {0, 0, 0};
	char *cmd = strtok(uart_message, "@");
	char *address = strtok(NULL, "=");
	char *data = strtok(NULL, "");
	UART_Message.Command = cmd;
	UART_Message.Address = address;
	UART_Message.Data = data;
	ParseUARTMessage();
}

void RecieveMessage(char data)
{
	data = toupper(data);
	if (rec_len < UART_RECIEVE_BUFFER - 1)
	{
		if (data == 13)
		{
			SendMessage(">");
			received[rec_len++] = 0;	  // делаем нуль-терминированную строку
			Lexem_UART_Message(received); // отправляем принятую команду на обработку
			rec_len = 0;				  // очищаем буфер приёма
		}
		else
		{
			received[rec_len++] = data; // Складываем символ в приёмный буфер
		}
	}
	else
	{
		//TODO: выключаем прием
		SendMessage("Too long command");
		rec_len = 0;
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

void TIM1_UP_IRQHandler(void)
{
	TIM1->SR &= ~TIM_SR_UIF; //очистили статус регистр
	if (GPIOB->ODR & GPIO_ODR_ODR0)
	{
		GPIOB->ODR &= ~GPIO_ODR_ODR0;
	}
	else
	{
		GPIOB->ODR |= GPIO_ODR_ODR0;
	}
}

void DeviceConfigurationInit(void)
{
	SendMessage("Preparing device configuration...");
	FlashControl.hash = *(__IO uint32_t *)FLASH_CONFIG_START_ADDRESS;
	if (FlashControl.hash == FLASH_KEY_WORD)
	{
		DeviceConfiguration.AlignMode = *(__IO uint8_t *)FLASH_CONFIG_START_ADDRESS + 0x0F;
		DeviceConfiguration.ModeX = *(__IO uint8_t *)FLASH_CONFIG_START_ADDRESS + 0x10;
		SendMessage("Configuration loaded from FLASH");
	}
	else
	{
		DefaultCommand();
	}
}

/*MAIN*/
int main(int argc, char *argv[])
{
	IO_Init();
	USARTInit();
	IRQHandlerInit();
	TIM1_Init_ms_timer();
	SendMessage("Ready");
	while (1)
	{
	}
}
