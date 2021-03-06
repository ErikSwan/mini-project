// spi.c
//
// Example program for bcm2835 library
// Shows how to interface with SPI to transfer a byte to and from an SPI device
//
// After installing bcm2835, you can build this 
// with something like:
// gcc -o spi spi.c -l bcm2835
// sudo ./spi
//
// Or you can test it before installing with:
// gcc -o spi -I ../../src ../../src/bcm2835.c spi.c
// sudo ./spi
//
// Author: Mike McCauley
// Copyright (C) 2012 Mike McCauley
// $Id: RF22.h,v 1.21 2012/05/30 01:51:25 mikem Exp $

#include <bcm2835.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    // If you call this, it will not actually access the GPIO
// Use for testing
//        bcm2835_set_debug(1);

      if (!bcm2835_init())
	return 1;
    int i=0;
    uint8_t send_data[11] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0,0xB0};
    uint8_t rec_data[11] = {0,0,0,0,0,0,0,0,0,0};
    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_128); // The default
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default
    uint8_t read_data = 0;
    // Send a byte to the slave and simultaneously read a byte back from the slave
    // If you tie MISO to MOSI, you should read back what was sent
  
   // read_data = bcm2835_spi_transfer(send_data[0]);

    while(1){
     for (i=0;i<11;i++){
        read_data = bcm2835_spi_transfer(send_data[i]);
        //usleep(1000);
        printf("Sent to SPI: 0x%02X. Read back from SPI: 0x%02X.\n", send_data[i], read_data);
        rec_data[i] = read_data;
        printf("Received data = 0x%02X\n",read_data); 
        read_data = 0;
     }
     usleep(2000);
    }
    //read_data = bcm2835_spi_transfer(send_data[0]);
    //printf("Track no = 0x%02X\n",read_data);
//   }
//}
    
    
   usleep(250000);
  //}
  //  if (send_data != read_data)
    //  printf("Do you have the loopback from MOSI to MISO connected?\n");
    bcm2835_spi_end();
    bcm2835_close();
    return 0;
}
