#include <msp430.h>

void uart_putchar(char c)
{
	while (!(UCA0IFG & UCTXIFG));
	UCA0TXBUF = c;

	if (c == '\n')
		uart_putchar('\r');
}

void uart_puthex(unsigned char num)
{
	int tmp;

	tmp = (num & 0xf0) >> 4;
	if (tmp < 10)
		uart_putchar('0' + tmp);
	else
		uart_putchar('A' + tmp - 10);

	tmp = num & 0x0f;
	if (tmp < 10)
		uart_putchar('0' + tmp);
	else
		uart_putchar('A' + tmp - 10);
}

void uart_puts(const char* text)
{
	while (*text)
		uart_putchar(*text++);
}

void uart_setup(void)
{
	UCA0CTLW0 = UCSWRST | UCSSEL__SMCLK;
	UCA0MCTLW = UCOS16 | (2<<5) | 0xD600;
	UCA0BR0 = 104;

	UCA0IRCTL = 0;
	UCA0ABCTL = 0;

	P2SEL0 &= ~(BIT0 | BIT1);
	P2SEL1 |= BIT0 | BIT1;
	P2DIR |= BIT0;

	UCA0CTLW0 &= ~UCSWRST;

	UCA0IE |= UCRXIE;
}

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;

	P1DIR = BIT0;
	P1OUT = BIT0;
	P4DIR = BIT6;
	P4OUT = BIT6;

	PM5CTL0 &= ~LOCKLPM5;

	FRCTL0 = FWPW;
	FRCTL0_L = 0x10;
	FRCTL0_H = 0xff;

	CSCTL0_H = CSKEY >> 8;
	CSCTL1 = DCORSEL | DCOFSEL_4;
	CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
	CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;
	CSCTL0_H = 0;

	__delay_cycles(1000000);
	P1OUT = 0;
	P4OUT = 0;

	uart_setup();
	uart_puts("=^.^=\n");

	__eint();

	while (1) {
		__delay_cycles(1600000);
		P4OUT = 0;
		P1OUT = BIT0;
		__delay_cycles(160000);
		P4OUT = 0;
		P1OUT = 0;
		__delay_cycles(16000000);
		P4OUT = 0;
		__delay_cycles(160000);
		P4OUT = 0;
	}
}

#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
	char buf;
	if (UCA0IFG & UCRXIFG) {
		P4OUT |= BIT6;
		buf = UCA0RXBUF;
		if (buf == '\r')
			uart_putchar('\n');
		else
			uart_putchar(buf);
	}
}
