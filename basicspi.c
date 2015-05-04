#include <bcm2835.h>
#include <stdio.h>
#include <stdlib.h>

main(){
    if (!bcm2835_init()) return 1;

    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_128);
   // while(1){
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

    uint8_t send_data = 0x10;
    uint8_t read_data = bcm2835_spi_transfer(send_data);
    printf("Sent to SPI:0x%02X, rad back: 0x%02X\n", send_data, read_data);
    
    bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW);
    send_data = 0x30;
    read_data = bcm2835_spi_transfer(send_data);
    printf("READ BACK FROM 2: 0x%02X\n", read_data);
    
   // usleep(250000);
   // }
    
    bcm2835_spi_end();
    bcm2835_close();

    return 0;

}
