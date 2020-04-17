#include <gpio_stm32f1.h>

const tGPIO_Line IOs[] = {

	{GPIOA, 0, OUT_50MHz + OUT_PP, HIGH}, //Address_A23
	{GPIOA, 1, OUT_50MHz + OUT_PP, HIGH}, //Address_A22
	{GPIOA, 2, OUT_50MHz + OUT_PP, HIGH}, //Address_A21
	{GPIOA, 3, OUT_50MHz + OUT_PP, HIGH}, //Address_A20
	{GPIOA, 4, OUT_50MHz + OUT_PP, HIGH}, //Address_A19
	{GPIOA, 5, OUT_50MHz + OUT_PP, HIGH}, //Address_A18
	{GPIOA, 6, OUT_50MHz + OUT_PP, HIGH}, //Address_A17
	{GPIOA, 7, OUT_50MHz + OUT_PP, HIGH}, //Address_A16

	{GPIOD, 15, OUT_50MHz + OUT_PP, HIGH}, //Address_A15
	{GPIOD, 14, OUT_50MHz + OUT_PP, HIGH}, //Address_A14
	{GPIOD, 13, OUT_50MHz + OUT_PP, HIGH}, //Address_A13
	{GPIOD, 12, OUT_50MHz + OUT_PP, HIGH}, //Address_A12
	{GPIOD, 11, OUT_50MHz + OUT_PP, HIGH}, //Address_A11
	{GPIOD, 10, OUT_50MHz + OUT_PP, HIGH}, //Address_A10
	{GPIOD, 9, OUT_50MHz + OUT_PP, HIGH},  //Address_A09
	{GPIOD, 8, OUT_50MHz + OUT_PP, HIGH},  //Address_A08
	{GPIOD, 7, OUT_50MHz + OUT_PP, HIGH},  //Address_A07
	{GPIOD, 6, OUT_50MHz + OUT_PP, HIGH},  //Address_A06
	{GPIOD, 5, OUT_50MHz + OUT_PP, HIGH},  //Address_A05
	{GPIOD, 4, OUT_50MHz + OUT_PP, HIGH},  //Address_A04
	{GPIOD, 3, OUT_50MHz + OUT_PP, HIGH},  //Address_A03
	{GPIOD, 2, OUT_50MHz + OUT_PP, HIGH},  //Address_A02
	{GPIOD, 1, OUT_50MHz + OUT_PP, HIGH},  //Address_A01
	{GPIOD, 0, OUT_50MHz + OUT_PP, HIGH},  //Address_A00

	{GPIOE, 15, IN, HIGH}, //Data_D15
	{GPIOE, 14, IN, HIGH}, //Data_D14
	{GPIOE, 13, IN, HIGH}, //Data_D13
	{GPIOE, 12, IN, HIGH}, //Data_D12
	{GPIOE, 11, IN, HIGH}, //Data_D11
	{GPIOE, 10, IN, HIGH}, //Data_D10
	{GPIOE, 9, IN, HIGH},  //Data_D09
	{GPIOE, 8, IN, HIGH},  //Data_D08
	{GPIOE, 7, IN, HIGH},  //Data_D07
	{GPIOE, 6, IN, HIGH},  //Data_D06
	{GPIOE, 5, IN, HIGH},  //Data_D05
	{GPIOE, 4, IN, HIGH},  //Data_D04
	{GPIOE, 3, IN, HIGH},  //Data_D03
	{GPIOE, 2, IN, HIGH},  //Data_D02
	{GPIOE, 1, IN, HIGH},  //Data_D01
	{GPIOE, 0, IN, HIGH},  //Data_D00

	{GPIOA, 8, OUT_50MHz + OUT_PP, HIGH},  //ALE - Address Lock Enable
	{GPIOA, 11, OUT_50MHz + OUT_PP, HIGH}, //LDS - Low Data Set
	{GPIOA, 12, OUT_50MHz + OUT_PP, HIGH}, //UDS - Upper Data Set

	{GPIOC, 0, OUT_50MHz + OUT_PP, HIGH}, //BHE
	{GPIOC, 1, OUT_50MHz + OUT_PP, HIGH}, //AS
	{GPIOC, 2, OUT_50MHz + OUT_PP, HIGH}, //WR - Write
	{GPIOC, 3, OUT_50MHz + OUT_PP, HIGH}, //RD - Read
	{GPIOC, 6, IN, HIGH},				  //SYNC
	{GPIOC, 7, IN, HIGH},				  //RDY - Ready
	{GPIOC, 8, OUT_50MHz + OUT_PP, HIGH}, //MRQ -
	{GPIOC, 9, OUT_50MHz + OUT_PP, HIGH}, //DT/R

	//альтернативная функция
	{GPIOA, 9, OUT_50MHz + OUT_PP, HIGH}, //UART_TX
	{GPIOA, 10, OUT_50MHz + OUT_PP, HIGH} //UART_RX

};

const uint32_t cIO_COUNT = sizeof(IOs) / sizeof(tGPIO_Line);

void IO_Init(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; //Включение тактирования GPIO A
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; //Включение тактирования GPIO B
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN; //Включение тактирования GPIO C
	RCC->APB2ENR |= RCC_APB2ENR_IOPDEN; //Включение тактирования GPIO D
	RCC->APB2ENR |= RCC_APB2ENR_IOPEEN; //Включение тактирования GPIO E
	//RCC->APB2ENR |= RCC_APB2ENR_IOPFEN; //Включение тактирования GPIO F
	//RCC->APB2ENR |= RCC_APB2ENR_IOPGEN; //Включение тактирования GPIO G
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN; //Включение альтернативной функции GPIO

	// В цикле пробегаемся по нашему массиву и конфигурируем.
	for (int Line = 0; Line < cIO_COUNT; Line++)
	{
		IO_ConfigLine(Line, IOs[Line].MODE, IOs[Line].DefState);
	}
}

void IO_ConfigLine(tIOLine Line, uint8_t Mode, uint8_t State)
{
	if (IOs[Line].GPIO_Pin < 8) // Определяем в старший или младший регистр надо запихивать данные.
	{
		IOs[Line].GPIOx->CRL &= ~(0x0F << (IOs[Line].GPIO_Pin * 4)); // Стираем биты
		IOs[Line].GPIOx->CRL |= Mode << (IOs[Line].GPIO_Pin * 4);	 // Вносим нашу битмаску, задвинув ее на нужное место.
	}
	else
	{
		IOs[Line].GPIOx->CRH &= ~(0x0F << ((IOs[Line].GPIO_Pin - 8) * 4)); // Аналогично для старшего регистра.
		IOs[Line].GPIOx->CRH |= Mode << ((IOs[Line].GPIO_Pin - 8) * 4);
	}

	IOs[Line].GPIOx->ODR &= ~(1 << IOs[Line].GPIO_Pin); // Прописываем ODR, устанавливая состояние по умолчанию.
	IOs[Line].GPIOx->ODR |= State << IOs[Line].GPIO_Pin;
}

void IO_SetLine(tIOLine Line, int State)
{
	if (State)
	{
		IOs[Line].GPIOx->BSRR = 1 << IOs[Line].GPIO_Pin;
	}
	else
	{
		IOs[Line].GPIOx->BRR = 1 << IOs[Line].GPIO_Pin;
	}
}

void IO_InvertLine(tIOLine Line)
{
	IOs[Line].GPIOx->ODR ^= 1 << IOs[Line].GPIO_Pin;
}

int IO_GetLine(tIOLine Line)
{
	uint32_t State = 0;
	if (Line < cIO_COUNT)
	{
		State = ((IOs[Line].GPIOx->IDR) & (1 << IOs[Line].GPIO_Pin));
	}
	else
	{
		State = 0;
	}
	return State;
}
