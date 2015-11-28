#include <msp430.h>
#include <string.h>

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
		UCB0IFG &= ~UCTXIFG0;
		UCB0IFG &= ~UCRXIFG0;
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

	//UCB0CTL1 |= UCTXSTP;

	while (UCB0CTL1 & UCTXSTP);
	return 0;
}

void check_command(unsigned char argc, char** argv)
{
	unsigned char i2c_rxbuf[16];
	unsigned char i2c_txbuf[16];
	if (!strcmp(argv[0], "i2c")) {
		if (argc == 0) {
			uart_puts("Usage: i2c <on|off|detect|gettemp> [-u]\n");
			return;
		}
		if (!strcmp(argv[1], "on")) {
			if ((argc >= 2) && !strcmp(argv[2], "-u")) {
				if (i2c_setup(1) < 0)
					uart_puts("Error initializing I²C: Line is busy\n");
			} else {
				if (i2c_setup(0) < 0)
					uart_puts("Error initializing I²C: Line is busy\n"
							"Do you have hardware pullups on SDA/SCL?\n");
			}
		} else if (!strcmp(argv[1], "off")) {
			uart_puts("Error: not implemented yet\n");
		} else if (!strcmp(argv[1], "detect")) {
			i2c_scan();
		} else if (!strcmp(argv[1], "gettemp")) {
			i2c_txbuf[0] = 0x00;
			i2c_xmit(0x4d, 1, 1, i2c_txbuf, i2c_rxbuf);
			uart_puthex(i2c_rxbuf[0]);
			uart_putchar('\n');
		}
	} else {
		uart_puts("Unknown command\n");
	}
}

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;

	P1DIR = BIT0;
	P1OUT = BIT0;
	P4DIR = BIT6;
	P4OUT = 0;

	PJSEL0 = BIT4 | BIT5;

	PM5CTL0 &= ~LOCKLPM5;

	FRCTL0 = FWPW;
	FRCTL0_L = 0x10;
	FRCTL0_H = 0xff;

	// 16MHz DCO
	CSCTL0_H = CSKEY >> 8;
	CSCTL1 = DCORSEL | DCOFSEL_4;
	CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
	CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;
	CSCTL0_H = 0;

	// enable LXFT for RTC
	CSCTL0_H = CSKEY >> 8;
	CSCTL4 &= ~LFXTOFF;
	while (SFRIFG1 & OFIFG) {
		CSCTL5 &= ~LFXTOFFG;
		SFRIFG1 &= ~OFIFG;
	}
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
	static char prompt[64];
	static unsigned int prompt_pos = 0;

	char buf;
	unsigned char raw_p_pos, parse_p_pos;

	char parsed_prompt[64];
	unsigned char argc = 0;
	char *argv[32];

	if (UCA0IFG & UCRXIFG) {
		buf = UCA0RXBUF;
		if (buf == '\r') {

			uart_putchar('\n');
			if (prompt_pos > 0) {

				parse_p_pos = 0;
				argv[0] = parsed_prompt;

				for (raw_p_pos = 0; raw_p_pos < prompt_pos; raw_p_pos++) {
					if (prompt[raw_p_pos] != ' ') {
						parsed_prompt[parse_p_pos++] = prompt[raw_p_pos];
					} else if ((raw_p_pos > 0) && (prompt[raw_p_pos-1] != ' ')) {
						argc++;
						parsed_prompt[parse_p_pos++] = 0;
						argv[argc] = parsed_prompt + parse_p_pos;
					}
				}

				if (parse_p_pos < 64)
					parsed_prompt[parse_p_pos] = 0;
				else
					parsed_prompt[63] = 0;

				check_command(argc, argv);
				prompt_pos = 0;
				*prompt = 0;
			}
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

			if (prompt_pos < sizeof(prompt)-1) {
				prompt[prompt_pos++] = buf;
				uart_putchar(buf);
			}
		}
	}
}
