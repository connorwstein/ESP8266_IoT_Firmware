#ifndef MESSAGE_QUEUE
#define MESSAGE_QUEUE

#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"
#include "queue.h"

#define MESSAGE_QUEUE_INITIALIZER(msgq) {.messages = STAILQ_HEAD_INITIALIZER(msgq.messages), .count = 0}

enum Memtype {
	STATIC_MEM,	/* Static memory. Do not free. */
	HEAP_MEM	/* Heap memory. Use os_free(). */
};

struct Message {
	struct espconn *conn;
	void *data;
	uint16 len;
	enum Memtype mem;
	STAILQ_ENTRY(Message) next;
};

struct MessageQueue {
	STAILQ_HEAD(_messages, Message) messages;
	uint16 count;
};

void MessageQueue_clear(struct MessageQueue *msgq);

int MessageQueue_push(struct MessageQueue *msgq, struct espconn *conn, void *data, uint16 len, enum Memtype mem);
int MessageQueue_unshift(struct MessageQueue *msgq, struct espconn **connp, void **datap, uint16 *lenp, enum Memtype *memp);

bool MessageQueue_empty(const struct MessageQueue *msgq);

#endif
