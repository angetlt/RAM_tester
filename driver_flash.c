// This is an open source non-commercial project. Dear PVS-Studio, please check it.

/**
@brief Драйвер записи и чтения FLASH памяти микроконтроллера STM32F103
Использование:
1. Перед записью память должна быть стёрта.
Память считается очищенной когда все биты по адресу установлены в единицу

2. Стирается память не побайтно, а целыми страницами \n
32 pages of 1 kByte:\n
STM32F103x4 - low-density device\n
STM32F103x6 - low-density device\n

128 pages of 1 kByte:
STM32F103x8 - medium-density device
STM32F103xB - medium-density device

256 pages of 2 kByte:
STM32F103xC - high-density device
STM32F103xD - high-density device
STM32F103xE - high-density device

3. Для контроля окончания операции надо использовать не бит BSY, а бит EOP.
*/

#include <stm32f10x.h>
#define FLASH_KEY1 ((uint32_t)0x45670123)
#define FLASH_KEY2 ((uint32_t)0xCDEF89AB)

void flash_lock(void)
{
	FLASH->CR |= FLASH_CR_LOCK;
}

void flash_unlock(void)
{
	FLASH->KEYR = FLASH_KEY1;
	FLASH->KEYR = FLASH_KEY2;
}

static uint8_t flash_ready(void)
{
	return !(FLASH->SR & FLASH_SR_BSY);
}

//Функция стирает ВСЕ страницы. При её вызове прошивка самоуничтожается
void flash_erase_all_pages(void)
{
	FLASH->CR |= FLASH_CR_MER;  //Устанавливаем бит стирания ВСЕХ страниц
	FLASH->CR |= FLASH_CR_STRT; //Начать стирание
	while (!flash_ready())
		; // Ожидание готовности
	FLASH->CR &= FLASH_CR_MER;
}

//Функция стирает одну страницу. В качестве адреса можно использовать любой
//принадлежащий диапазону адресов той странице которую нужно очистить.
void flash_erase_page(uint32_t address)
{
	FLASH->CR |= FLASH_CR_PER;  //Устанавливаем бит стирания одной страницы
	FLASH->AR = address;		// Задаем её адрес
	FLASH->CR |= FLASH_CR_STRT; // Запускаем стирание
	while (!flash_ready())
		;
	FLASH->CR &= ~FLASH_CR_PER; //Сбрасываем бит обратно
}

void flash_write(uint32_t address, uint32_t data)
{
	FLASH->CR |= FLASH_CR_PG; //Разрешаем программирование флеша
	while (!flash_ready())
		; //Ожидаем готовности флеша к записи

	*(__IO uint16_t *)address = (uint16_t)data; //Пишем младшие 2 бата
	while (!flash_ready())
		;

	address += 2;
	data >>= 16;
	*(__IO uint16_t *)address = (uint16_t)data; //Пишем старшие 2 байта
	while (!flash_ready())
		;

	FLASH->CR &= ~(FLASH_CR_PG); //Запрещаем программирование флеша
}
