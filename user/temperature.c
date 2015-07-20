#include "c_types.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "eagle_soc.h"
#include "gpio.h"
#include "debug.h"
#include "mem.h"
#include "user_interface.h"

#include "device_config.h"

#define INITIAL_DELAY 200000 
#ifndef HIGH 
#define HIGH 1
#endif
#ifndef LOW 
#define LOW 0
#endif
#define GPIO2 2
#define T_BE 20000 //Tbe in data sheet
#define T_GO 40 //Tgo in data sheet
#define MAX_READ_TIME 4000
#define PROTOCOL_BITS 1 //received bits to ignore (part of the protocol and not the data, varies on the chips due to timing, might be 2 on some chips)
#define READ_BUFFER_SIZE 42 //buffer for the 40 data bits, with some extra space for the initial response signals
#define DATA_BIT_HIGH_THRESHOLD 18 //number of ones in a row that indicates a bit high, 
//note that this depends on the code in the while loop of the read function, if that loop takes longer to execute i.e. code is added
//to it, this value may change


typedef struct _dht{
	unsigned short humidity;
	unsigned short temperature;
	bool negative_temp;
}dht;

static dht DHT={0,0,false};


void ICACHE_FLASH_ATTR set_dht(unsigned char* data){
	int i;
	//Skip the first 2, just initiating the communication
	//The first bit is Tbe --> Tgo
	//Second bit is Trel --> Treh
	//40 bytes representing 40 bits--> 40 bits i.e. 5 bytes
	unsigned short data_compressed[2];
	memset(data_compressed,0,sizeof data_compressed);
	data+=1;
	if(data[0]==1){
		DHT.negative_temp=true;
		ets_uart_printf("Negative temperature detected\n");
		data[0]=0; //flip the sign bit
	}
	//10 = 8 bits of the checksum and 2 bits of the initial protocol are not data bits
	for(i=0;i<READ_BUFFER_SIZE-10;i++){
		data_compressed[i/16]<<=1;
		data_compressed[i/16]|=data[i];
	}
	DHT.humidity=data_compressed[0];
	DHT.temperature=data_compressed[1];
	ets_uart_printf("Humidity: %d Temp: %d\n\n",data_compressed[0],data_compressed[1]);
}


bool ICACHE_FLASH_ATTR checksum(unsigned char *data){
	data+=1; //skip the first 2 protocol bits
	unsigned char data_bytes[5]; //40 data bits --> 5 bytes
	memset(data_bytes,0,sizeof data_bytes);
	int i;
	for(i=0;i<READ_BUFFER_SIZE-2;i++){
		data_bytes[i/8]<<=1;
		data_bytes[i/8]|=data[i];
	}
	if(data[0]+data[1]+data[2]+data[3] != data[4]){
		ets_uart_printf("Checksum not valid\n");
		return false;
	} 
	return true;
}

unsigned char* ICACHE_FLASH_ATTR get_data_bits(unsigned char *raw_data, int size){
	int i;
	//ets_uart_printf("\n\nDATA: ");
	unsigned char *data=(unsigned char*)os_zalloc(READ_BUFFER_SIZE*sizeof(char));
	memset(data,0,sizeof data);
	int ones_counter=0;
	int data_index=0;
	int data_bit_high_threshold=DATA_BIT_HIGH_THRESHOLD; //18 ones in a row or more and you have a 1
	bool ones_streak=false;
	ets_uart_printf("DATA: \n");
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
			//data_index++;
			ones_counter=0; //reset the counter
			ones_streak=false;
		}
	}
	ets_uart_printf("\n\n");
	return data;

}

int ICACHE_FLASH_ATTR read(void){
    int j = 0;
	int k;
	unsigned char *raw_data=(unsigned char*)os_zalloc(MAX_READ_TIME*sizeof(char));
    memset(raw_data,0,sizeof raw_data);
    GPIO_OUTPUT_SET(GPIO2, HIGH);
    os_delay_us(INITIAL_DELAY); //Keep it high for a while before initiating.
    GPIO_OUTPUT_SET(GPIO2, LOW);
    os_delay_us(T_BE); //Tbe in data sheet. Host the start signal down time. Using the max of 20ms.
    GPIO_OUTPUT_SET(GPIO2,HIGH); 
    os_delay_us(T_GO);
    GPIO_DIS_OUTPUT(GPIO2); //When you disable output mode (i.e. switch to input), the pull up resistor on the SDA data will cause the line to go high 
    //Wait for pin to drop, sensor takes over right here 
    //Now should send response signal that goes Low 80us then High 80us 
    //(the sensor response signal)
    //Then 40 bits will be sent
    //A data bit 1 = 50us low 26us high
    //A data bit 0 = 50us low 70us high
  	//ets_uart_printf("DEVICE READY TO COMMUNICATE\n");
  	//Maximum, 40 1s would be 120us *40 =4800us, 4800 sets off the watch dog however,
  	//just use 4000us (very unlikely to be all 1s anyways)
  	while(j++<MAX_READ_TIME){
  		raw_data[j]=0x1&GPIO_INPUT_GET(GPIO2);
  		os_delay_us(1);
  	}
  	// for(k=0;k<MAX_READ_TIME;k++){
  	//  	ets_uart_printf("%d",raw_data[k]);
  	//  }
  	unsigned char* data=get_data_bits(raw_data,MAX_READ_TIME);
  	if(!checksum(data)){
  		goto fail;
  	}
  	else{
  		set_dht(data);
  	}
  	ets_uart_printf("Freeing buffers\n");
  	os_free(raw_data);
  	os_free(data);
  	return 0;
    
    fail: 
    	ets_uart_printf("Failed to read from device\n");
    	return -1;
}


void ICACHE_FLASH_ATTR Temperature_get_temperature(struct espconn *conn)
{
	DEBUG("enter Temperature_get_temperature");
	//ENABLE the power on GPIO2 3.3V
	//defined in eagle_soc.h PERIPHS_IO_MUX_GPIO2_U --> (PERIPHS_IO_MUX + 0x38)
	//FUNC_GPIO2 -->  0
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2); 
    PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO2_U);	
	char buff[10];
	if(read()!=-1){
		if(DHT.negative_temp){
			//add negative sign
			os_sprintf(buff,"%d -%d",DHT.humidity,DHT.temperature);
		}
		else{
			os_sprintf(buff,"%d %d",DHT.humidity,DHT.temperature);
		}
		ets_uart_printf("Sending Humidity: %d Temperature: %d\n",DHT.humidity,DHT.temperature);
		espconn_sent(conn, (uint8 *)buff, 10);
	}
	else{
		ets_uart_printf("Error getting temperature from the device\n");
	}
}

int ICACHE_FLASH_ATTR Temperature_init(struct DeviceConfig *config)
{
	gpio_init();
	return 0;
}

int ICACHE_FLASH_ATTR Temperature_set_default_data(struct DeviceConfig *config)
{
	return 0;
}
