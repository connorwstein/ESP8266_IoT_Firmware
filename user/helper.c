#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "ip_addr.h"
#include "user_interface.h"

#include "user_config.h"

#define FLASH_GUARD_ADDRESS 0x3C000
#define FLASH_USED_VALUE 0xDEADBEEF
#define USER_FLASH_ADDRESS 0x3C004
#define USER_FLASH_SECTOR 0x3C

extern bool HAS_BEEN_CONNECTED_AS_STATION;
extern bool HAS_BEEN_AP;

char * ICACHE_FLASH_ATTR separate(char *str, char sep, unsigned short len)
{
	size_t i;

	for (i = 0; i < len; i++) {
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

			//Disconnected from network - convert to AP for reconfiguration
			if (!wifi_station_disconnect())
				 ets_uart_printf("Failed to disconnect.\n");

			if (HAS_BEEN_CONNECTED_AS_STATION||HAS_BEEN_AP)
				//Retart to discovery mode i.e. AP only
				//Either lost connection or failed to connect to users specified network
				system_restart();

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
