#ifndef SPI_H
#define SPI_H

#include <msp430.h>
#include <string.h>

int spi_setup();

int spi_xmit(unsigned char tx_bytes, unsigned char rx_bytes,
		unsigned char* tx_buf, unsigned char* rx_buf);

int spi_send(unsigned char tx_bytes, unsigned char* tx_buf);

#endif
