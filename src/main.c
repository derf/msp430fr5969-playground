#include <msp430.h>
#include <string.h>
#include "adc.h"
#include "i2c.h"
#include "uart.h"

void check_command(unsigned char argc, char** argv)
{
	unsigned char i2c_rxbuf[16];
	unsigned char i2c_txbuf[16];
	if (!strcmp(argv[0], "i2c")) {
		if (argc == 0) {
			uart_puterr("Usage: i2c <on|off|detect|gettemp> [-u]\n");
			return;
		}
		if (!strcmp(argv[1], "on")) {
			if ((argc >= 2) && !strcmp(argv[2], "-u")) {
				if (i2c_setup(1) < 0)
					uart_puterr("Error initializing I²C: Line is busy\n");
			} else {
				if (i2c_setup(0) < 0)
					uart_puterr("Error initializing I²C: Line is busy\n"
							"Do you have hardware pullups on SDA/SCL?\n");
			}
		} else if (!strcmp(argv[1], "off")) {
			uart_puterr("Error: not implemented yet\n");
		} else if (!strcmp(argv[1], "detect")) {
			i2c_scan();
		} else if (!strcmp(argv[1], "gettemp")) {
			i2c_txbuf[0] = 0x00;
			i2c_xmit(0x4d, 1, 1, i2c_txbuf, i2c_rxbuf);
			uart_putint(i2c_rxbuf[0]);
			uart_puts("°C\n");
		}
	} else if (!strcmp(argv[0], "sensors")) {
		uart_puts("Temperature : ");
		uart_putfloat(adc_gettemp());
		uart_puts("°C\n    Voltage : ");
		uart_putfloat(adc_getvcc());
		uart_puts("V\n");
	} else {
		uart_puterr("Unknown command\n");
	}
}

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;

	P1DIR = BIT0;
	P1OUT = BIT0;
	P4DIR = BIT6;
	P4OUT = BIT6;

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
	uart_puts("\n" COL_YELLOW "msp430fr5969" COL_GREEN " > " COL_RESET);

	__eint();
	__bis_SR_register(LPM0_bits); // should not return

	P4OUT |= BIT6;
	return 0;
}
