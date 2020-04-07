#include <stm32f10x.h>

#define IN (0x00)
#define OUT_10MHz (0x01)
#define OUT_2MHz (0x02)
#define OUT_50MHz (0x03)

#define OUT_PP (0x00)
#define OUT_OD (0x04)
#define OUT_APP (0x08)
#define OUT_AOD (0x0C)

#define IN_ADC (0x00)
#define IN_HIZ (0x04)
#define IN_PULL (0x08)

#define PORTE_CNF1_IN ((uint32_t)0x88888888)

typedef enum
{
	OFF = 0,
	ON = 1,
	LOW = 0,
	HIGH = 1
} tIOState;

typedef struct
{
	GPIO_TypeDef *GPIOx; // Имя порта
	uint16_t GPIO_Pin;	 // Номер порта
	uint8_t MODE;		 // Режим
	uint8_t DefState;	 // Стартовое значение
} tGPIO_Line;

typedef enum
{
	o_Address_A23 = 0,
	o_Address_A22 = 1,
	o_Address_A21,
	o_Address_A20,
	o_Address_A19,
	o_Address_A18,
	o_Address_A17,
	o_Address_A16,

	o_Address_A15,
	o_Address_A14,
	o_Address_A13,
	o_Address_A12,
	o_Address_A11,
	o_Address_A10,
	o_Address_A09,
	o_Address_A08,
	o_Address_A07,
	o_Address_A06,
	o_Address_A05,
	o_Address_A04,
	o_Address_A03,
	o_Address_A02,
	o_Address_A01,
	o_Address_A00,

	io_Data_D15,
	io_Data_D14,
	io_Data_D13,
	io_Data_D12,
	io_Data_D11,
	io_Data_D10,
	io_Data_D09,
	io_Data_D08,
	io_Data_D07,
	io_Data_D06,
	io_Data_D05,
	io_Data_D04,
	io_Data_D03,
	io_Data_D02,
	io_Data_D01,
	io_Data_D00,

	o_ALE,
	o_LDS,
	o_UDS,

	o_BHE,
	o_AS,
	o_WR,
	o_RD,
	i_SYNC,
	i_RDY,
	o_MRQ,
	o_DTR,

	o_UART_TX,
	o_UART_RX

} tIOLine;

void IO_Init(void);
void IO_ConfigLine(tIOLine, uint8_t, uint8_t);
void IO_SetLine(tIOLine, int);
void IO_InvertLine(tIOLine);
int IO_GetLine(tIOLine);
