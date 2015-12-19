#ifndef ADC_H
#define ADC_H

#include <msp430.h>
#include <string.h>

#define CALADC12_12V_30C  *((unsigned int *)0x1A1A)
#define CALADC12_12V_85C  *((unsigned int *)0x1A1C)

float adc_gettemp(void);

float adc_getvcc(void);

#endif
