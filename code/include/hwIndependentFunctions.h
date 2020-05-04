/**
 * Функции аппаратно независимые 
*/

#include <ctype.h>
#include <stdint.h>
/**
 * Функция проверки входных символов на HEX формат.
 * Возвращает 0 если HEX формат, либо !0 если строка не содержит HEX символов.
 * Пример: 
 * #include <string.h>
 * hexCheck (iString, strlen(iString));
*/
uint32_t checkHexFormat(const char *str, uint32_t length);

/**
 * Функция проверки адреса на указанный диапазон.
 * Возвращает 0 если iNumber <= iRange, либо !0 если iNumber > iRange
 * Пример:
 * checkRange(iNumber,0xFFFFFF);
*/
uint32_t checkRange(uint32_t iNumber, uint32_t iRange);

/**
 * Функция проверки четности.
 * Возвращает 0 если iNumber четная, либо !0 если iNumber нечетный.
 * Пример:
 * checkParity(iNumber);
*/
uint32_t checkParity(uint32_t iNumber);
