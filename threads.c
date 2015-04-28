/*
Because threads within the same process share resources:

    Changes made by one thread to shared system resources (such as closing a file) will be seen by all other threads.
    Two pointers having the same value point to the same data.
    Reading and writing to the same memory locations is possible, and therefore requires explicit synchronization by the programmer. 

When compiling the program, you will also need to add -lpthread to the compile command. ie: gcc program.c -o program -lpthread

pthread_create arguments:

    thread: An opaque, unique identifier for the new thread returned by the subroutine.
    attr: An opaque attribute object that may be used to set thread attributes. You can specify a thread attributes object, or NULL for the default values.
    start_routine: the C routine that the thread will execute once it is created.
    arg: A single argument that may be passed to start_routine. It must be passed by reference as a pointer cast of type void. NULL may be used if no argument is to be passed. 

https://computing.llnl.gov/tutorials/pthreads/

http://timmurphy.org/2010/05/04/pthreads-in-c-a-minimal-working-example/

*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

#define TRUE 1
#define FALSE 0

/* 
the data structure to hold all the changing inputs coming
from the board via SPI
*/

typedef struct InputData {
	float speed; 	// slide bar potentiometers, slave 1
	int gain;  	// slide bar potentiometers, slave 1
	//uint8 high_eq;	// knobs, slave 1
	//uint8 mid_eq;	// knobs, slave 1
	//uint8 low_eq;	// knobs, slave 1
	//uint8 fx1;		// knobs, slave 1
	//uint8 fx2;		// knobs, slave 1
	//uint8 fx3;		// knobs, slave 1
	//uint8 encoder;	// turntable encoder count, slave 1
	//uint8 next;		// buttons, slave 2
	//uint8 prev;		// buttons, slave 2
	int play_pause; // buttons, slave 2
	//uint8 fx_onoff;	// buttons, slave 2
} InputData;

InputData data = {1,5,1};
int flag = 0;

void * thread_sampling(void * unused);

int main(int argc, char * * argv)
{
    // main thread declarations
	printf("INITIAL VALUES:\ndata.speed = %.1f\n",data.speed);
	printf("data.gain = %d\n",data.gain);
	printf("data.play_pause = %d\n",data.play_pause);
	
	// create the SPI thread
	pthread_t * spithread = malloc(sizeof(pthread_t));
	pthread_create(spithread, NULL,  thread_sampling, NULL);

	// DSP
	
	
	while (1) {
		if (flag == 1){
			flag = 0;
			printf("data.speed = %.1f\n",data.speed);
			printf("data.gain = %d\n",data.gain);
			printf("data.play_pause = %d\n",data.play_pause);
		}
	}

	// free data before ending main
	free(spithread);
    return 0;
}

// entry function for the sampling thread
void * thread_sampling(void * unused)
{
	// DECLARATIONS
	char c = ' ';
	
	while (1) {
		c = getc(stdin);
		getc(stdin);
		
		// CONDITIONAL ADJUSTMENTS	
		if (c == 'p'){
			data.play_pause = !(data.play_pause);
			flag = 1;
		} else if (c == 'f'){
			data.speed = data.speed - .1;
			flag = 1;
		} else if (c == 's'){
			data.speed = data.speed + .1;
			flag = 1;
		} else if (c == 'u'){
			data.gain++;
			flag = 1;
		} else if (c == 'd'){
			data.gain--;
			flag = 1;
		}
		
		/*
		printf("data.speed = %.1f\n",data.speed);
		printf("data.gain = %d\n",data.gain);
		printf("data.play_pause = %d\n",data.play_pause);
		*/
	}

	return NULL;
}