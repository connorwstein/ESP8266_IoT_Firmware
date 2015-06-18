#ifndef WIFI_USER_INTERFACE_H
#define WIFI_USER_INTERFACE_H

#include "c_types.h"
#include "ip_addr.h"

enum {
	EVENT_STAMODE_CONNECTED = 0,
	EVENT_STAMODE_DISCONNECTED,
	EVENT_STAMODE_AUTHMODE_CHANGE,
	EVENT_STAMODE_GOT_IP,
	EVENT_SOFTAPMODE_STACONNECTED,
	EVENT_SOFTAPMODE_STADISCONNECTED,
	EVENT_MAX
};

enum {
	REASON_UNSPECIFIED              = 1,
	REASON_AUTH_EXPIRE              = 2,
	REASON_AUTH_LEAVE               = 3,
	REASON_ASSOC_EXPIRE             = 4,
	REASON_ASSOC_TOOMANY            = 5,
	REASON_NOT_AUTHED               = 6,
	REASON_NOT_ASSOCED              = 7,
	REASON_ASSOC_LEAVE              = 8,
	REASON_ASSOC_NOT_AUTHED         = 9,
	REASON_DISASSOC_PWRCAP_BAD      = 10,  /* 11h */
	REASON_DISASSOC_SUPCHAN_BAD     = 11,  /* 11h */
	REASON_IE_INVALID               = 13,  /* 11i */
	REASON_MIC_FAILURE              = 14,  /* 11i */
	REASON_4WAY_HANDSHAKE_TIMEOUT   = 15,  /* 11i */
	REASON_GROUP_KEY_UPDATE_TIMEOUT = 16,  /* 11i */
	REASON_IE_IN_4WAY_DIFFERS       = 17,  /* 11i */
	REASON_GROUP_CIPHER_INVALID     = 18,  /* 11i */
	REASON_PAIRWISE_CIPHER_INVALID  = 19,  /* 11i */
	REASON_AKMP_INVALID             = 20,  /* 11i */
	REASON_UNSUPP_RSN_IE_VERSION    = 21,  /* 11i */
	REASON_INVALID_RSN_IE_CAP       = 22,  /* 11i */
	REASON_802_1X_AUTH_FAILED       = 23,  /* 11i */
	REASON_CIPHER_SUITE_REJECTED    = 24,  /* 11i */

	REASON_BEACON_TIMEOUT           = 200,
	REASON_NO_AP_FOUND              = 201,
};

typedef struct {
	uint8 ssid[32];
	uint8 ssid_len;
	uint8 bssid[6];
	uint8 channel;
} Event_StaMode_Connected_t;

typedef struct {
	uint8 ssid[32];
	uint8 ssid_len;
	uint8 bssid[6];
	uint8 reason;
} Event_StaMode_Disconnected_t;

typedef struct {
	uint8 old_mode;
	uint8 new_mode;
} Event_StaMode_AuthMode_Change_t;

typedef struct {
	struct ip_addr ip;
	struct ip_addr mask;
	struct ip_addr gw;
} Event_StaMode_Got_IP_t;

typedef struct {
	uint8 mac[6];
	uint8 aid;
} Event_SoftAPMode_StaConnected_t;

typedef struct {
	uint8 mac[6];
	uint8 aid;
} Event_SoftAPMode_StaDisconnected_t;

typedef union {
	Event_StaMode_Connected_t		connected;
	Event_StaMode_Disconnected_t		disconnected;
	Event_StaMode_AuthMode_Change_t		auth_change;
	Event_StaMode_Got_IP_t			got_ip;
	Event_SoftAPMode_StaConnected_t		sta_connected;
	Event_SoftAPMode_StaDisconnected_t	sta_disconnected;
} Event_Info_u;

typedef struct _esp_event {
	uint32 event;
	Event_Info_u event_info;
} System_Event_t;

typedef void (* wifi_event_handler_cb_t)(System_Event_t *event);

void wifi_set_event_handler_cb(wifi_event_handler_cb_t cb);

#endif
