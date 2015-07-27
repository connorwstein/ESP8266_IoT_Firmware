#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"
#include "debug.h"

#include "connection_table.h"

#define TCP_MAX_PACKET_SIZE 1460

struct Message {
	void *data;
	uint16 len;
	enum Memtype mem;
	STAILQ_ENTRY(Message) next;
};

struct MessageQueue {
	STAILQ_HEAD(_messages, Message) messages;
	uint16 count;
};

struct ConnectionInfo {
	enum espconn_type type;
	uint16 remote_port;
	uint8 remote_ip[4];
};

struct ConnectionTableRecord {
	struct ConnectionInfo info;
	struct MessageQueue sendq;
	struct MessageQueue recvq;
	SLIST_ENTRY(ConnectionTableRecord) next;
};

struct ConnectionTable {
	SLIST_HEAD(_entries, ConnectionTableRecord) entries;
	uint16 count;
};

static bool is_same_conn(const struct ConnectionInfo *a, const struct espconn *b)
{
	DEBUG("enter is_same_conn");
	ets_uart_printf("a = %p, b = %p\n", a, b);

	if (a->type != b->type) {
		DEBUG("exit is_same_conn");
		return false;
	}

	DEBUG("after type check");

	switch (b->type) {
		case ESPCONN_TCP:
			DEBUG("in ESPCONN_TCP");
			ets_uart_printf("DEBUG: b->proto.tcp = %p\n", b->proto.tcp);

			if (a->remote_port != b->proto.tcp->remote_port) {
				DEBUG("exit is_same_conn");
				return false;
			}

			DEBUG("Comparing IP's");
			if (os_memcmp(a->remote_ip, b->proto.tcp->remote_ip, 4) != 0) {
				DEBUG("exit is_same_conn");
				return false;
			}

			break;

		case ESPCONN_UDP:
			if (a->remote_port != b->proto.udp->remote_port) {
				DEBUG("exit is_same_conn");
				return false;
			}

			if (os_memcmp(a->remote_ip, b->proto.tcp->remote_ip, 4) != 0) {
				DEBUG("exit is_same_conn");
				return false;
			}

			break;

		default:
			DEBUG("exit is_same_conn");
			return false;
	}

	DEBUG("exit is_same_conn");
	return true;
}

static void MessageQueue_destroy(struct MessageQueue *msgq)
{
	DEBUG("enter MessageQueue_destroy");
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
	DEBUG("exit MessageQueue_destroy");
}

static int MessageQueue_push(struct MessageQueue *msgq, void *data, uint16 len, enum Memtype mem)
{
	DEBUG("enter MessageQueue_push");
	struct Message *msg;

	if (len == 0) {
		DEBUG("exit MessageQueue_push");
		return 1;
	}

	while (len > TCP_MAX_PACKET_SIZE) {
		msg = (struct Message *)os_zalloc(sizeof *msg);

		if (msg == NULL) {
			DEBUG("exit MessageQueue_push");
			return -1;
		}

		msg->data = data;
		msg->len = TCP_MAX_PACKET_SIZE;
		msg->mem = mem;

		STAILQ_INSERT_TAIL(&msgq->messages, msg, next);
		++msgq->count;
		data = (uint8 *)data + TCP_MAX_PACKET_SIZE;
		len -= TCP_MAX_PACKET_SIZE;
	}

	msg = (struct Message *)os_zalloc(sizeof *msg);

	if (msg == NULL) {
		DEBUG("exit MessageQueue_push");
		return -1;
	}

	msg->data = data;
	msg->len = len;
	msg->mem = mem;

	STAILQ_INSERT_TAIL(&msgq->messages, msg, next);
	++msgq->count;

	DEBUG("exit MessageQueue_push");
	return 0;
}

static int MessageQueue_unshift(struct MessageQueue *msgq, void **datap, uint16 *lenp, enum Memtype *memp)
{
	DEBUG("enter MessageQueue_unshift");
	struct Message *msg;

	ets_uart_printf("DEBUG: msgq = %p, datap = %p, lenp = %p, memp = %p\n", msgq, datap, lenp, memp);

	if (STAILQ_EMPTY(&msgq->messages)) {
		DEBUG("exit MessageQueue_unshift");
		return 1;
	}

	DEBUG("after STAILQ_EMPTY");
	msg = STAILQ_FIRST(&msgq->messages);
	STAILQ_REMOVE_HEAD(&msgq->messages, next);

	if (msg == NULL) {
		DEBUG("exit MessageQueue_unshift");
		return -1;
	}

	DEBUG("after REMOVE_HEAD");
	--msgq->count;
	*datap = msg->data;
	*lenp = msg->len;
	*memp = msg->mem;

	os_free(msg);
	DEBUG("exit MessageQueue_unshift");
	return 0;
}

static bool MessageQueue_empty(const struct MessageQueue *msgq)
{
	DEBUG("enter MessageQueue_empty");

	if (STAILQ_EMPTY(&msgq->messages)) {
		DEBUG("exit MessageQueue_empty");
		return true;
	}

	DEBUG("exit MessageQueue_empty");
	return false;
}

static struct ConnectionTableRecord *ConnectionTableRecord_create(const struct espconn *conn)
{
	DEBUG("enter ConnectionTableRecord_create");
	struct ConnectionTableRecord *record;

	record = (struct ConnectionTableRecord *)os_zalloc(sizeof *record);

	if (record == NULL) {
		DEBUG("exit ConnectionTableRecord_create");
		return NULL;
	}

	record->info.type = conn->type;

	switch (conn->type) {
		case ESPCONN_TCP:
			record->info.remote_port = conn->proto.tcp->remote_port;
			os_memcpy(record->info.remote_ip, conn->proto.tcp->remote_ip, 4);
			break;

		case ESPCONN_UDP:
			record->info.remote_port = conn->proto.udp->remote_port;
			os_memcpy(record->info.remote_ip, conn->proto.udp->remote_ip, 4);
			break;

		default:
			break;	/* XXX give an error? */
	}

	STAILQ_INIT(&record->sendq.messages);
	STAILQ_INIT(&record->recvq.messages);
	record->sendq.count = 0;
	record->recvq.count = 0;

	DEBUG("exit ConnectionTableRecord_create");
	return record;
}

static void ConnectionTableRecord_destroy(struct ConnectionTableRecord *record)
{
	DEBUG("enter ConnectionTableRecord_destroy");
	MessageQueue_destroy(&record->sendq);
	MessageQueue_destroy(&record->recvq);
	os_free(record);
	DEBUG("exit ConnectionTableRecord_destroy");
}

struct ConnectionTable *ConnectionTable_create()
{
	DEBUG("enter ConnectionTable_create");
	struct ConnectionTable *table;

	table = (struct ConnectionTable *)os_zalloc(sizeof *table);

	if (table == NULL) {
		DEBUG("exit ConnectionTable_create");
		return NULL;
	}

	SLIST_INIT(&table->entries);
	table->count = 0;
	DEBUG("exit ConnectionTable_create");
	return table;
}

void ConnectionTable_destroy(struct ConnectionTable *table)
{
	DEBUG("enter ConnectionTable_destroy");
	struct ConnectionTableRecord *record;
	struct ConnectionTableRecord *prev = NULL;

	if (table == NULL) {
		DEBUG("exit ConnectionTable_destroy");
		return;
	}

	SLIST_FOREACH(record, &table->entries, next) {
		if (prev != NULL)
			ConnectionTableRecord_destroy(prev);

		prev = record;
	}

	if (prev != NULL)
		ConnectionTableRecord_destroy(prev);

	os_free(table);
	DEBUG("exit ConnectionTable_destroy");
}

int ConnectionTable_insert(struct ConnectionTable *table, const struct espconn *conn)
{
	DEBUG("enter ConnectionTable_insert");
	struct ConnectionTableRecord *record;

	if (table == NULL || conn == NULL) {
		DEBUG("exit ConnectionTable_insert");
		return 1;
	}

	SLIST_FOREACH(record, &table->entries, next) {
		if (is_same_conn(&record->info, conn)) {
			DEBUG("exit ConnectionTable_insert");
			return 1;
		}
	}

	record = ConnectionTableRecord_create(conn);

	if (record == NULL) {
		DEBUG("exit ConnectionTable_insert");
		return -1;
	}

	SLIST_INSERT_HEAD(&table->entries, record, next);
	++table->count;
	DEBUG("exit ConnectionTable_insert");
	return 0;
}

int ConnectionTable_delete(struct ConnectionTable *table, const struct espconn *conn)
{
	DEBUG("enter ConnectionTable_delete");
	struct ConnectionTableRecord *record;

	if (table == NULL || conn == NULL) {
		DEBUG("exit ConnectionTable_delete");
		return 1;
	}

	SLIST_FOREACH(record, &table->entries, next) {
		if (is_same_conn(&record->info, conn))
			break;
	}

	if (record == NULL) {
		DEBUG("exit ConnectionTable_delete");
		return 1;
	}

	SLIST_REMOVE(&table->entries, record, ConnectionTableRecord, next);
	ConnectionTableRecord_destroy(record);
	--table->count;
	DEBUG("exit ConnectionTable_delete");
	return 0;
}

int ConnectionTable_recvmsg_push(struct ConnectionTable *table, const struct espconn *conn,
					void *data, uint16 len, enum Memtype mem)
{
	DEBUG("enter ConnectionTable_recvmsg_push");
	struct ConnectionTableRecord *record;

	if (table == NULL || conn == NULL) {
		DEBUG("exit ConnectionTable_recvmsg_push");
		return 1;
	}

	SLIST_FOREACH(record, &table->entries, next) {
		if (is_same_conn(&record->info, conn))
			break;
	}

	if (record == NULL) {
		DEBUG("exit ConnectionTable_recvmsg_push");
		return 1;
	}

	if (MessageQueue_push(&record->recvq, data, len, mem) != 0) {
		DEBUG("exit ConnectionTable_recvmsg_push");
		return -1;
	}

	DEBUG("exit ConnectionTable_recvmsg_push");
	return 0;
}

int ConnectionTable_recvmsg_unshift(struct ConnectionTable *table, const struct espconn *conn,
					void **datap, uint16 *lenp, enum Memtype *memp)
{
	DEBUG("enter ConnectionTable_recvmsg_unshift");
	struct ConnectionTableRecord *record;

	if (table == NULL || conn == NULL) {
		DEBUG("exit ConnectionTable_recvmsg_unshift");
		return 1;
	}

	SLIST_FOREACH(record, &table->entries, next) {
		if (is_same_conn(&record->info, conn))
			break;
	}

	if (record == NULL) {
		DEBUG("exit ConnectionTable_recvmsg_unshift");
		return 1;
	}

	if (MessageQueue_unshift(&record->recvq, datap, lenp, memp) != 0) {
		DEBUG("exit ConnectionTable_recvmsg_unshift");
		return -1;
	}

	DEBUG("exit ConnectionTable_recvmsg_unshift");
	return 0;
}

int ConnectionTable_sendmsg_push(struct ConnectionTable *table, const struct espconn *conn,
					void *data, uint16 len, enum Memtype mem)
{
	DEBUG("enter ConnectionTable_sendmsg_push");
	struct ConnectionTableRecord *record;

	if (table == NULL || conn == NULL) {
		DEBUG("exit ConnectionTable_sendmsg_push");
		return 1;
	}

	SLIST_FOREACH(record, &table->entries, next) {
		if (is_same_conn(&record->info, conn))
			break;
	}

	if (record == NULL) {
		DEBUG("exit ConnectionTable_sendmsg_push");
		return 1;
	}

	if (MessageQueue_push(&record->sendq, data, len, mem) != 0) {
		DEBUG("exit ConnectionTable_sendmsg_push");
		return -1;
	}

	DEBUG("exit ConnectionTable_sendmsg_push");
	return 0;
}

int ConnectionTable_sendmsg_unshift(struct ConnectionTable *table, const struct espconn *conn,
					void **datap, uint16 *lenp, enum Memtype *memp)
{
	DEBUG("enter ConnectionTable_sendmsg_unshift");
	struct ConnectionTableRecord *record;

	if (table == NULL || conn == NULL) {
		DEBUG("exit ConnectionTable_sendmsg_unshift");
		return 1;
	}

	SLIST_FOREACH(record, &table->entries, next) {
		if (is_same_conn(&record->info, conn))
			break;
	}

	if (record == NULL) {
		DEBUG("exit ConnectionTable_sendmsg_unshift");
		return 1;
	}

	if (MessageQueue_unshift(&record->sendq, datap, lenp, memp) != 0) {
		DEBUG("exit ConnectionTable_sendmsg_unshift");
		return -1;
	}

	DEBUG("exit ConnectionTable_sendmsg_unshift");
	return 0;
}

bool ConnectionTable_sendmsg_empty(const struct ConnectionTable *table, const struct espconn *conn)
{
	DEBUG("enter ConnectionTable_sendmsg_empty");
	struct ConnectionTableRecord *record;

	if (table == NULL || conn == NULL) {
		DEBUG("exit ConnectionTable_sendmsg_empty");
		return true;
	}

	SLIST_FOREACH(record, &table->entries, next) {
		if (is_same_conn(&record->info, conn))
			break;
	}

	if (record == NULL) {
		DEBUG("exit ConnectionTable_sendmsg_empty");
		return true;
	}

	DEBUG("exit ConnectionTable_sendmsg_empty (calling MessageQueue_empty)");
	return MessageQueue_empty(&record->sendq);
}

bool ConnectionTable_recvmsg_empty(const struct ConnectionTable *table, const struct espconn *conn)
{
	DEBUG("enter ConnectionTable_recvmsg_empty");
	struct ConnectionTableRecord *record;

	if (table == NULL || conn == NULL) {
		DEBUG("exit ConnectionTable_recvmsg_empty");
		return true;
	}

	SLIST_FOREACH(record, &table->entries, next) {
		if (is_same_conn(&record->info, conn))
			break;
	}

	if (record == NULL) {
		DEBUG("exit ConnectionTable_recvmsg_empty");
		return true;
	}

	DEBUG("exit ConnectionTable_recvmsg_empty (calling MessageQueue_empty)");
	return MessageQueue_empty(&record->recvq);
}
