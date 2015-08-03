#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "mem.h"
#include "ets_sys.h"
#include "eagle_soc.h"
#include "gpio.h"
#include "pwm.h"

#include "device_config.h"
#include "lighting.h"
#include "debug.h"

#define LIGHT_PIN	2 /* GPIO2 */

#define PWM_0_OUT_IO_MUX	PERIPHS_IO_MUX_GPIO2_U
#define PWM_0_OUT_IO_FUNC	FUNC_GPIO2
#define PWM_0_OUT_IO_NUM	LIGHT_PIN

static enum LightState state = LIGHT_OFF;
static const uint32 period = 83;	/* In microseconds: 120 Hz */
static uint32 duty = 0;
static uint32 intensity_c;

void ICACHE_FLASH_ATTR Lighting_dim(uint8 intensity)
{
	uint32 duuts;

	/* Clamp intensity to 100 */
	if (intensity > 100)
		intensity = 100;

	if (intensity == 0)
		duuts = 0.0;
	else
		duuts = 400.0 + 1.7 * (float)intensity;

	ets_uart_printf("Dimming light: %d%%\n", intensity);
	ets_uart_printf("duuts = %d\n", (int)duuts);
	duty = (uint32)((duuts * (float)period) / 45.0);
	pwm_set_period(period);
	pwm_set_duty(duty, 0);
	pwm_start();
	intensity_c = intensity;
}

void ICACHE_FLASH_ATTR Lighting_toggle_light()
{
	DEBUG("enter Lighting_toggle_light");
	if (state == LIGHT_OFF) {
		state = LIGHT_ON;
		GPIO_OUTPUT_SET(LIGHT_PIN, 1);
	} else {
		state = LIGHT_OFF;
		GPIO_OUTPUT_SET(LIGHT_PIN, 0);
	}

	ets_uart_printf("Toggled light to %s!\n", state == LIGHT_OFF ? "OFF" : "ON");

//	if (DeviceConfig_set_data(&state, sizeof (enum LightState)) != 0)
//		ets_uart_printf("Failed to set lighting data in device config.\n");

	DEBUG("exit Lighting_toggle_light");
}

void ICACHE_FLASH_ATTR Lighting_get_light(struct espconn *conn)
{
	DEBUG("enter Lighting_get_light");
	char buff[5];

	memset(buff, 0, sizeof buff);
//	os_sprintf(buff, "%s", state == LIGHT_OFF ? "OFF" : "ON");
	os_sprintf(buff, "%d", intensity_c);
	if (espconn_sent(conn, (uint8 *)buff, os_strlen(buff)) != 0)
		ets_uart_printf("espconn_sent failed.\n");

	ets_uart_printf("Replied with: %s\n", buff);
	DEBUG("exit Lighting_get_light");
}

int ICACHE_FLASH_ATTR Lighting_init(struct DeviceConfig *config)
{
	uint32 io_info[][3] = {{PWM_0_OUT_IO_MUX, PWM_0_OUT_IO_FUNC, PWM_0_OUT_IO_NUM}};
	gpio_init();

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO2_U);
	GPIO_OUTPUT_SET(LIGHT_PIN, 0);
	state = LIGHT_OFF;

	pwm_init(period, &duty, 1, io_info);

//	if (config->data_len != sizeof (enum LightState)) {
//		ets_uart_printf("Wrong size for lighting data: %d.\n", config->data_len);
//		return -1;
//	}

//	state = *(enum LightState *)(config->data);
	ets_uart_printf("Lighting initialized: state = %s!\n", state == LIGHT_ON ? "ON" : "OFF");
	return 0;
}

int ICACHE_FLASH_ATTR Lighting_set_default_data(struct DeviceConfig *config)
{
/*	config->data = (enum LightState *)os_zalloc(sizeof (enum LightState));

	if (config->data == NULL) {
		ets_uart_printf("Failed to allocate memory for lighting data.\n");
		return -1;
	}

	config->data_len = sizeof (enum LightState);
	*(enum LightState *)(config->data) = LIGHT_OFF;
*/	return 0;
}
