#include <msp430.h>
#include <string.h>
#include "adc.h"
#include "i2c.h"
#include "spi.h"
#include "uart.h"

void check_command(unsigned char argc, char** argv)
{
	unsigned char i2c_rxbuf[16];
	unsigned char i2c_txbuf[16];
	float buf = 0;
	int i;
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
		} else if (!strcmp(argv[1], "tc74")) {
			i2c_txbuf[0] = 0x00;
			i2c_xmit(0x4d, 1, 1, i2c_txbuf, i2c_rxbuf);
			uart_putint(i2c_rxbuf[0]);
			uart_puts("°C\n");
		} else if (!strcmp(argv[1], "lm75")) {
			i2c_txbuf[0] = 0x00;
			i2c_xmit(0x48, 1, 2, i2c_txbuf, i2c_rxbuf);
			uart_putfloat(i2c_rxbuf[0] + (i2c_rxbuf[1] / 256.0));
			uart_puts("°C\n");
		}
		else if (!strcmp(argv[1], "eepr")) {
			i2c_rxbuf[0] = 0;
			i2c_txbuf[0] = 0;
			i2c_txbuf[1] = argv[2][0];
			i2c_xmit(0x50, 2, 1, i2c_txbuf, i2c_rxbuf);
			uart_putint(i2c_rxbuf[0]);
			uart_puts("\n");
		}
		else if (!strcmp(argv[1], "eepw")) {
			i2c_txbuf[0] = 0;
			i2c_txbuf[1] = argv[2][0];
			i2c_txbuf[2] = argv[3][0];
			i2c_txbuf[3] = argv[3][1];
			i2c_txbuf[4] = argv[3][2];
			i2c_txbuf[5] = argv[3][3];
			i2c_txbuf[6] = argv[3][4];
			i2c_txbuf[7] = argv[3][5];
			i2c_xmit(0x50, 8, 0, i2c_txbuf, i2c_rxbuf);
		}
	} else if (!strcmp(argv[0], "sensors")) {
		for (i = 0; i < 32; i++) {
			buf += adc_gettemp();
			__delay_cycles(64000);
		}
		uart_puts("Temperature : ");
		uart_putfloat(buf / 32);
		uart_puts("°C avg / ");
		uart_putfloat(adc_gettemp());
		uart_puts("°C single\n    Voltage : ");
		uart_putfloat(adc_getvcc());
		uart_puts("V\n");
	} else if (!strcmp(argv[0], "spi")) {
		if (argc == 0) {
			uart_puterr("Usage: spi <on|off|sharp>\n");
			return;
		}
		if (!strcmp(argv[1], "on")) {
			spi_setup();
		} else if (!strcmp(argv[1], "sharp")) {
			P2DIR |= BIT4; // CS
			P4DIR |= BIT2; // PWR
			P4DIR |= BIT3; // EN
			P2OUT &= ~BIT4;
			P4OUT |= BIT2;
			P4OUT |= BIT3;
			P2OUT |= BIT4;
			spi_xmit(3, 0, (unsigned char *)"\x20\x00\x00", NULL);
			P2OUT &= ~BIT4;
		}
	} else if (!strcmp(argv[0], "help")) {
		uart_puts("Supported commands: i2c sensors\n");
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
