#ifndef CONNECTION_TABLE_H
#define CONNECTION_TABLE_H

#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"

/* These functions can be used to make the server
   handle individual connections in a more general way.
   They are not currently used in the code, but this
   might be an improvement on the current global variables we have.

   The idea would be that each server has its own ConnectionTable.
   When a new connection is obtained (via espconn_connect_cb), insert the
   connection into the table using ConnectionTable_insert().
   When the connection is disconnected (via espconn_disconnec_cb), delete the
   connection from the table using ConnectionTable_delete().

   New messages (from espconn_recvcb) are pushed into the recieve message queue
   using ConnectionTable_recvmsg_push, and read by the server parser in FIFO order with
   ConnectionTable_recvmsg_unshift. Similarly, the server pushes new messages to send
   to the espconn using ConnectionTable_sendmsg_push, and they are processed
   in FIFO order using ConnectionTable_sendmsg_unshift.

   Large messages are automatically fragmented into chunks of a maximum size
   that can be sent in one shot using espconn_sent. The rest of the packet
   is sent after receiving the espconn_sent_cb of the previous one. This allows
   handling large buffers (for example the pictures taken by camera). */
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
