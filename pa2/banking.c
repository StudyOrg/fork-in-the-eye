#include "banking.h"

#include "string.h"
#include "ipc.h"

void transfer(void * parent_data, local_id src, local_id dst, balance_t amount) {
    TransferOrder order;
    order.s_src = src;
    order.s_dst = dst;
    order.s_amount = amount;

    Message msg, resMsg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = TRANSFER;
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_payload_len = sizeof(order);
    memcpy(msg.s_payload, &order, sizeof(order));

    send(parent_data, src, &msg);

    while(1) {
        if(receive(parent_data, dst, &resMsg) == 0) {
            if(resMsg.s_header.s_type == ACK)
                break;
        }
    }
}
