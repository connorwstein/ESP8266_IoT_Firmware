#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"
#include "osapi.h"
#include "mem.h"
#include "queue.h"

#include "connection_table.h"

#define TCP_MAX_PACKET_SIZE 1460

struct Message {
	const void *data;
	uint16 len;
	enum Memtype mem;
	STAILQ_ENTRY(Message) next;
};

struct MessageQueue {
	STAILQ_HEAD(_messages, Message) messages;
	uint16 count;
};

struct ConnectionTableRecord {
	const struct espconn *conn;
	struct MessageQueue sendq;
	struct MessageQueue recvq;
	SLIST_ENTRY(ConnectionTableRecord) next;
};

struct ConnectionTable {
	SLIST_HEAD(_entries, ConnectionTableRecord) entries;
	uint16 count;
};

static bool is_same_conn(const struct espconn *a, const struct espconn *b)
{
	if (a->type != b->type)
		return false;

	switch (a->type) {
		case ESPCONN_TCP:
			if (a->proto.tcp->remote_port != b->proto.tcp->remote_port)
				return false;

			if (os_memcmp(a->proto.tcp->remote_ip, b->proto.tcp->remote_ip, 4) != 0)
				return false;

			break;
		case ESPCONN_UDP:
			if (a->proto.udp->remote_port != b->proto.udp->remote_port)
				return false;

			if (os_memcmp(a->proto.tcp->remote_ip, b->proto.udp->remote_ip, 4) != 0)
				return false;

			break;

		default:
			return false;
	}

	return true;
}

static void MessageQueue_destroy(struct MessageQueue *msgq)
{
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
}

static int MessageQueue_push(struct MessageQueue *msgq, const void *data, uint16 len, enum Memtype mem)
{
	struct Message *msg;

	if (len == 0)
		return 1;

	while (len > TCP_MAX_PACKET_SIZE) {
		msg = (struct Message *)os_zalloc(sizeof *msg);

		if (msg == NULL)
			return -1;

		msg->data = data;
		msg->len = TCP_MAX_PACKET_SIZE;
		msg->mem = mem;

		STAILQ_INSERT_TAIL(&msgq->messages, msg, next);
		++msgq->count;
		data = (uint8 *)data + TCP_MAX_PACKET_SIZE;
		len -= TCP_MAX_PACKET_SIZE;
	}

	msg = (struct Message *)os_zalloc(sizeof *msg);

	if (msg == NULL)
		return -1;

	msg->data = data;
	msg->len = TCP_MAX_PACKET_SIZE;
	msg->mem = mem;

	STAILQ_INSERT_TAIL(&msgq->messages, msg, next);
	++msgq->count;

	return 0;
}

static int MessageQueue_unshift(struct MessageQueue *msgq, const void **datap, uint16 *lenp, enum Memtype *memp)
{
	struct Message *msg;

	if (STAILQ_EMPTY(&msgq->messages))
		return 1;

	msg = STAILQ_FIRST(&msgq->messages);
	STAILQ_REMOVE_HEAD(&msgq->messages, next);

	if (msg == NULL)
		return -1;

	--msgq->count;
	*datap = msg->data;
	*lenp = msg->len;
	*memp = msg->mem;

	os_free(msg);
	return 0;
}

static struct ConnectionTableRecord *ConnectionTableRecord_create(const struct espconn *conn)
{
	struct ConnectionTableRecord *record;

	record = (struct ConnectionTableRecord *)os_zalloc(sizeof *record);

	if (record == NULL)
		return NULL;

	record->conn = conn;
	STAILQ_INIT(&record->sendq.messages);
	STAILQ_INIT(&record->recvq.messages);
	record->sendq.count = 0;
	record->recvq.count = 0;

	return record;
}

static void ConnectionTableRecord_destroy(struct ConnectionTableRecord *record)
{
	espconn_disconnect(record->conn);
	MessageQueue_destroy(&record->sendq);
	MessageQueue_destroy(&record->recvq);
	os_free(record);
}

struct ConnectionTable *ConnectionTable_create()
{
	struct ConnectionTable *table;

	table = (struct ConnectionTable *)os_zalloc(sizeof *table);

	if (table == NULL)
		return NULL;

	SLIST_INIT(&table->entries);
	table->count = 0;
	return table;
}

void ConnectionTable_destroy(struct ConnectionTable *table)
{
	struct ConnectionTableRecord *record;
	struct ConnectionTableRecord *prev = NULL;

	SLIST_FOREACH(record, &table->entries, next) {
		if (prev != NULL)
			ConnectionTableRecord_destroy(prev);

		prev = record;
	}

	if (prev != NULL)
		ConnectionTableRecord_destroy(prev);

	os_free(table);
}

int ConnectionTable_insert(struct ConnectionTable *table, const struct espconn *conn)
{
	struct ConnectionTableRecord *record;

	SLIST_FOREACH(record, &table->entries, next) {
		if (is_same_conn(record->conn, conn))
			return 1;
	}

	record = ConnectionTableRecord_create(conn);

	if (record == NULL)
		return -1;

	SLIST_INSERT_HEAD(&table->entries, record, next);
	++table->count;
	return 0;
}

int ConnectionTable_delete(struct ConnectionTable *table, const struct espconn *conn)
{
	struct ConnectionTableRecord *record;

	SLIST_FOREACH(record, &table->entries, next) {
		if (is_same_conn(record->conn, conn))
			break;
	}

	if (record == NULL)
		return 1;

	SLIST_REMOVE(&table->entries, record, ConnectionTableRecord, next);
	ConnectionTableRecord_destroy(record);
	--table->count;

	return 0;
}

int ConnectionTable_recvmsg_push(struct ConnectionTable *table, const struct espconn *conn,
					const void *data, uint16 len, enum Memtype mem)
{
	struct ConnectionTableRecord *record;

	SLIST_FOREACH(record, &table->entries, next) {
		if (is_same_conn(record->conn, conn))
			break;
	}

	if (record == NULL)
		return 1;

	if (MessageQueue_push(&record->recvq, data, len, mem) != 0)
		return -1;

	return 0;
}

int ConnectionTable_recvmsg_unshift(struct ConnectionTable *table, const struct espconn *conn,
					const void **datap, uint16 *lenp, enum Memtype *memp)
{
	struct ConnectionTableRecord *record;

	SLIST_FOREACH(record, &table->entries, next) {
		if (is_same_conn(record->conn, conn))
			break;
	}

	if (record == NULL)
		return 1;

	if (MessageQueue_unshift(&record->recvq, datap, lenp, memp) != 0)
		return -1;

	return 0;
}

int ConnectionTable_sendmsg_push(struct ConnectionTable *table, const struct espconn *conn,
					const void *data, uint16 len, enum Memtype mem)
{
	struct ConnectionTableRecord *record;

	SLIST_FOREACH(record, &table->entries, next) {
		if (is_same_conn(record->conn, conn))
			break;
	}

	if (record == NULL)
		return 1;

	if (MessageQueue_push(&record->sendq, data, len, mem) != 0)
		return -1;

	return 0;
}

int ConnectionTable_sendmsg_unshift(struct ConnectionTable *table, const struct espconn *conn,
					const void **datap, uint16 *lenp, enum Memtype *memp)
{
	struct ConnectionTableRecord *record;

	SLIST_FOREACH(record, &table->entries, next) {
		if (is_same_conn(record->conn, conn))
			break;
	}

	if (record == NULL)
		return 1;

	if (MessageQueue_unshift(&record->sendq, datap, lenp, memp) != 0)
		return -1;

	return 0;
}
