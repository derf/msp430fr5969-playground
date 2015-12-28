#include "uart.h"

void check_command(unsigned char argc, char** argv);

void uart_putchar(char c)
{
	while (!(UCA0IFG & UCTXIFG));
	UCA0TXBUF = c;

	if (c == '\n')
		uart_putchar('\r');
}

void uart_putdigit(unsigned char digit)
{
	if (digit < 10)
		uart_putchar('0' + digit);
	else
		uart_putchar('A' + digit - 10);
}

void uart_puthex(unsigned char num)
{
	uart_putdigit(num >> 4);
	uart_putdigit(num & 0x0f);
}

void uart_putint(int num)
{
	if (num > 10000)
		uart_putdigit(num / 10000);
	if (num > 1000)
		uart_putdigit((num % 10000) / 1000);
	if (num > 100)
		uart_putdigit((num % 1000) / 100);
	if (num > 10)
		uart_putdigit((num % 100) / 10);

	uart_putdigit(num % 10);

}

void uart_putfloat(float num)
{
	if (num > 1000)
		uart_putdigit(((int)num % 10000) / 1000);
	if (num > 100)
		uart_putdigit(((int)num % 1000) / 100);
	if (num > 10)
		uart_putdigit(((int)num % 100) / 10);

	uart_putdigit((int)num % 10);
	uart_putchar('.');
	uart_putdigit((int)(num * 10) % 10);
	uart_putdigit((int)(num * 100) % 10);

}

void uart_puts(char* text)
{
	while (*text)
		uart_putchar(*text++);
}

void uart_puterr(char* text)
{
	uart_puts(COL_RED);
	uart_puts(text);
	uart_puts(COL_RESET);
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
			uart_puts(COL_YELLOW "msp430fr5969" COL_GREEN " > " COL_RESET);

		} else if (buf == '\f') {

			uart_puts("\n" COL_YELLOW "msp430fr5969" COL_GREEN " > " COL_RESET);
			uart_nputs(prompt, prompt_pos);

		} else if (buf == 0x7f) {

			if (prompt_pos) {
				prompt_pos--;
				uart_puts("\e[D \e[D");
			}

		} else if (buf == 0x15) { // ^U

			for ( ; prompt_pos > 0; prompt_pos-- )
				uart_puts("\e[D \e[D");
			*prompt = 0;

		} else if (buf == 0x17) { // ^W

			for ( ; (prompt_pos > 0) && (prompt[prompt_pos] != ' '); prompt_pos-- )
				uart_puts("\e[D \e[D");
			for ( ; (prompt_pos > 0) && (prompt[prompt_pos-1] == ' '); prompt_pos-- )
				uart_puts("\e[D \e[D");
			prompt[prompt_pos] = 0;

		} else if (buf >= ' ') {

			if (prompt_pos < sizeof(prompt)-1) {
				prompt[prompt_pos++] = buf;
				uart_putchar(buf);
			}
		}
	}
}
