#include <msp430.h>
#include <string.h>

volatile char prompt[64];
volatile unsigned int prompt_pos = 0;

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

void uart_puts(char* text)
{
	while (*text)
		uart_putchar(*text++);
}

void uart_nputs(char *text, int len)
{
	int i;
	for (i = 0; i < len; i++)
		uart_putchar(text[i]);
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

int i2c_setup()
{
	UCB0CTL1 = UCSWRST;
	UCB0CTLW0 = UCMODE_3 | UCMST | UCSYNC | UCSSEL_2 | UCSWRST | UCCLTO_1;
	UCB0BRW = 0xf00;
	P1DIR &= ~(BIT6 | BIT7);
	P1SEL0 &= ~(BIT6 | BIT7);
	P1SEL1 |= BIT6 | BIT7;

	// use internal pull-ups
	P1OUT |= (BIT6 | BIT7);
	P1REN |= (BIT6 | BIT7);

	UCB0CTL1 &= ~UCSWRST;
	UCB0I2CSA = 0;

	if (UCB0STAT & UCBBUSY)
		return -1;
	return 0;
}

void i2c_scan()
{
	int slave_addr;

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
}

void check_command()
{
	if (!strcmp(prompt, "i2cdetect")) {
		if (i2c_setup() < 0) {
			uart_puts("Error initalizing I²C: Bus is busy\n");
			return;
		}
		i2c_scan();
	}
}

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;

	P1DIR = BIT0;
	P1OUT = BIT0;
	P4DIR = BIT6;
	P4OUT = 0;

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
	uart_puts("\nmsp430fr5969 > ");

	__eint();
	__bis_SR_register(LPM0_bits); // should not return

	P4OUT |= BIT6;
	return 0;
}

#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
	char buf;
	if (UCA0IFG & UCRXIFG) {
		buf = UCA0RXBUF;
		if (buf == '\r') {

			uart_putchar('\n');
			check_command();
			prompt_pos = 0;
			*prompt = 0;
			uart_puts("msp430fr5969 > ");

		} else if (buf == '\f') {

			uart_puts("\nmsp430fr5969 > ");
			uart_nputs(prompt, prompt_pos);

		} else if (buf == 0x7f) {

			if (prompt_pos) {
				prompt_pos--;
				uart_puts("\e[D \e[D");
			}

		} else if (buf >= ' ') {

			if (prompt_pos < sizeof(prompt)) {
				prompt[prompt_pos++] = buf;
				uart_putchar(buf);
			}
		}
	}
}
