#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "user_interface.h"

#include "debug.h"
#include "wifi_user_interface.h"

#define WIFI_EVENT_HANDLER_TASK_QUEUE_LEN 10
static os_event_t wifi_event_handler_task_queue[WIFI_EVENT_HANDLER_TASK_QUEUE_LEN];

static wifi_event_handler_cb_t wifi_handle_event_cb = NULL;

static void ICACHE_FLASH_ATTR wifi_event_handler_task(os_event_t *e)
{
	static uint8 prev_status = 0;
	System_Event_t evt;
	uint8 status;

	if (wifi_handle_event_cb != NULL) {
		status = wifi_station_get_connect_status();

		if (status != prev_status) {
			if (status == STATION_GOT_IP) {
				evt.event = EVENT_STAMODE_GOT_IP;
				wifi_get_ip_info(STATION_IF, (struct ip_info *)&(evt.event_info.got_ip));
				wifi_handle_event_cb(&evt);
			} else if (prev_status == STATION_GOT_IP) {
				evt.event = EVENT_STAMODE_DISCONNECTED;
				os_memset(&(evt.event_info), 0, sizeof evt.event_info);
				wifi_handle_event_cb(&evt);
			} else if (status == STATION_WRONG_PASSWORD ||
				   status == STATION_NO_AP_FOUND ||
				   status == STATION_CONNECT_FAIL) {
				evt.event = EVENT_STAMODE_DISCONNECTED;
				os_memset(&(evt.event_info), 0, sizeof evt.event_info);
				wifi_handle_event_cb(&evt);
			}
		}
	}

	system_os_post(USER_TASK_PRIO_1, 0, 0);
}

void wifi_set_event_handler_cb(wifi_event_handler_cb_t cb)
{
	wifi_handle_event_cb = cb;
}

void ICACHE_FLASH_ATTR wifi_event_handler_task_start()
{
	system_os_task(wifi_event_handler_task, USER_TASK_PRIO_1, wifi_event_handler_task_queue, WIFI_EVENT_HANDLER_TASK_QUEUE_LEN);
	system_os_post(USER_TASK_PRIO_1, 0, 0);
}
