#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "mem.h"
#include "ets_sys.h"
#include "eagle_soc.h"
#include "gpio.h"

#include "device_config.h"
#include "heater.h"
#include "debug.h"

#define HEATER_PIN	2 /* GPIO2: Active low, must set to 0 to turn on. */

#define PWM_0_OUT_IO_MUX	PERIPHS_IO_MUX_GPIO2_U
#define PWM_0_OUT_IO_FUNC	FUNC_GPIO2
#define PWM_0_OUT_IO_NUM	HEATER_PIN

static enum HeaterState state = HEATER_OFF;

void ICACHE_FLASH_ATTR Heater_turn_on()
{
	DEBUG("enter Heater_turn_on");

	if (state == HEATER_ON) {
		ets_uart_printf("Heater already turned on.\n");
		return;
	}

	state = HEATER_ON;
	GPIO_OUTPUT_SET(HEATER_PIN, 0);	/* Pin 2 is active low */

	ets_uart_printf("Heater turned on!\n");
	DEBUG("exit Heater_turn_on");
}

void ICACHE_FLASH_ATTR Heater_turn_off()
{
	DEBUG("enter Heater_turn_off");

	if (state == HEATER_OFF) {
		ets_uart_printf("Heater should have already been turned off.\n");
		ets_uart_printf("Turning off anyways just in case...\n");
	}

	state = HEATER_OFF;
	GPIO_OUTPUT_SET(HEATER_PIN, 1);	/* Pin 2 is active low */

	ets_uart_printf("Heater turned off!\n");
	DEBUG("exit Heater_turn_off");
}

int ICACHE_FLASH_ATTR Heater_init(struct DeviceConfig *config)
{
	gpio_init();

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO2_U);
	GPIO_OUTPUT_SET(HEATER_PIN, 1);	/* Default is off, active-low pin. */
	state = HEATER_OFF;

	ets_uart_printf("Heater initialized: state = %s!\n", state == HEATER_ON ? "ON" : "OFF");
	return 0;
}

int ICACHE_FLASH_ATTR Heater_set_default_data(struct DeviceConfig *config)
{
	return 0;
}
