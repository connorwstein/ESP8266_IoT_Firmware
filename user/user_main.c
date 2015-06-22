#include "c_types.h"
#include "osapi.h"
#include "ets_sys.h"
#include "user_interface.h"
#include "os_type.h"
#include "gpio.h"
#include "mem.h"

static int READ_BUFFER_SIZE=42; //buffer for the 40 data bits, with some extra space for the initial response signals
static int DATA_BIT_HIGH_THRESHOLD=18; //number of ones in a row that indicates a bit high, 
//note that this depends on the code in the while loop of the read function, if that loop takes longer to execute i.e. code is added
//to it, this value may change

void print_dht(unsigned char* data){
	int i;
	//Skip the first 2, just initiating the communication
	//The first bit is Tbe --> Tgo
	//Second bit is Trel --> Treh
	//40 bytes representing 40 bits--> 40 bits i.e. 5 bytes
	unsigned short data_compressed[2];
	memset(data_compressed,0,sizeof data_compressed);
	data+=2;
	//10 = 8 bits of the checksum and 2 bits of the initial protocol are not data bits
	for(i=0;i<READ_BUFFER_SIZE-10;i++){
		data_compressed[i/16]<<=1;
		data_compressed[i/16]|=data[i];
	}
	ets_uart_printf("Humidity: %d Temp: %d\n\n",data_compressed[0],data_compressed[1]);
}


void checksum(unsigned char *data){
	data+=2; //skip the first 2 protocol bits
	unsigned char data_bytes[5];
	memset(data_bytes,0,sizeof data_bytes);
	int i;
	for(i=0;i<READ_BUFFER_SIZE-2;i++){
		data_bytes[i/8]<<=1;
		data_bytes[i/8]|=data[i];
	}
	if(data[0]+data[1]+data[2]+data[3] != data[4]){
		ets_uart_printf("Checksum not valid\n");
	} 
}
void get_data_bits(unsigned char *raw_data, int size){
	int i;
	ets_uart_printf("\n\nDATA: ");
	unsigned char data[READ_BUFFER_SIZE];
	memset(data,0,sizeof data);
	int ones_counter=0;
	int data_index=0;
	int data_bit_high_threshold=DATA_BIT_HIGH_THRESHOLD; //18 ones in a row or more and you have a 1
	bool ones_streak=false;
	for(i=0;i<size;i++){
		//loop through the array of ones and zeros
		if(data_index>=READ_BUFFER_SIZE) break;
		if(raw_data[i]==1){
			ones_streak=true;
			ones_counter++; //if you get a one will be a string of ones 
			continue; //keep looping until you get a zero
		}
		else if(ones_streak){
			//first zero after streak of ones
			if(ones_counter>data_bit_high_threshold){
				data[data_index]=1;
			}
			else{
				data[data_index]=0;
			}
			ets_uart_printf("%d",data[data_index++]);
			ones_counter=0; //reset the counter
			ones_streak=false;
		}
	}
	ets_uart_printf("\n\n");
	checksum(data);
	print_dht(data);

}

void ICACHE_FLASH_ATTR read(void){
	int counter = 0;
    int laststate = 1;
    int i = 0;
    int j = 0;
    int checksum = 0;
	int k;
	unsigned char *raw_data=(unsigned char*)os_zalloc(4000*sizeof(char));
    memset(raw_data,0,sizeof raw_data);
    // os_intr_lock();
    // wdt_feed();
    GPIO_OUTPUT_SET(2, 1);
    ets_uart_printf("GPIO2 %d for 20ms\n",GPIO_INPUT_GET(2)); //READS pin 1=high 3.3V
    os_delay_us(20000); //Keep it high for a while before initiating.
    GPIO_OUTPUT_SET(2, 0);
    ets_uart_printf("GPIO2 %d delay for 20ms\n",GPIO_INPUT_GET(2)); //READS pin 1=high 3.3V
    os_delay_us(20000); //Tbe in data sheet. Host the start signal down time. Using the max of 20ms.
    GPIO_DIS_OUTPUT(2); //When you disable output mode (i.e. switch to input), the pull up resistor on the SDA data will cause the line to go high 
    //ets_uart_printf("GPIO2 %d delay for 40us\n",GPIO_INPUT_GET(2)); //READS pin 1=high 3.3V
    //os_delay_us(40); //Tgo in data sheet. Bus master has released time
    //Wait for pin to drop, sensor takes over right here 
    //Now should send response signal that goes Low 80us then High 80us 
    //(the sensor response signal)
    //Then 40 bits will be sent
    //A data bit 1 = 50us low 26us high
    //A data bit 0 = 50us low 70us high
  	//ets_uart_printf("DEVICE READY TO COMMUNICATE\n");
  	//Maximum, 40 1s would be 120us *40 =4800us, 4800 sets off the watch dog however,
  	//just use 4000us (very unlikely to be all 1s anyways)
  	while(j++<4000){
  		raw_data[j]=GPIO_INPUT_GET(2);
  		os_delay_us(1);
  	}
  	// os_intr_unlock();
  	ets_uart_printf("DEVICE DONE\n");
  	for(k=0;k<4000;k++){
  		ets_uart_printf("%d",raw_data[k]);
  	}
  	get_data_bits(raw_data,4000);
  	return;
    fail: 
    	ets_uart_printf("Failed to read from device\n");
    	return;
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
	os_delay_us(2000000);
	read();
	os_delay_us(2000000);
	read();
	os_delay_us(2000001);
	read();

	// static ETSTimer dhtTimer;
 //  	os_timer_setfn(&dhtTimer, read, NULL);
 //  	os_timer_arm(&dhtTimer, 30000, 1);
}

