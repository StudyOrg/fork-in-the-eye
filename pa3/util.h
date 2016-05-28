#pragma once

#include "stdversion.h"
#include "banking.h"

#define forever while(1)

/* Функция, переводящая функцию приема сообщения в сон */
void receive_sleep();
/* Установить статус "неблокирующий" для файла */
void set_nonlock(int);
/* Закрыть неиспользуемые каналы */
int close_unused_pipes(void *);

Message get_stop_message();

Message get_history_message(BalanceHistory);

Message get_ack_message();
