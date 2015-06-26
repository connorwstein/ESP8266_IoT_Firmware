#include <unistd.h>

int main(void){

	while(1){
		fork();
		fork();
	}

	return 0;
}


