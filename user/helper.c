#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "ip_addr.h"
#include "user_interface.h"

#include "user_config.h"
#include "helper.h"

#define FLASH_GUARD_ADDRESS 0x3C000
#define FLASH_USED_VALUE 0xDEADBEEF
#define USER_FLASH_ADDRESS 0x3C004
#define USER_FLASH_SECTOR 0x3C

extern bool HAS_BEEN_CONNECTED_AS_STATION;
extern bool HAS_RECEIVED_CONNECT_INSTRUCTION;

char * ICACHE_FLASH_ATTR separate(char *str, char sep)
{
	size_t i;

	for (i = 0; str[i] != '\0'; i++) {
		if (str[i] == sep) {
			str[i] = '\0';
			return str + i + 1;
		}
	}

	return NULL;
}

void ICACHE_FLASH_ATTR strip_newline(char *str)
{
	int i;

	i = strlen(str) - 1;

	while ((i >= 0) && (str[i] == '\n' || str[i] == '\r')) {
		str[i] = '\0';
		--i;
	}
}

int ICACHE_FLASH_ATTR generate_default_ssid(char *ssid, uint8 len)
{
	uint8 mac[6];

	if (sizeof DEFAULT_AP_SSID_PREFIX + 3 >= len) {
		ets_uart_printf("SSID too long.\n");
		return -1;
	}

	if (!wifi_get_macaddr(SOFTAP_IF, mac)) {
		ets_uart_printf("Failed to get MAC address.\n");
		return -1;
	}

	os_sprintf(ssid, DEFAULT_AP_SSID_PREFIX "%02x%02x%02x", mac[3], mac[4], mac[5]);
	return 0;
}

void ICACHE_FLASH_ATTR print_softap_config(const struct softap_config *config)
{
	ets_uart_printf("SSID:\t\t\t%s\n", config->ssid);
	ets_uart_printf("Password:\t\t%s\n", config->password);
	ets_uart_printf("Channel:\t\t%d\n", config->channel);
	ets_uart_printf("Authmode:\t\t%d\n", config->authmode);
	ets_uart_printf("Hidden:\t\t\t%d\n", config->ssid_hidden);
	ets_uart_printf("Max connection:\t\t%d\n", config->max_connection);
	ets_uart_printf("Beacon interval:\t%d\n", config->beacon_interval);
}

const char * ICACHE_FLASH_ATTR str_bssid(uint8 *bssid)
{
	static char bssid_str[20];

	os_sprintf(bssid_str, "%02x:%02x:%02x:%02x:%02x:%02x",
			bssid[0], bssid[1], bssid[2],
			bssid[3], bssid[4], bssid[5]);

	return bssid_str;
}
const char * ICACHE_FLASH_ATTR str_mac(uint8 *mac)
{
	static char mac_str[20];

	os_sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
			mac[0], mac[1], mac[2],
			mac[3], mac[4], mac[5]);

	return mac_str;
}

const char * ICACHE_FLASH_ATTR inet_ntoa(uint32 addr)
{
	static char addr_str[17];
	uint8 *bytes = (uint8 *)&addr;

	os_sprintf(addr_str, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
	return addr_str;
}

void ICACHE_FLASH_ATTR print_ip_info(const struct ip_info *info)
{
	ets_uart_printf("IP Address:\t\t%s\n", inet_ntoa(info->ip.addr));
	ets_uart_printf("Netmask:\t\t%s\n", inet_ntoa(info->netmask.addr));
	ets_uart_printf("Gateway:\t\t%s\n", inet_ntoa(info->gw.addr));
}

int ICACHE_FLASH_ATTR start_station(const char *ssid, const char *password)
{
	struct station_config config;

	if (!wifi_set_opmode(STATION_MODE)) {
		ets_uart_printf("Failed to set as station mode.\n");
		return -1;
	}

	memset(&config.ssid, 0, 32);
	memset(&config.password, 0, 64);

	os_memcpy(&config.ssid, ssid, strlen(ssid));
	os_memcpy(&config.password, password, strlen(password));
	config.bssid_set = 0;
	memset(config.bssid, 0, sizeof config.bssid);

	if (!wifi_station_set_config(&config)) {
		ets_uart_printf("Failed to configure station.\n");
		return -1;
	}

	if (!wifi_station_connect()) {
		ets_uart_printf("Failed to connect to router.\n");
		return -1;
	}

	return 0;
}

int ICACHE_FLASH_ATTR start_access_point(const char *ssid, const char *password, uint8 channel)
{
	struct softap_config config;
	ets_uart_printf("Starting access point\n");

	//Set access point mode
	if (!wifi_set_opmode(SOFTAP_MODE)) {
		ets_uart_printf("Failed to set as access point mode.\n");
		return -1;
	}

	//Set ap settings
	os_memcpy(&config.ssid, ssid, 32);
	os_memcpy(&config.password, password, 64);
	config.ssid_len = strlen(ssid);
	config.channel = channel;
	config.authmode = 0;
	config.ssid_hidden = 0;
	config.max_connection = 10;
	config.beacon_interval = 100;

	if (!wifi_softap_set_config(&config)) {
		ets_uart_printf("Failed to configure access point.\n");
		return -1;
	}

	return 0;
}

int ICACHE_FLASH_ATTR read_from_flash(uint32 *data, uint32 size)
{
	SpiFlashOpResult read;

	ets_uart_printf("Reading %u bytes from 0x%08x\n", size, USER_FLASH_ADDRESS);
	read = spi_flash_read(USER_FLASH_ADDRESS, data, size);

	if (read != SPI_FLASH_RESULT_OK) {
		ets_uart_printf("Spi flash read failed.\n");
		return -1;
	}

	return 0;
}

int ICACHE_FLASH_ATTR write_to_flash(uint32 *data, uint32 size)
{
	SpiFlashOpResult erase;
	SpiFlashOpResult write;
	uint32 used = FLASH_USED_VALUE;

	ets_uart_printf("Erasing sector 0x%02x\n", USER_FLASH_SECTOR);
	// must erase whole sector before writing
	erase = spi_flash_erase_sector(USER_FLASH_SECTOR);

	if (erase != SPI_FLASH_RESULT_OK) {
		ets_uart_printf("Erase failed: %d\n", erase);
		return -1;
	}

	ets_uart_printf("Writing %u bytes to 0x%08x\n", size, USER_FLASH_ADDRESS);
	write = spi_flash_write(USER_FLASH_ADDRESS, data, size);

	if (write != SPI_FLASH_RESULT_OK) {
		ets_uart_printf("Write failed.");
		return -1;
	}

	ets_uart_printf("Writing flash used value 0x%08x to 0x%08x\n", FLASH_USED_VALUE, FLASH_GUARD_ADDRESS);
	write = spi_flash_write(FLASH_GUARD_ADDRESS, &used, sizeof used);

	if (write != SPI_FLASH_RESULT_OK) {
		ets_uart_printf("Write guard flash failed.\n");
		return -1;
	}

/*	ets_uart_printf("Reading data from 0x%08x\n", USER_FLASH_ADDRESS);
	read_back = spi_flash_read(USER_FLASH_ADDRESS, &read_back_test, size);

	if (read_back) {
		ets_uart_printf("Read back failed: %d\n", read_back);
		return -1;
	} else if (read_back_test != *data) {
		ets_uart_printf("Read back does not match: data = %u, read_back_test = %u\n", *data, read_back_test);
		return -1;
	}

	ets_uart_printf("Read %d from 0x%08x\n", read_back_test, USER_FLASH_ADDRESS);
*/
	return 0;
} 

int ICACHE_FLASH_ATTR is_flash_used()
{
	SpiFlashOpResult read;
	uint32 blank;

	read = spi_flash_read(FLASH_GUARD_ADDRESS, &blank, sizeof blank);

	if (read != SPI_FLASH_RESULT_OK) {
		ets_uart_printf("Spi flash read failed: %d\n", read);
		return -1;
	}

	if (blank == FLASH_USED_VALUE)
		return 1;

	return 0;
}

int ICACHE_FLASH_ATTR save_device_config(const struct DeviceConfig *conf)
{
	char buf[DEVICE_CONFIG_BUF_LEN];

	if (conf == NULL || conf->device_name == NULL || conf->device_type == NULL)
		return -1;

	ets_uart_printf("Save device config: device_name = %s, device_type = %s\n", conf->device_name, conf->device_type);
	os_memset(buf, 0, sizeof buf);
	os_memcpy(buf, conf->device_name, sizeof conf->device_name);
	os_memcpy(buf + sizeof conf->device_name, conf->device_type, sizeof conf->device_type);
	ets_uart_printf("Will write to flash: %s\n", buf);

	if (write_to_flash((uint32 *)buf, sizeof buf) != 0)
		return -1;

	return 0;
}

int ICACHE_FLASH_ATTR read_device_config(struct DeviceConfig *conf)
{
	char buf[DEVICE_CONFIG_BUF_LEN];

	if (!is_flash_used())
		return 1;

	if (conf == NULL)
		return -1;

	os_memset(buf, 0, sizeof buf);

	if (read_from_flash((uint32 *)buf, sizeof buf) != 0)
		return -1;

	ets_uart_printf("Read from flash: %s\n", buf);
	os_memset(conf->device_name, 0, sizeof conf->device_name);
	os_memset(conf->device_type, 0, sizeof conf->device_type);
	os_memcpy(conf->device_name, buf, sizeof conf->device_name);
	os_memcpy(conf->device_type, buf + sizeof conf->device_name, sizeof conf->device_type);
	ets_uart_printf("Read device config: device_name = %s, device_type = %s\n", conf->device_name, conf->device_type);
	return 0;
}

/* Set this as wifi handler between system_restart() and resetting the flags
   to avoid getting wifi disconnected events before the flags have been reset
   and get into a restart loop. */
static void ICACHE_FLASH_ATTR empty_wifi_handler(System_Event_t *event)
{
	ets_uart_printf("Empty wifi event handler: event = %d\n", event->event);
}

void ICACHE_FLASH_ATTR sta_wifi_handler(System_Event_t *event)
{
	struct ip_info info;
	
	switch (event->event) {
		case EVENT_STAMODE_CONNECTED:
			ets_uart_printf("Connected to Access Point!\n");
			ets_uart_printf("SSID: %s\n", event->event_info.connected.ssid);
			ets_uart_printf("BSSID: %s\n", str_bssid(event->event_info.connected.bssid));
			ets_uart_printf("Channel: %d\n", event->event_info.connected.channel);
			ets_uart_printf("\n");
			break;

		case EVENT_STAMODE_DISCONNECTED:
			ets_uart_printf("Disconnected from Access Point!\n");
			ets_uart_printf("SSID: %s\n", event->event_info.disconnected.ssid);
			ets_uart_printf("BSSID: %s\n", str_bssid(event->event_info.disconnected.bssid));
			ets_uart_printf("Reason: %d\n", event->event_info.disconnected.reason);
			ets_uart_printf("\n");

			//Retart to discovery mode i.e. AP only
			//Either lost connection or failed to connect to users specified network
			/* Ok so this is tricky... There are two cases where we want to restart.
			   
			1. We were connected as a station to a router and had a valid IP.
				
				In this case we want to restart because the router might be down, and
				we need to try auto-reconnect to it or else timeout and start in access point mode.
				The relevant flag that gets set in this case is HAS_BEEN_CONNECTED_AS_STATION.

			2. We were in access point mode, and have received an instruction to connect to a router,
			   but the connection was unsuccessful, either due to an nonexistent network config,
			   or due to a valid network being temporarily unavailable.
				
				If the config was invalid (bad SSID or password) the device will try to auto-connect
				again on restart and fail. This will cause multiple EVENT_STAMODE_DISCONNECTED events
				to be sent even after the first restart, which we want to ignore after the first restart,
				otherwise we fall into a restart loop. That is, we don't want to restart if the disconnect
				event was due to a connection attempt from the previous system restart, only if it was due
				to a disconnect from the *current* session.

				If the config was valid, but still got disconnected, it could be a temporary router failure.
				When we restart, auto-connect will try to reconnect, and if the router is working again,
				it will succeed next time. If the router is still down, we go back to access point mode after
				the timeout.

				The relevant flag that gets set in this case is HAS_RECEIVED_CONNECT_INSTRUCTION.

			On restart, both flags get set to false again. But we need to disable the current wifi event handler
			to make sure we don't get DISCONNECTED events just after restart, before the flags have been reset.
			We register an empty wifi handler callback just before restarting to avoid this.
			*/
				
			if (HAS_BEEN_CONNECTED_AS_STATION||HAS_RECEIVED_CONNECT_INSTRUCTION) {
				//Disconnected from network - convert to AP for reconfiguration
				if (!wifi_station_disconnect())
					 ets_uart_printf("Failed to disconnect.\n");
	
				wifi_set_event_handler_cb(empty_wifi_handler);
				system_restart();
			}

			break;

		case EVENT_STAMODE_GOT_IP:
			ets_uart_printf("Got IP!\n");
			print_ip_info((struct ip_info *)&(event->event_info.got_ip));
			ets_uart_printf("\n");	
			HAS_BEEN_CONNECTED_AS_STATION = true;
			sta_server_init_tcp();
			sta_server_init_udp();
			break;

		case EVENT_STAMODE_AUTHMODE_CHANGE:
			ets_uart_printf("EVENT_STAMODE_AUTHMODE_CHANGE\n");
			break;

		case  EVENT_SOFTAPMODE_STACONNECTED:
			ets_uart_printf("EVENT_SOFTAPMODE_STACONNECTED\n");
			break;

		case EVENT_SOFTAPMODE_STADISCONNECTED:
			ets_uart_printf("EVENT_SOFTAPMODE_STADISCONNECTED\n");			
			break;

		default:
			ets_uart_printf("Got event: %d\n", event->event);
			break;
	}
}
