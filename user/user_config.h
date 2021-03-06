#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#define DEFAULT_AP_SSID_PREFIX	"ESP_"
#define DEFAULT_AP_PASSWORD	""
#define DEFAULT_AP_CHANNEL	6

#define AUTO_CONNECT_TIMEOUT	7000

#define MQTT_HOST	"192.168.2.216"
#define MQTT_PORT	1883
#define MQTT_SECURITY	0
#define MQTT_DEVICE_ID	"client_id"
#define MQTT_USER	""
#define MQTT_PASS	""
#define MQTT_KEEPALIVE	60

/* Comment/uncomment this to determine if the
   device will be used as a locator or not. */
#define USE_AS_LOCATOR

#endif
