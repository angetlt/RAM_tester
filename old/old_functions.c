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

void debugUARTMessage(void)
{
    char rMessage1[UART_RECIEVE_BUFFER];
    char rMessage2[UART_RECIEVE_BUFFER];
    char rMessage3[UART_RECIEVE_BUFFER];

    //функция переводит число в строку
    snprintf(rMessage1, UART_RECIEVE_BUFFER, "%X", DeviceConfiguration.AlignMode);
    snprintf(rMessage2, UART_RECIEVE_BUFFER, "%X", DeviceConfiguration.ModeX);
    snprintf(rMessage3, UART_RECIEVE_BUFFER, "%X", FlashControl.write_count);

    SendMessage(rMessage1);
    SendMessage(rMessage2);
    SendMessage(rMessage3);
}

void DeviceOSC_start(void)
{
    TIM3->CR1 |= TIM_CR1_CEN;
    SendMessage("<----------");
    SendMessage("OSC started");
}

void DeviceOSC_stop(void)
{
    TIM3->CR1 &= ~(TIM_CR1_CEN);
    SendMessage("<----------");
    SendMessage("OSC stoped");
}

//LDS,UDS,ALE, MRQ, DT/R Init
void SignalInit(void)
{
    //PA08 - ALE
    GPIOA->CRH &= ~GPIO_CRH_CNF8;
    GPIOA->CRH |= GPIO_CRH_MODE8;
    //PA11 - LDS
    GPIOA->CRH &= ~GPIO_CRH_CNF11;
    GPIOA->CRH |= GPIO_CRH_MODE11;
    //PA12 - UDS
    GPIOA->CRH &= ~GPIO_CRH_CNF12;
    GPIOA->CRH |= GPIO_CRH_MODE12;
    //PC08 - MRQ
    GPIOC->CRH &= ~GPIO_CRH_CNF8;
    GPIOC->CRH |= GPIO_CRH_MODE8;
    //PC09 - DT/R
    GPIOC->CRH &= ~GPIO_CRH_CNF9;
    GPIOC->CRH |= GPIO_CRH_MODE9;
}

void DOCommand(void)
{
    switch (CurrentCommand.Command)
    {
    case WRITE:
        WriteCommand();
        break;
    case READ:
        ReadCommand();
        break;
    case RAM:
        break;
    case ROM:
        break;
    default:
        SendMessage("default");
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

void TIM1_init_OSC(void)
{
    /*PB0 - настроен в режим выхода */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; //clock PORT_B
    GPIOB->CRL &= ~GPIO_CRL_CNF0;
    GPIOB->CRL |= GPIO_CRL_MODE0;

    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; //clock TIM1
    /*Configure CR1: ARPE=1; OPM=1; DIR=0; URS=1;*/
    TIM1->CR1 |= TIM_CR1_ARPE; //Use ARR
    TIM1->CR1 &= ~TIM_CR1_OPM; //DO NOT STOP counting after event
    TIM1->CR1 &= ~TIM_CR1_DIR; //upcounting
    TIM1->CR1 |= TIM_CR1_URS;
    TIM1->DIER |= TIM_DIER_UIE; //interrupt after overflow
    TIM1->PSC = OSC_1MHz;       //prescaller PLLCLK
    TIM1->ARR = 7;
    TIM1->CR1 &= ~TIM_CR1_CEN; //STOP timer
    TIM1->CNT = 0x00;          //TIM2 Counter = 0
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
    TIM2->PSC = 1;              //prescaller PLLCLK
    TIM2->ARR = 1;
    TIM2->CR1 &= ~TIM_CR1_CEN; //STOP timer
    TIM2->CNT = 0x00;          //TIM2 Counter = 0
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
    TIM3->ARR = TIM3_ARR;     //ARR counting
    TIM3->CCR3 = TIM3_CCR3;

    TIM3->CR1 &= ~(TIM_CR1_CEN);
}

#include "stm32f10x_conf.h"
#include "string.h"
#include "stdio.h"

void USART1_Send(char chr)
{
    while (!(USART1->SR & USART_SR_TC))
        ;
    USART1->DR = chr;
}

void USART1_Send_String(char *str)
{
    int i = 0;
    while (str[i])
        USART1_Send(str[i++]);
}

char *prompt = "
               > ";

               int
               main()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // Включаем модули USART1 и GPIOA, а также включаем альтернативные функции выходов
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
    // Контакт PA9 будет выходом с альтернативной функцией, а контакт PA10 - входом
    GPIOA->CRH &= !GPIO_CRH_CNF9;
    GPIOA->CRH |= GPIO_CRH_CNF9_1 | GPIO_CRH_MODE9_0 | GPIO_CRH_MODE9_1 | GPIO_CRH_CNF10_0;
    // Настраиваем регистр тактирования, скорость составит 9600 бод (при тактовой частоте 24 МГц)
    USART1->BRR = 0x1D4C; //1302;//0x1458;//0x9C4;
    // Выключаем TxD и RxD USART
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;
    // Запускаем модуль USART
    USART1->CR1 |= USART_CR1_UE;
    // Разрешаем прерывание по приёму информации с RxD
    USART1->CR1 |= USART_CR1_RXNEIE;
    // Назначаем обработчик для всех прерываний от USART1
    NVIC_EnableIRQ(USART1_IRQn);
    // Приветствие
USART1_Send_String("Start
");
USART1_Send_String(prompt);

// Бесконечный цикл
while(1);
}

int LED1, LED2;

void processCommand(char *cmd)
{
    // Обрабатываем запрос
    char answer[20] = "";
    // Команда echo
    if (strncmp(cmd, "echo ", 5) == 0)
        // отправляем строку запроса, начиная с 5 символа (т.е. после "echo ") - работа с указателями.
        USART1_Send_String(cmd + 5);

    // Запрос состояния светодиодов
    if (strncmp(cmd, "get leds", 8) == 0)
    {
        sprintf(answer, "LEDs state: %s, %s", (LED1 ? "On" : "Off"), (LED2 ? "On" : "Off"));
        USART1_Send_String(answer);
    }

    // Установка состояния светодиодов
    if (strncmp(cmd, "set leds", 8) == 0)
    {
        sscanf(cmd + 9, "%d, %d", &LED1, &LED2); // берём командную строку, начиная с 9 символа и читаем в ней два integer
        GPIO_WriteBit(GPIOC, GPIO_Pin_8, (LED1 ? Bit_SET : Bit_RESET));
        GPIO_WriteBit(GPIOC, GPIO_Pin_9, (LED2 ? Bit_SET : Bit_RESET));
        sprintf(answer, "LEDs state: %s, %s", (LED1 ? "On" : "Off"), (LED2 ? "On" : "Off"));
        USART1_Send_String(answer);
    }

    // Справка по существующим функциям
    if (strncmp(cmd, "help", 4) == 0)
USART1_Send_String("Help on our LED controller
  \echo [text] - print text
  get leds - get LEDs state
  \
set leds [LED1] [LED2] - set LEDs state, 0(off)/1(on)
  \help - this help");

USART1_Send_String(prompt);
}

char received[50], rec_len = 0;

// Обработчик всех прерываний от USART1
void USART1_IRQHandler(void)
{
    // Выясняем, какое именно событие вызвало прерывание. Если это приём байта в RxD - обрабатываем.
    if (USART1->SR & USART_SR_RXNE)
    {
        // Сбрасываем флаг прерывания
        USART1->SR &= ~USART_SR_RXNE;
        short unsigned int dat = USART1->DR;
        // Обработка запроса
        if (dat == 13)
        {
            USART1_Send_String(prompt);
            received[rec_len++] = 0; // делаем нуль-терминированную строку
            // отправляем принятую команду на обработку
            processCommand(received);
            // очищаем буфер приёма
            rec_len = 0;
        }
        else
        {
            // Эхо, чтобы не печатать вслепую
            USART1_Send(dat);
            // Складываем символ в приёмный буфер
            received[rec_len++] = dat;
        }
    }
}
