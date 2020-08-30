/**
 * Функции инициализации периферийных устройств микроконтроллера STM32F103VE
 * Функции аппаратнозависимые
*/

/**
 * Функция инициализации USART1 микроконтроллера
 * Для использования вызвать функцию initUSART1_19200();
*/
void initUSART1_19200(void);

/**
 * Функция инициализации прерваний микроконтроллера
 * Для использования вызвать функцию initIRQHandler();
*/
void initIRQHandler(void);

/**
 * Функция инициализации прерваний микроконтроллера
 * Для использования вызвать функцию initIRQHandler();
*/
void initTIM1_msTimer(void);

/**
 * Функция инициализации прерваний микроконтроллера
 * Для использования вызвать функцию initIRQHandler();
*/
void initTIM3CH2_externalCounter(void);