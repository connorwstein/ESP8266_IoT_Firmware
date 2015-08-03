#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "user_interface.h"

#include "user_config.h"
#include "debug.h"

char * ICACHE_FLASH_ATTR separate(char *str, char sep)
{
	DEBUG("enter separate");
	size_t i;

	for (i = 0; str[i] != '\0'; i++) {
		if (str[i] == sep) {
			str[i] = '\0';
			DEBUG("exit separate");
			return str + i + 1;
		}
	}

	DEBUG("exit separate");
	return "";
}

void ICACHE_FLASH_ATTR strip_newline(char *str)
{
	DEBUG("enter strip_newline");
	int i;

	i = os_strlen(str) - 1;

	while ((i >= 0) && (str[i] == '\n' || str[i] == '\r')) {
		str[i] = '\0';
		--i;
	}

	DEBUG("exit strip_newline");
}

int ICACHE_FLASH_ATTR generate_default_ssid(char *ssid, uint8 len)
{
	DEBUG("enter generate_default_ssid");
	uint8 mac[6];

	if (sizeof DEFAULT_AP_SSID_PREFIX + 3 >= len) {
		ets_uart_printf("SSID too long.\n");
		DEBUG("exit generate_default_ssid");
		return -1;
	}

	if (!wifi_get_macaddr(SOFTAP_IF, mac)) {
		ets_uart_printf("Failed to get MAC address.\n");
		DEBUG("exit generate_default_ssid");
		return -1;
	}

	os_sprintf(ssid, DEFAULT_AP_SSID_PREFIX "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	DEBUG("exit generate_default_ssid");
	return 0;
}

void ICACHE_FLASH_ATTR print_softap_config(const struct softap_config *config)
{
	DEBUG("enter print_softap_config");
	ets_uart_printf("SSID:\t\t\t%s\n", config->ssid);
	ets_uart_printf("Password:\t\t%s\n", config->password);
	ets_uart_printf("Channel:\t\t%d\n", config->channel);
	ets_uart_printf("Authmode:\t\t%d\n", config->authmode);
	ets_uart_printf("Hidden:\t\t\t%d\n", config->ssid_hidden);
	ets_uart_printf("Max connection:\t\t%d\n", config->max_connection);
	ets_uart_printf("Beacon interval:\t%d\n", config->beacon_interval);
	DEBUG("exit print_softap_config");
}

char * ICACHE_FLASH_ATTR str_bssid(uint8 *bssid)
{
	DEBUG("enter str_bssid");
	static char bssid_str[20];

	os_sprintf(bssid_str, "%02x:%02x:%02x:%02x:%02x:%02x",
			bssid[0], bssid[1], bssid[2],
			bssid[3], bssid[4], bssid[5]);

	DEBUG("exit str_bssid");
	return bssid_str;
}

char * ICACHE_FLASH_ATTR str_mac(uint8 *mac)
{
	DEBUG("enter str_mac");
	static char mac_str[20];

	os_sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
			mac[0], mac[1], mac[2],
			mac[3], mac[4], mac[5]);

	DEBUG("exit str_mac");
	return mac_str;
}

char * ICACHE_FLASH_ATTR inet_ntoa(uint32 addr)
{
	DEBUG("enter inet_ntoa");
	static char addr_str[17];
	uint8 *bytes = (uint8 *)&addr;

	os_sprintf(addr_str, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
	DEBUG("exit inet_ntoa");
	return addr_str;
}

void ICACHE_FLASH_ATTR print_ip_info(const struct ip_info *info)
{
	DEBUG("enter print_ip_info");
	ets_uart_printf("IP Address:\t\t%s\n", inet_ntoa(info->ip.addr));
	ets_uart_printf("Netmask:\t\t%s\n", inet_ntoa(info->netmask.addr));
	ets_uart_printf("Gateway:\t\t%s\n", inet_ntoa(info->gw.addr));
	DEBUG("exit print_ip_info");
}
