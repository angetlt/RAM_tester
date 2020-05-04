/*FLASH MACROS*/
#define FLASH_CONFIG_START_ADDRESS ((uint32_t)0x0800F000)
#define FLASH_KEY_WORD ((uint32_t)0x248F135B)
#define FLASH_BYTE_STEP 0x4
#define FLASH_STEP 0x0F0 //4 bytes
#define FLASH_PAGE 0x080 //128 pages

/*UART RECIEVE BUFFER*/
#define UART_RECIEVE_BUFFER 40
#define ADDRESS_RANGE 0xFFFFFF

typedef enum
{
    EMPTY = 0,
    WRITE = 1,
    READ,
    RAM,
    ROM,
    FD
} CommandType;

typedef enum
{
    BLANK = 0,
    MEM = 1,
    IO
} SpaceAddress;

typedef struct
{
    CommandType Command;
    SpaceAddress AttributeSpaceAddress;
    uint32_t Address;
    uint32_t Start_Address;
    uint32_t Stop_Address;
    uint32_t IncrementAddress;
    uint16_t Data;
    uint16_t RepeatNumber;
} Command;

/*Тип сообщения UART*/
typedef struct
{
    char *Command;
    char *Address;
    char *Start_Address;
    char *Stop_Address;
    char *IncrementAddress;
    char *Data;
    char *RepeatNumber;
} Message;

typedef struct
{
    uint16_t DataBusSize; /*DataBusSize - режим передачи данных 8 бит, 16 бит*/
    uint8_t AlignMode;    /*AlignMode - Синхронизация по адресу или по данным*/
    uint8_t reservedConf; /*Резервные 8 бит настройки*/
} Config;

typedef struct
{
    uint32_t write_count; /*Количество сохранений в ячейку FLASH памяти*/
    uint32_t hash;        /*Ключевое слово для доступа к FLASH*/
} flash;

/**
 * Функция 
 * Для работы функции необходимо включить библиотеку 
 * Пример: 
*/
void ReadCommand(void);
void DefaultCommand(void);
void HelpCommand(void);
void SaveCommand(void);

void WriteCommand(void);
void ReadCommand(void);

void parseUARTMessage(char *uart_message);
void RecieveMessage(char data);
void USART1_IRQHandler(void);

void initDeviceConfig(void);

/**
 * Функция вывода в терминал ошибки дешифрования команды
*/
void errorType(uint32_t err_number);

/**
 * Функция отправки в терминал текстового сообщения по UART
*/
void SendMessage(char *str);

/**
 * Функция задержки на указанное количество мс
*/
void delay_ms(uint16_t Count_ms);

/**
 * Функция сброса текущей команды CurrentCommand в состояние EMPTY
 */
void resetCC(void);

/**
 * Функция копирования CurrentCommand в LastCommand
 */
void copyCCtoLC(void);

/**
 * Функция выполнения текущей команды (function execute Current Command)
 * Заканчивается функция перемещением CurrentCommand в LastCommand
 * и обнулением CurrentCommand
 * 
*/
void taskExecCommand(void);