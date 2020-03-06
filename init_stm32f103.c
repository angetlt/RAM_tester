// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <stm32f10x.h>
/*TIMERS PRESCALLER*/
#define US (SystemCoreClock / 1000000) - 1
#define MS (SystemCoreClock / 2000) - 1

#define OSC_1MHz (72000000 / 72000000) - 1
#define OSC_2MHz (SystemCoreClock / 2000000) - 1
#define OSC_4NHz (SystemCoreClock / 4000000) - 1

/*PWM SETTINGS
*	Calculate:
*	TIM3_PSC_OSC_set = (SystemCoreClock/2)/N-1
*	SystemCoreClock/2 = 36MHz
*	N - prescale 36MHz
*	1MHz -> N=36
*	2MHz -> N=18
*/
#define TIM3_PSC_OSC 0 //multiple 72MHz
#define TIM3_ARR 71
#define TIM3_CCR3 36

void TIM1_Init_ms_timer(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; //clock TIM1
	//Configure CR1: ARPE=1; OPM=1; DIR=0; URS=1;
	TIM1->CR1 |= TIM_CR1_ARPE; //Use ARR
	TIM1->CR1 &= ~TIM_CR1_OPM; //DO NOT STOP counting after event
	TIM1->CR1 &= ~TIM_CR1_DIR; //upcounting
	TIM1->CR1 |= TIM_CR1_URS;
	TIM1->DIER = 0x00;		   //no interrupt after overflow
	TIM1->PSC = MS;			   //prescaller PLLCLK
	TIM1->CR1 &= ~TIM_CR1_CEN; //STOP timer
	TIM1->CNT = 0x00;		   //TIM1 Counter = 0
}

void TIM1_init_OSC(void)
{
	/**PB0 - настроен в режим выхода */
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; //clock PORT_B
	GPIOB->CRL &= ~GPIO_CRL_CNF0;
	GPIOB->CRL |= GPIO_CRL_MODE0;

	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; //clock TIM1
	//Configure CR1: ARPE=1; OPM=1; DIR=0; URS=1;
	TIM1->CR1 |= TIM_CR1_ARPE; //Use ARR
	TIM1->CR1 &= ~TIM_CR1_OPM; //DO NOT STOP counting after event
	TIM1->CR1 &= ~TIM_CR1_DIR; //upcounting
	TIM1->CR1 |= TIM_CR1_URS;
	TIM1->DIER |= TIM_DIER_UIE; //interrupt after overflow
	TIM1->PSC = OSC_1MHz;		//prescaller PLLCLK
	TIM1->ARR = 7;
	TIM1->CR1 &= ~TIM_CR1_CEN; //STOP timer
	TIM1->CNT = 0x00;		   //TIM2 Counter = 0
}

void TIM2_init_OSC(void)
{
	/**PB0 - настроен в режим выхода */
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; //clock PORT_B

	GPIOB->CRL &= ~GPIO_CRL_CNF0;
	GPIOB->CRL |= GPIO_CRL_MODE0;

	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; //clock TIM2
	//Configure CR1: ARPE=1; OPM=1; DIR=0; URS=1;
	TIM2->CR1 |= TIM_CR1_ARPE; //Use ARR
	TIM2->CR1 &= ~TIM_CR1_OPM; //DO NOT STOP counting after event
	TIM2->CR1 &= ~TIM_CR1_DIR; //upcounting
	TIM2->CR1 |= TIM_CR1_URS;
	TIM2->DIER |= TIM_DIER_UIE; //interrupt after overflow
	TIM2->PSC = 1;				//prescaller PLLCLK
	TIM2->ARR = 1;
	TIM2->CR1 &= ~TIM_CR1_CEN; //STOP timer
	TIM2->CNT = 0x00;		   //TIM2 Counter = 0
}

void TIM3_Init_CH3OSC_PWM(void)
{
	/**PB0 - настроен в режим выхода */
	GPIOB->CRL &= ~GPIO_CRL_CNF0_0;
	GPIOB->CRL |= GPIO_CRL_CNF0_1;
	GPIOB->CRL |= GPIO_CRL_MODE0;

	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN; ///Включили тактирование TIM3

	TIM3->CR1 |= TIM_CR1_ARPE; ///Используем Auto Reload Register
	TIM3->CCER |= TIM_CCER_CC3E;
	TIM3->CCMR2 |= TIM_CCMR2_OC3M;

	TIM3->PSC = TIM3_PSC_OSC; //Prescaller = 0
	TIM3->ARR = TIM3_ARR;	 //ARR counting
	TIM3->CCR3 = TIM3_CCR3;

	TIM3->CR1 &= ~(TIM_CR1_CEN);
}

/*
*	Calculate:
*	USARTDIV = (F/baudrate)/16
*	F - frequency APB2ENR
*	baudrate - 19200
*/
void USARTInit(void)
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

void IRQHandlerInit(void)
{
	__enable_irq();
	NVIC_EnableIRQ(USART1_IRQn);
	NVIC_EnableIRQ(TIM1_UP_IRQn);
	//NVIC_EnableIRQ(TIM2_IRQn);
	NVIC_SetPriority(USART1_IRQn, 0);
	USART1->CR1 |= USART_CR1_RXNEIE;
}
