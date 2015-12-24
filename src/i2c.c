#include "i2c.h"
#include "uart.h"

int i2c_setup(int enable_pullups)
{
	UCB0CTL1 = UCSWRST;
	UCB0CTLW0 = UCMODE_3 | UCMST | UCSYNC | UCSSEL_2 | UCSWRST | UCCLTO_1;
	UCB0BRW = 0xf00;
	P1DIR &= ~(BIT6 | BIT7);
	P1SEL0 &= ~(BIT6 | BIT7);
	P1SEL1 |= BIT6 | BIT7;

	if (enable_pullups) {
		P1OUT |= (BIT6 | BIT7);
		P1REN |= (BIT6 | BIT7);
	}


	UCB0CTL1 &= ~UCSWRST;
	UCB0I2CSA = 0;

	__delay_cycles(1600);

	if (UCB0STAT & UCBBUSY)
		return -1;
	return 0;
}

void i2c_scan()
{
	unsigned char slave_addr;

	uart_puts("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");
	for (slave_addr = 0; slave_addr < 128; slave_addr++) {

		if ((slave_addr & 0x0f) == 0) {
			uart_putchar('\n');
			uart_puthex(slave_addr);
			uart_putchar(':');
		}

		UCB0I2CSA = slave_addr;
		UCB0CTL1 |= UCTR | UCTXSTT | UCTXSTP;

		while (UCB0CTL1 & UCTXSTP);

		if (UCB0IFG & UCNACKIFG) {
			uart_puts(" --");
			UCB0IFG &= ~UCNACKIFG;
		} else {
			uart_putchar(' ');
			uart_puthex(slave_addr);
		}
	}
	uart_puts("\n");
	UCB0IFG = 0;
}

int i2c_xmit(unsigned char slave_addr, unsigned char tx_bytes, unsigned char
		rx_bytes, unsigned char* tx_buf, unsigned char* rx_buf)
{
	int i;
	UCB0I2CSA = slave_addr;
	if (tx_bytes) {
		UCB0CTL1 |= UCTR | UCTXSTT;
		for (i = 0; i < tx_bytes; i++) {
			while (!(UCB0IFG & (UCTXIFG0 | UCNACKIFG | UCCLTOIFG)));
			if (UCB0IFG & (UCNACKIFG | UCCLTOIFG)) {
				UCB0IFG &= ~UCNACKIFG;
				UCB0IFG &= ~UCCLTOIFG;
				UCB0CTL1 |= UCTXSTP;
				return -1;
			}
			UCB0TXBUF = tx_buf[i];
		}
		while (!(UCB0IFG & (UCTXIFG0 | UCNACKIFG | UCCLTOIFG)));
		//if (UCB0IFG & (UCNACKIFG | UCCLTOIFG)) {
		//	UCB0IFG &= ~UCNACKIFG;
		//	UCB0IFG &= ~UCCLTOIFG;
		//	UCB0CTL1 |= UCTXSTP;
		//	return -1;
		//}
	}
	if (rx_bytes) {
		UCB0I2CSA = slave_addr;
		UCB0IFG = 0;
		UCB0CTL1 &= ~UCTR;
		UCB0CTL1 |= UCTXSTT;

		while (UCB0CTL1 & UCTXSTT);
		UCB0IFG &= ~UCTXIFG0;

		for (i = 0; i < rx_bytes; i++) {
			if (i == rx_bytes - 1)
				UCB0CTL1 |= UCTXSTP;
			while (!(UCB0IFG & (UCRXIFG0 | UCNACKIFG | UCCLTOIFG)));
			rx_buf[i] = UCB0RXBUF;
			UCB0IFG &= ~UCRXIFG0;
		}
		UCB0IFG &= ~UCRXIFG0;
	}

	UCB0CTL1 |= UCTXSTP;

	while (UCB0CTL1 & UCTXSTP);
	return 0;
}
