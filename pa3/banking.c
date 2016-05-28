#include "banking.h"

#include "string.h"
#include "ipc.h"
#include "util.h"

void transfer(void *router, local_id src, local_id dst, balance_t value) {
    TransferOrder order;
    // Оформляем передачу денежных средств
    order.s_src = src;
    order.s_dst = dst;
    order.s_amount = value;
    // Инкапсулируем заказ в сообщение
    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = TRANSFER;
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_payload_len = sizeof(order);
    memcpy(msg.s_payload, &order, sizeof(order));

    send(router, src, &msg);
    // Ждем подтверждения
    forever {
        Message answer;
        if(receive(router, dst, &answer) == 0) {
            if(answer.s_header.s_type == ACK)
                break;
        }
    }
}
