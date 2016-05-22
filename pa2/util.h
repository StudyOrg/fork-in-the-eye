#pragma once

/* Функция, переводящая функцию приема сообщения в сон */
int receive_sleep();
/* Установить статус "неблокирующий" для файла */
void set_nonlock(int);
/* Закрыть неиспользуемые каналы */
int close_unused_pipes(void *);
