#include "c_types.h"
#include "eagle_soc.h"
#include "gpio.h"
#include "ets_sys.h"
#include "mem.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"

#include "device_config.h"
#include "camera.h"
#include "software_uart.h"
#include "server.h"

#define RESET_RESPONSE_SIZE			4
#define TAKE_PICTURE_RESPONSE_SIZE		5
#define READ_SIZE_RESPONSE_SIZE			9
#define READ_CONTENT_RESPONSE_SIZE		5
#define STOP_PICTURES_RESPONSE_SIZE		5
#define COMPRESSION_RATIO_RESPONSE_SIZE		5
#define SET_IMAGE_SIZE_RESPONSE_SIZE		5
#define POWER_SAVING_ON_RESPONSE_SIZE		5
#define POWER_SAVING_OFF_RESPONSE_SIZE		5
#define SET_BAUD_RATE_RESPONSE_SIZE		5

/* In microseconds. Should be long enough for the camera to respond,
   but not too long to avoid triggering the watchdog reset. */
#define CAMERA_RESPONSE_TIMEOUT			10000

static struct rx_buffer *previous_rxbuffer = NULL;

static uint32 baud_rate;
static uint8 gpio_camera_rx;
static uint8 gpio_camera_tx;

static struct espconn *reply_conn = NULL;

struct camera_data {
	uint32 baud;
	uint8 gpio_rx;
	uint8 gpio_tx;
};

static const struct camera_data DEFAULT_CAMERA_DATA = {.baud = 38400, .gpio_rx = 5, .gpio_tx = 4};

static bool IS_CAMERA_BUSY = true;	/* true because reset! */

#define READ_PICTURE_CONTENTS_TIMEOUT	10000	/* In seconds */
static ETSTimer unlock_timer;		/* timer to unlock camera if read data never comes. */

static void ICACHE_FLASH_ATTR cancel_read_picture(void *timer_arg)
{
	disable_interrupts();
	destroy_rx_buffer(previous_rxbuffer);
	previous_rxbuffer = NULL;

	ets_intr_lock();
	IS_CAMERA_BUSY = false;
	ets_intr_unlock();

	ets_uart_printf("Camera read picture contents timeout.\n");
}

static void ICACHE_FLASH_ATTR print_rx_buffer(struct rx_buffer *buffer)
{
	int j = 0;
	ets_uart_printf("Response: ");

	while (j < buffer->size)
		ets_uart_printf("%02x ", buffer->buf[j++]);

	ets_uart_printf("\n");
}

static void camera_picture_recv_done(struct rx_buffer *buffer)
{
	uint16 len;
	uint16 offset;
	int rc;

	os_timer_disarm(&unlock_timer);
	ets_uart_printf("Done receiving picture contents: %d bytes\n", buffer->size);
	ets_uart_printf("Sending data to phone.\n");

	if (tcpserver_send(reply_conn, buffer->buf, buffer->size, HEAP_MEM) != 0) {
		ets_uart_printf("Failed to send picture contents.\n");
		os_free(buffer->buf);
	}

	os_free(buffer);
	previous_rxbuffer = NULL;
	/* Unlock camera for recv done */
	ets_intr_lock();
	IS_CAMERA_BUSY = false;
	ets_intr_unlock();
}

static int ICACHE_FLASH_ATTR camera_save_config()
{
	struct camera_data data;

	os_memset(&data, 0, sizeof data);
	data.baud = baud_rate;
	data.gpio_rx = gpio_camera_rx;
	data.gpio_tx = gpio_camera_tx;

	if (DeviceConfig_set_data(&data, sizeof data) != 0) {
		ets_uart_printf("Failed to save camera config.\n");
		return -1;
	}

	ets_uart_printf("Saved camera config.\n");
	return 0;
}

int ICACHE_FLASH_ATTR Camera_reset()
{
	uint8 command[] = {'\x56', '\x00', '\x26', '\x00'};
	uint8 success[] = {'\x76', '\x00', '\x26', '\x00'};
	struct rx_buffer *reset_buffer;
	int clock;

	ets_uart_printf("Resetting camera...\n");
	
	/* Add this delay to make sure the camera will be ready...
	   We had strange issues where the camera would not respond
	   depending on whether or not some line was added/commented out
	   in a function that never gets called to this point. We think
	   the line may have somehow caused the code to run faster, thus causing
	   bit_bang_send to send the reset command too fast... */
	os_delay_us(10);

	reset_buffer = create_rx_buffer(RESET_RESPONSE_SIZE, NULL);

	if (reset_buffer == NULL) {
		ets_uart_printf("reset_buffer was NULL.\n");
		return -1;
	}

	set_rx_buffer(reset_buffer);
	enable_interrupts();
	bit_bang_send(command, sizeof command);

	clock = NOW();

	/* wait until buffer is full or there is a timeout */
	while (!read_buffer_full()) {
		if ((NOW() - clock) > TOTICKS(CAMERA_RESPONSE_TIMEOUT)) {
			ets_uart_printf("Camera response timeout.\n");
			disable_interrupts();
			destroy_rx_buffer(reset_buffer);
			return 1;
		}
	}

	print_rx_buffer(reset_buffer);
 
	if (os_memcmp(reset_buffer->buf, success, sizeof success) != 0) {
		ets_uart_printf("in memcmp error\n");
		destroy_rx_buffer(reset_buffer);
		return 1;
	}

	destroy_rx_buffer(reset_buffer);
	return 0;
}

int ICACHE_FLASH_ATTR Camera_take_picture()
{
	uint8 command[] = {'\x56', '\x00', '\x36', '\x01', '\x00'};
	uint8 success[] = {'\x76', '\x00', '\x36', '\x00', '\x00'};
	struct rx_buffer *take_picture_buffer;
	int clock;

	take_picture_buffer = create_rx_buffer(TAKE_PICTURE_RESPONSE_SIZE, NULL);

	if (take_picture_buffer == NULL) {
		return -1;
	}

	ets_uart_printf("Taking picture...\n");
	set_rx_buffer(take_picture_buffer);
	enable_interrupts();
	bit_bang_send(command, sizeof command);

	clock = NOW();

	/* wait until buffer is full or there is a timeout */
	while (!read_buffer_full()) {
		if ((NOW() - clock) > TOTICKS(CAMERA_RESPONSE_TIMEOUT)) {
			ets_uart_printf("Camera response timeout.\n");
			disable_interrupts();
			destroy_rx_buffer(take_picture_buffer);
			return 1;
		}
	}

	print_rx_buffer(take_picture_buffer);

	if (os_memcmp(take_picture_buffer->buf, success, sizeof success) != 0) {
		destroy_rx_buffer(take_picture_buffer);
		return 1;
	}

	destroy_rx_buffer(take_picture_buffer);
	return 0;
}

int ICACHE_FLASH_ATTR Camera_read_size(uint16 *size_p)
{
	uint8 command[] = {'\x56', '\x00', '\x34', '\x01', '\x00'};
	uint8 success[] = {'\x76', '\x00', '\x34', '\x00', '\x04', '\x00', '\x00'};
	struct rx_buffer *read_size_buffer;
	int clock;

	read_size_buffer = create_rx_buffer(READ_SIZE_RESPONSE_SIZE, NULL);

	if (read_size_buffer == NULL) {
		return -1;
	}

	ets_uart_printf("Reading picture size...\n");
	set_rx_buffer(read_size_buffer);
	enable_interrupts();
	bit_bang_send(command, sizeof command);

	clock = NOW();

	/* wait until buffer is full or there is a timeout */
	while (!read_buffer_full()) {
		if ((NOW() - clock) > TOTICKS(CAMERA_RESPONSE_TIMEOUT)) {
			ets_uart_printf("Camera response timeout.\n");
			disable_interrupts();
			destroy_rx_buffer(read_size_buffer);
			return 1;
		}
	}

	print_rx_buffer(read_size_buffer);

	if (os_memcmp(read_size_buffer->buf, success, sizeof success) != 0) {
		destroy_rx_buffer(read_size_buffer);
		return 1;
	}

	*size_p = ((read_size_buffer->buf[7]) << 8) | (read_size_buffer->buf[8]);
	destroy_rx_buffer(read_size_buffer);
	return 0;
}

int ICACHE_FLASH_ATTR Camera_read_content(uint16 init_addr, uint16 data_len,
						uint16 spacing_int, struct espconn *rep_conn)
{
	uint8 command[] = {'\x56', '\x00', '\x32', '\x0c', '\x00', '\x0a', '\x00', '\x00', \
			   '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00'};
	uint8 success[] = {'\x76', '\x00', '\x32', '\x00', '\x00'};
	struct rx_buffer *read_content_buffer;

	/* In case the previous request was never finished, need to free it here */
	if (previous_rxbuffer != NULL)
		destroy_rx_buffer(previous_rxbuffer);

	command[8] = (uint8)((init_addr >> 8) & 0xff);
	command[9] = (uint8)(init_addr & 0xff);
	command[12] = (uint8)((data_len >> 8) & 0xff);
	command[13] = (uint8)(data_len & 0xff);
	command[14] = (uint8)((spacing_int >> 8) & 0xff);
	command[15] = (uint8)(spacing_int & 0xff);

	reply_conn = rep_conn;
	read_content_buffer = create_rx_buffer(2 * READ_CONTENT_RESPONSE_SIZE + data_len, camera_picture_recv_done);

	if (read_content_buffer == NULL) {
		return -1;
	}

	ets_uart_printf("Reading picture contents...\n");
	set_rx_buffer(read_content_buffer);
	enable_interrupts();
	bit_bang_send(command, sizeof command);
	previous_rxbuffer = read_content_buffer;

	os_timer_disarm(&unlock_timer);
	os_timer_setfn(&unlock_timer, cancel_read_picture, NULL);
	os_timer_arm(&unlock_timer, READ_PICTURE_CONTENTS_TIMEOUT, 0);

	/* Do not unlock the camera here. It will be done after the
	   camera has received all data, or the timeout has occured. */
	ets_uart_printf("Free heap size = %u\n", system_get_free_heap_size());
	return 0;
}

int ICACHE_FLASH_ATTR Camera_stop_pictures()
{
	uint8 command[] = {'\x56', '\x00', '\x36', '\x01', '\x03'};
	uint8 success[] = {'\x76', '\x00', '\x36', '\x00', '\x00'};
	struct rx_buffer *stop_pictures_buffer;
	int clock;

	stop_pictures_buffer = create_rx_buffer(STOP_PICTURES_RESPONSE_SIZE, NULL);

	if (stop_pictures_buffer == NULL)
		return -1;

	ets_uart_printf("Stop taking pictures...\n");
	set_rx_buffer(stop_pictures_buffer);
	enable_interrupts();
	bit_bang_send(command, sizeof command);

	clock = NOW();

	/* wait until buffer is full or there is a timeout */
	while (!read_buffer_full()) {
		if ((NOW() - clock) > TOTICKS(CAMERA_RESPONSE_TIMEOUT)) {
			ets_uart_printf("Camera response timeout.\n");
			disable_interrupts();
			destroy_rx_buffer(stop_pictures_buffer);
			return 1;
		}
	}

	print_rx_buffer(stop_pictures_buffer);

	if (os_memcmp(stop_pictures_buffer->buf, success, sizeof success) != 0) {
		destroy_rx_buffer(stop_pictures_buffer);
		return 1;
	}

	destroy_rx_buffer(stop_pictures_buffer);
	return 0;
}

int ICACHE_FLASH_ATTR Camera_compression_ratio(uint8 ratio)
{
	uint8 command[] = {'\x56', '\x00', '\x31', '\x05', '\x01', '\x01', '\x12', '\x04', '\x00'};
	uint8 success[] = {'\x76', '\x00', '\x31', '\x00', '\x00'};
	struct rx_buffer *compression_ratio_buffer;
	int clock;

	command[8] = ratio;
	compression_ratio_buffer = create_rx_buffer(COMPRESSION_RATIO_RESPONSE_SIZE, NULL);

	if (compression_ratio_buffer == NULL)
		return -1;

	ets_uart_printf("Set compression ratio to %d...\n", ratio);
	set_rx_buffer(compression_ratio_buffer);
	enable_interrupts();
	bit_bang_send(command, sizeof command);

	clock = NOW();

	/* wait until buffer is full or there is a timeout */
	while (!read_buffer_full()) {
		if ((NOW() - clock) > TOTICKS(CAMERA_RESPONSE_TIMEOUT)) {
			ets_uart_printf("Camera response timeout.\n");
			disable_interrupts();
			destroy_rx_buffer(compression_ratio_buffer);
			return 1;
		}
	}

	print_rx_buffer(compression_ratio_buffer);

	if (os_memcmp(compression_ratio_buffer->buf, success, sizeof success) != 0) {
		ets_uart_printf("Wrong compression response???\n");
		destroy_rx_buffer(compression_ratio_buffer);
		return 1;
	}

	destroy_rx_buffer(compression_ratio_buffer);
	return 0;
}

int ICACHE_FLASH_ATTR Camera_set_image_size(uint8 size)
{
	uint8 command[] = {'\x56', '\x00', '\x54', '\x01', '\x00'};
	uint8 success[] = {'\x76', '\x00', '\x54', '\x00', '\x00'};
	struct rx_buffer *set_image_size_buffer;
	int clock;

	if (size > 2) {
		ets_uart_printf("Invalid size value: %d\n", size);
		return -1;
	}

	command[4] = size * 0x11;
	set_image_size_buffer = create_rx_buffer(SET_IMAGE_SIZE_RESPONSE_SIZE, NULL);

	if (set_image_size_buffer == NULL) {
		return -1;
	}

	ets_uart_printf("Set image size to %s...\n", size == 0 ? "640 x 480" : (size == 1 ? "320 x 240" : "160 x 120"));
	set_rx_buffer(set_image_size_buffer);
	enable_interrupts();
	bit_bang_send(command, sizeof command);

	clock = NOW();

	/* wait until buffer is full or there is a timeout */
	while (!read_buffer_full()) {
		if ((NOW() - clock) > TOTICKS(CAMERA_RESPONSE_TIMEOUT)) {
			ets_uart_printf("Camera response timeout.\n");
			disable_interrupts();
			destroy_rx_buffer(set_image_size_buffer);
			return 1;
		}
	}

	print_rx_buffer(set_image_size_buffer);

	if (os_memcmp(set_image_size_buffer->buf, success, sizeof success) != 0) {
		ets_uart_printf("Wrong set_image_size response.\n");
		destroy_rx_buffer(set_image_size_buffer);
		return 1;
	}

	destroy_rx_buffer(set_image_size_buffer);
//	Camera_reset();	/* Need to reset the camera for this to take effect. */
	return 0;
}

int ICACHE_FLASH_ATTR Camera_power_saving_on()
{
	uint8 command[] = {'\x56', '\x00', '\x3e', '\x03', '\x00', '\x01', '\x01'};
	uint8 success[] = {'\x76', '\x00', '\x3e', '\x00', '\x00'};
	struct rx_buffer *power_saving_on_buffer;
	int clock;

	power_saving_on_buffer = create_rx_buffer(POWER_SAVING_ON_RESPONSE_SIZE, NULL);

	if (power_saving_on_buffer == NULL)
		return -1;

	ets_uart_printf("Power saving on...\n");
	set_rx_buffer(power_saving_on_buffer);
	enable_interrupts();
	bit_bang_send(command, sizeof command);

	clock = NOW();

	/* wait until buffer is full or there is a timeout */
	while (!read_buffer_full()) {
		if ((NOW() - clock) > TOTICKS(CAMERA_RESPONSE_TIMEOUT)) {
			ets_uart_printf("Camera response timeout.\n");
			disable_interrupts();
			destroy_rx_buffer(power_saving_on_buffer);
			return 1;
		}
	}

	print_rx_buffer(power_saving_on_buffer);

	if (os_memcmp(power_saving_on_buffer->buf, success, sizeof success) != 0) {
		destroy_rx_buffer(power_saving_on_buffer);
		return 1;
	}

	destroy_rx_buffer(power_saving_on_buffer);
	return 0;
}

int ICACHE_FLASH_ATTR Camera_power_saving_off()
{
	uint8 command[] = {'\x56', '\x00', '\x3e', '\x03', '\x00', '\x01', '\x00'};
	uint8 success[] = {'\x76', '\x00', '\x3e', '\x00', '\x00'};
	struct rx_buffer *power_saving_off_buffer;
	int clock;

	power_saving_off_buffer = create_rx_buffer(POWER_SAVING_OFF_RESPONSE_SIZE, NULL);

	if (power_saving_off_buffer == NULL)
		return -1;

	ets_uart_printf("Power saving off...\n");
	set_rx_buffer(power_saving_off_buffer);
	enable_interrupts();
	bit_bang_send(command, sizeof command);

	clock = NOW();

	/* wait until buffer is full or there is a timeout */
	while (!read_buffer_full()) {
		if ((NOW() - clock) > TOTICKS(CAMERA_RESPONSE_TIMEOUT)) {
			ets_uart_printf("Camera response timeout.\n");
			disable_interrupts();
			destroy_rx_buffer(power_saving_off_buffer);
			return 1;
		}
	}

	print_rx_buffer(power_saving_off_buffer);

	if (os_memcmp(power_saving_off_buffer->buf, success, sizeof success) != 0) {
		destroy_rx_buffer(power_saving_off_buffer);
		return 1;
	}

	destroy_rx_buffer(power_saving_off_buffer);
	return 0;
}

int ICACHE_FLASH_ATTR Camera_set_baud_rate(uint32 baud)
{
	uint8 command[] = {'\x56', '\x00', '\x24', '\x03', '\x01', '\x00', '\x00'};
	uint8 success[] = {'\x76', '\x00', '\x24', '\x00', '\x00'};
	struct rx_buffer *set_baud_rate_buffer;
	int clock;

	switch (baud) {
		case 9600:	command[5] = '\xae';
			   	command[6] = '\xc8';
			   	break;

		case 19200:	command[5] = '\x56';
				command[6] = '\xe4';
				break;

		case 38400:	command[5] = '\x2a';
				command[6] = '\xf2';
				break;

		case 57600:	command[5] = '\x1c';
				command[6] = '\x4c';
				break;

		case 115200:	command[5] = '\x0d';
				command[6] = '\xa6';
				break;

		default:	ets_uart_printf("Camera baud rate not supported.\n");
				return;
	}

	set_baud_rate_buffer = create_rx_buffer(SET_BAUD_RATE_RESPONSE_SIZE, NULL);

	if (set_baud_rate_buffer == NULL)
		return -1;

	ets_uart_printf("Set baud rate to %u...\n", baud);
	set_rx_buffer(set_baud_rate_buffer);
	enable_interrupts();
	bit_bang_send(command, sizeof command);

	clock = NOW();

	/* This might mess with the timing... */
	baud_rate = baud;
	software_uart_config(baud_rate, gpio_camera_rx, gpio_camera_tx);

	/* wait until buffer is full or there is a timeout */
	while (!read_buffer_full()) {
		if ((NOW() - clock) > TOTICKS(CAMERA_RESPONSE_TIMEOUT)) {
			ets_uart_printf("Camera response timeout.\n");
			disable_interrupts();
			destroy_rx_buffer(set_baud_rate_buffer);
			return 1;
		}
	}

	print_rx_buffer(set_baud_rate_buffer);

	if (os_memcmp(set_baud_rate_buffer->buf, success, sizeof success) != 0) {
		destroy_rx_buffer(set_baud_rate_buffer);
		return 1;
	}

	destroy_rx_buffer(set_baud_rate_buffer);

	if (camera_save_config() != 0) {
		ets_uart_printf("Failed to save new baud rate.\n");
		return 1;
	}

	return 0;
}

int ICACHE_FLASH_ATTR Camera_init(struct DeviceConfig *conf)
{
	struct camera_data *dat;

	if (conf->data_len != sizeof (struct camera_data)) {
		ets_uart_printf("Wrong size for camera data: %d\n", conf->data_len);
		return -1;
	}

	dat = (struct camera_data *)(conf->data);

	gpio_init();
	baud_rate = dat->baud;
	gpio_camera_rx = dat->gpio_rx;
	gpio_camera_tx = dat->gpio_tx;

	/* Need to fix the PIN FUNC SELECT constants before this is supported... */
	if (gpio_camera_rx != 5 || gpio_camera_tx != 4) {
		ets_uart_printf("Unsupported values for gpio_camera_rx, gpio_camera_tx\n");
		return -1;
	}

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO4_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);

	GPIO_DIS_OUTPUT(gpio_camera_rx);

	software_uart_config(baud_rate, gpio_camera_rx, gpio_camera_tx);

	ets_uart_printf("Camera initialized. Rx: %d Tx: %d Baud: %d\n", gpio_camera_rx, gpio_camera_tx, baud_rate);
	ets_intr_lock();
	IS_CAMERA_BUSY = false;
	ets_intr_unlock();
	return 0;
}

int ICACHE_FLASH_ATTR Camera_set_default_data(struct DeviceConfig *config)
{
	config->data = (void *)os_zalloc(sizeof (struct camera_data));

	if (config->data == NULL) {
		ets_uart_printf("Failed to allocate memory for config->data.\n");
		return -1;
	}

	config->data_len = sizeof (struct camera_data);
	os_memcpy(config->data, &DEFAULT_CAMERA_DATA, sizeof (struct camera_data));

	return 0;
}

bool ICACHE_FLASH_ATTR Camera_is_busy()
{
	return IS_CAMERA_BUSY;
}

void ICACHE_FLASH_ATTR Camera_set_busy()
{
	IS_CAMERA_BUSY = true;
}

void ICACHE_FLASH_ATTR Camera_unset_busy()
{
	IS_CAMERA_BUSY = false;
}
