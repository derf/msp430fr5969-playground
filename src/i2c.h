#ifndef I2C_H
#define I2C_H

#include <msp430.h>
#include <string.h>

int i2c_setup(int enable_pullups);

void i2c_scan(void);

int i2c_xmit(unsigned char slave_addr, unsigned char tx_bytes, unsigned char
		rx_bytes, unsigned char* tx_buf, unsigned char* rx_buf);

#endif
