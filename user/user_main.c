#include "c_types.h"
#include "osapi.h"
#include "ets_sys.h"
#include "user_interface.h"


#include "gpio.h"



void ICACHE_FLASH_ATTR read(void){

	int data[100];
	int i=0;
    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

    GPIO_OUTPUT_SET(2, 1);
    ets_uart_printf("GPIO2 after output set, delay for 0.25s: %d\n",GPIO_INPUT_GET(2)); //READS pin 1=high 3.3V
    os_delay_us(250000);
    GPIO_OUTPUT_SET(2, 0);
    ets_uart_printf("GPIO2 after output set, delay for 20ms: %d\n",GPIO_INPUT_GET(2)); //READS pin 1=high 3.3V
    os_delay_us(20000);
    GPIO_OUTPUT_SET(2, 1);
    ets_uart_printf("GPIO2 after output set, delay for 40us: %d\n",GPIO_INPUT_GET(2)); //READS pin 1=high 3.3V
    os_delay_us(40);
    GPIO_DIS_OUTPUT(2);
    ets_uart_printf("GPIO2 after output set, before pullup: %d\n",GPIO_INPUT_GET(2)); //READS pin 1=high 3.3V
    PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO2_U);
    ets_uart_printf("GPIO2 after output set, after pullup: %d\n",GPIO_INPUT_GET(2)); //READS pin 1=high 3.3V

    // wait for pin to drop?
    while (GPIO_INPUT_GET(2) == 1 && i<100000) {
          os_delay_us(1);
          i++;
    }

    if(i==100000)
      return;
  	ets_uart_printf("DEVICE READY TO COMMUNICATE\n");
}

void ICACHE_FLASH_ATTR user_init()
{

	// static ETSTimer wifi_connect_timer;

	// system_restore();
	system_set_os_print(0);
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	
	//ENABLE the power on GPIO2 3.3V
	//defined in eagle_soc.h PERIPHS_IO_MUX_GPIO2_U --> (PERIPHS_IO_MUX + 0x38)
	//FUNC_GPIO2 -->  0
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2); 
	ets_uart_printf("GPIO2 after PIN_FUNC_SELECT: %d\n",GPIO_INPUT_GET(BIT2)); //READS pin 1=high 3.3V
    PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO2_U);	
	ets_uart_printf("GPIO2 after PIN_PULLUP_EN: %d\n",GPIO_INPUT_GET(BIT2)); //READS pin 1=high 3.3V
	read();
}

