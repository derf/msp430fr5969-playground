#ifndef UART_H
#define UART_H

#include <msp430.h>
#include <string.h>

#define COL_BLACK "\e[0;30m"
#define COL_RED "\e[0;31m"
#define COL_GREEN "\e[0;32m"
#define COL_YELLOW "\e[0;33m"
#define COL_BLUE "\e[0;34m"
#define COL_MAGENTA "\e[0;35m"
#define COL_CYAN "\e[0;36m"
#define COL_WHITE "\e[0;37m"
#define COL_RESET "\e[0m"

void uart_putchar(char c);

void uart_putdigit(unsigned char digit);

void uart_puthex(unsigned char num);

void uart_putint(int num);

void uart_putfloat(float num);

void uart_puts(char* text);

void uart_puterr(char* text);

void uart_nputs(char *text, int len);

void uart_setup(void);

#endif
