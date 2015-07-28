#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"
#include "debug.h"

#include "message_queue.h"

void MessageQueue_clear(struct MessageQueue *msgq)
{
	DEBUG("enter MessageQueue_clear");
	struct Message *msg;
	struct Message *prev = NULL;

	STAILQ_FOREACH(msg, &msgq->messages, next) {
		if ((prev != NULL) && (prev->mem == HEAP_MEM))
			os_free(prev->data);

		os_free(prev);
		prev = msg;
	}

	if ((prev != NULL) && (prev->mem == HEAP_MEM))
		os_free(prev->data);

	os_free(prev);

	STAILQ_INIT(&msgq->messages);
	msgq->count = 0;

	DEBUG("exit MessageQueue_clear");
}

int MessageQueue_push(struct MessageQueue *msgq, struct espconn *conn, void *data, uint16 len, enum Memtype mem)
{
	DEBUG("enter MessageQueue_push");
	struct Message *msg;

	msg = (struct Message *)os_zalloc(sizeof *msg);

	if (msg == NULL) {
		DEBUG("exit MessageQueue_push");
		return -1;
	}

	msg->conn = conn;
	msg->data = data;
	msg->len = len;
	msg->mem = mem;

	STAILQ_INSERT_TAIL(&msgq->messages, msg, next);
	++msgq->count;

	DEBUG("exit MessageQueue_push");
	return 0;
}

int MessageQueue_unshift(struct MessageQueue *msgq, struct espconn **connp, void **datap, uint16 *lenp, enum Memtype *memp)
{
	DEBUG("enter MessageQueue_unshift");
	struct Message *msg;

	if (STAILQ_EMPTY(&msgq->messages)) {
		DEBUG("exit MessageQueue_unshift");
		return 1;
	}

	msg = STAILQ_FIRST(&msgq->messages);
	STAILQ_REMOVE_HEAD(&msgq->messages, next);

	if (msg == NULL) {
		DEBUG("exit MessageQueue_unshift");
		return -1;
	}

	--msgq->count;
	*connp = msg->conn;
	*datap = msg->data;
	*lenp = msg->len;
	*memp = msg->mem;

	os_free(msg);
	DEBUG("exit MessageQueue_unshift");
	return 0;
}

bool MessageQueue_empty(const struct MessageQueue *msgq)
{
	DEBUG("enter MessageQueue_empty");

	if (STAILQ_EMPTY(&msgq->messages)) {
		DEBUG("exit MessageQueue_empty");
		return true;
	}

	DEBUG("exit MessageQueue_empty");
	return false;
}
