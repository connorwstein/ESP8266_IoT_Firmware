#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include "c_types.h"
#include "espconn.h"

#define INITIAL_DELAY 2000000
#define INITIATE_PROTOCOL_DELAY 20000
#define MAX_READ_TIME 4000
#define READ_BUFFER_SIZE=42; //buffer for the 40 data bits, with some extra space for the initial response signals
#define DATA_BIT_HIGH_THRESHOLD=18; //number of ones in a row that indicates a bit high, 
//note that this depends on the code in the while loop of the read function, if that loop takes longer to execute i.e. code is added
//to it, this value may change
void ICACHE_FLASH_ATTR Temperature_set_temperature(int temp);
void ICACHE_FLASH_ATTR Temperature_get_temperature(struct espconn *conn);

#endif
