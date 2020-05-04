#include <stm32f10x.h>

/*TIMERS PRESCALLER*/
#define US (SystemCoreClock / 1000000) - 1
#define MS (SystemCoreClock / 2000) - 1
#define EXTERNAL_OSC_CYCLE 7
#define PRESCALLER_NOT_USE 0

/**
 * Функция инициализации USART1 микроконтроллера
 * Расчет:
 * USARTDIV = (F/baudrate)/16
 * F - Частота шины APB2ENR
 * baudrate - 19200
*/
void initUSART1_19200(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;					   //USART1 clock enable
	USART1->BRR = 0xEA6;									   //Baudrate 19200 on SysCLK_72MHz AHB_72MHZ APB2_72MHz
	USART1->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE; //USART1 ON, TX ON, RX ON
	//TX
	GPIOA->CRH &= ~GPIO_CRH_CNF9; //clear
	GPIOA->CRH |= GPIO_CRH_CNF9_1;
	GPIOA->CRH |= GPIO_CRH_MODE9_0;
	//RX
	GPIOA->CRH &= ~GPIO_CRH_CNF10; //clear
	GPIOA->CRH |= GPIO_CRH_CNF10_1;
	GPIOA->CRH &= ~GPIO_CRH_MODE10;
	USART1->SR &= ~USART_SR_TC; //clear TX empty flag
}

/**
 * Функция инициализации прерваний микроконтроллера
*/
void initIRQHandler(void)
{
	__enable_irq();					  //Глобальное разрешение прерываний
	NVIC_EnableIRQ(USART1_IRQn);	  //Прерывания по приему байта USART1
	NVIC_SetPriority(USART1_IRQn, 0); //Установка приоритета 0 (высший) прерываниям по USART1
	USART1->CR1 |= USART_CR1_RXNEIE;  //Активация прерывания USART1
}

/**
 * Функция инициализации таймера TIM1 микроконтроллера как милисекундный счетчик.
 * Один счет таймера равен одной милисекунде
*/
void initTIM1_msTimer(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; //Включение тактирования TIM1
	TIM1->CR1 |= TIM_CR1_ARPE;			//Использование ARR - предельное значение счета
	TIM1->CR1 &= ~TIM_CR1_OPM;			//Не останавливать счет после обнуления
	TIM1->CR1 &= ~TIM_CR1_DIR;			//Счет вверх (идет от меньшего к большему)
	TIM1->CR1 |= TIM_CR1_URS;
	TIM1->DIER = 0x0;		   //Не вызывать прерывание при достижении ARR
	TIM1->PSC = MS;			   //prescaller PLLCLK
	TIM1->CR1 &= ~TIM_CR1_CEN; //STOP timer
	TIM1->CNT = 0x0;		   //TIM1 Counter = 0
}

/**
 * Функция инициализации таймера TIM3_CH2 микроконтроллера как счетчик внешних импульсов.
 * Один счет таймера равен одному тику внешнего генератора
 * Частота тактирования контроллера должна быть как минимум в 2 раза выше измеряемой частоты,
 * т.к. таймеры в STM32F103 синхронные.
 * TIM3 тактируется по шине APB1. Но счет видет внешних импульсов.
*/
void initTIM3CH2_externalCounter(void)
{
	AFIO->MAPR |= AFIO_MAPR_TIM3_REMAP_PARTIALREMAP; //Ремапим альтернативную функцию TIM3
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;				 //Включение тактирования TIM3

	TIM3->CR1 |= TIM_CR1_ARPE;		//Использование ARR - предельное значение счета
	TIM3->CR1 &= ~TIM_CR1_OPM;		//DO NOT STOP counting after event
	TIM3->DIER = 0x0;				//Не вызывать прерывание при достижении ARR
	TIM3->PSC = PRESCALLER_NOT_USE; //Не использовать предделитель

	/*Записываем в регистр CCMR2=01*/
	TIM3->CCMR1 |= TIM_CCMR1_CC2S_0;  //Нулевой бит
	TIM3->CCMR1 &= ~TIM_CCMR1_CC2S_1; //Первый бит
	TIM3->CCMR1 |= TIM_CCMR1_IC2F_3;  //Используем фильтр - пишем в регистр CCMR2 IC3F = 1000
	TIM3->CCER &= ~TIM_CCER_CC2P;	  //Счет вверх по положительному фронту
	TIM3->SMCR |= TIM_SMCR_SMS;		  //Устанавливаем внешний источник тактирования счетчика
	/*Записываем в регистр TS=110*/
	TIM3->SMCR &= ~TIM_SMCR_TS_0;
	TIM3->SMCR |= TIM_SMCR_TS_1;
	TIM3->SMCR |= TIM_SMCR_TS_2;

	TIM3->CR1 |= TIM_CR1_URS;		//Обновление (используется для принятия настроек)
	TIM3->CNT &= ~(0xFFFF);			//Начальное значение равно нулю
	TIM3->ARR = EXTERNAL_OSC_CYCLE; //Ограничение счетчика значением 7 (цикл из 8 импульсов внешнего генератора OSC)
	TIM3->CR1 &= ~TIM_CR1_CEN;		//Остановили счетчик
	TIM3->CR1 |= TIM_CR1_URS;		//Обновление (используется для принятия настроек)
	TIM3->CR1 |= TIM_CR1_CEN;		//Запустили счетчик
}
