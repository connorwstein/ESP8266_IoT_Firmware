#ifndef CONNECTION_TABLE_H
#define CONNECTION_TABLE_H

#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"

/* These functions can be used to make the server
   handle individual connections in a more general way.
   They are not currently used in the code, but this
   would be an improvement on the current global variables we have. */
struct ConnectionTable;

struct ConnectionTable *ConnectionTable_create();
void ConnectionTable_destroy(struct ConnectionTable *table);

int ConnectionTable_insert(struct ConnectionTable *table, const struct espconn *conn);
int ConnectionTable_delete(struct ConnectionTable *table, const struct espconn *conn);

int ConnectionTable_recvmsg_push(struct ConnectionTable *table, const struct espconn *conn, const void *data, uint16 len);
int ConnectionTable_recvmsg_unshift(struct ConnectionTable *table, const struct espconn *conn, const void **datap, uint16 *lenp);
int ConnectionTable_sendmsg_push(struct ConnectionTable *table, const struct espconn *conn, const void *data, uint16 len);
int ConnectionTable_sendmsg_unshift(struct ConnectionTable *table, const struct espconn *conn, const void **datap, uint16 *lenp);

#endif
