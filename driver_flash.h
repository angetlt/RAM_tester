void flash_lock(void);

void flash_unlock(void);

static uint8_t flash_ready(void);

//Функция стирает ВСЕ страницы. При её вызове прошивка самоуничтожается
void flash_erase_all_pages(void);

//Функция стирает одну страницу. В качестве адреса можно использовать любой
//принадлежащий диапазону адресов той странице которую нужно очистить.
void flash_erase_page(uint32_t);

void flash_write(uint32_t, uint32_t);
