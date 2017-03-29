#include "spi.h"
#include "uart.h"

int spi_setup()
{
	P2SEL0 &= ~BIT2;
	P2SEL1 |= BIT2;
	P2DIR |= BIT2;

	P1SEL0 &= ~BIT6;
	P1SEL1 |= BIT6;
	P1DIR |= BIT6;

	P1SEL0 &= ~BIT7;
	P1SEL1 |= BIT7;
	P1DIR &= ~BIT7;
	P1REN &= ~BIT7;

	UCB0CTLW0 = UCCKPH | UCMSB | UCMST | UCSYNC | UCMODE_0 | UCSSEL__SMCLK | UCSWRST;
	UCB0BRW = 32;
	UCB0CTLW0 &= ~UCSWRST;

	return 0;
}

static inline unsigned char clean_rxbuf()
{
	return UCB0RXBUF;
}

int spi_xmit(unsigned char tx_bytes, unsigned char rx_bytes,
		unsigned char* tx_buf, unsigned char* rx_buf)
{
	UCB0IE &= ~(UCTXIE | UCRXIE);

	while (UCB0STATW & UCBUSY) ;

	clean_rxbuf();

	for (int i = 0; i < tx_bytes; i++) {
		UCB0TXBUF = tx_buf[i];

		if (UCB0IFG & UCRXIFG) {
			if (rx_buf != NULL && i < rx_bytes)
				rx_buf[i] = UCB0RXBUF;
			else
				clean_rxbuf();
		}
	}

	return 0;
}
