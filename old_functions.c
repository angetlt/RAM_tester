///**OSC defines */
//#define TIM3_PSC_OSC 0 //multiple 72MHz
//#define TIM3_ARR 35
//#define TIM3_CCR3 18
/**SET_PINS defines*/
#define ALE_SET GPIOA->BSRR = GPIO_BSRR_BS8
#define AS_SET GPIOC->BSRR = GPIO_BSRR_BS1
#define LDS_SET GPIOA->BSRR = GPIO_BSRR_BS11
#define UDS_SET GPIOA->BSRR = GPIO_BSRR_BS12
#define BHE_SET GPIOC->BSRR = GPIO_BSRR_BS0
#define WR_SET GPIOC->BSRR = GPIO_BSRR_BS2
#define RD_SET GPIOC->BSRR = GPIO_BSRR_BS3
#define MRQ_SET GPIOC->BSRR = GPIO_BSRR_BS8
#define DTR_SET GPIOC->BSRR = GPIO_BSRR_BS9
/**RESET_PINS defines*/
#define ALE_RESET GPIOA->BSRR = GPIO_BSRR_BR8
#define AS_RESET GPIOC->BSRR = GPIO_BSRR_BR1
#define LDS_RESET GPIOA->BSRR = GPIO_BSRR_BR11
#define UDS_RESET GPIOA->BSRR = GPIO_BSRR_BR12
#define BHE_RESET GPIOC->BSRR = GPIO_BSRR_BR0
#define WR_RESET GPIOC->BSRR = GPIO_BSRR_BR2
#define RD_RESET GPIOC->BSRR = GPIO_BSRR_BR3
#define MRQ_RESET GPIOC->BSRR = GPIO_BSRR_BR8
#define DTR_RESET GPIOC->BSRR = GPIO_BSRR_BR9

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

void RepeatCommand(void)
{
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