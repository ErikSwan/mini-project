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
#include "answer12.h"
#include <math.h>

#define TRUE 1
#define FALSE 0

/* 
the data structure to hold all the changing inputs coming
from the board via SPI
*/

typedef struct {
	uint8 speed; 	// slide bar potentiometers, slave 1
	uint8 gain;  	// slide bar potentiometers, slave 1
	uint8 high_eq;	// knobs, slave 1
	uint8 mid_eq;	// knobs, slave 1
	uint8 low_eq;	// knobs, slave 1
	uint8 fx1;		// knobs, slave 1
	uint8 fx2;		// knobs, slave 1
	uint8 fx3;		// knobs, slave 1
	uint8 encoder;	// turntable encoder count, slave 1
	uint8 next;		// buttons, slave 2
	uint8 prev;		// buttons, slave 2
	uint8 play_pause; // buttons, slave 2
	uint8 fx_onoff;	// buttons, slave 2
} InputData;

int main(int argc, char * * argv)
{
    // main thread

	// create the SPI thread
	pthread_t * spithread = malloc(sizeof(pthread_t));
	InputData * data = malloc(sizeof(InputData));
	pthread_create(&spithread, NULL,  thread_sampling, &data);

	// extract samples from .wav files

    // DSP processing code goes here

    // send buffers of processed data to DAC
    
	// free data before ending main
	free(spithread);
	free(data);
    return 0;
}

// entry function for the sampling thread
void * thread_sampling(void * data)
{
	// typecast the "data" structure that's coming in
	InputData * inputs = (InputData*)data;
	
	// add more code here to update values via SPI

	return NULL;
}