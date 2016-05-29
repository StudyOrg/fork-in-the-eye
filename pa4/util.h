#pragma once

#include "stdversion.h"
#include "banking.h"

#define forever while(1)

#define INC(X) (++X)
#define DEC(X) (--X)
#define ZERRO 0
#define ASS(X, Y) (X = Y)
#define EQ(X, Y) (X == Y)

/* Функция, переводящая функцию приема сообщения в сон */
void receive_sleep();
/* Установить статус "неблокирующий" для файла */
void set_nonlock(int);
/* Закрыть неиспользуемые каналы */
int close_unused_pipes(void *);
