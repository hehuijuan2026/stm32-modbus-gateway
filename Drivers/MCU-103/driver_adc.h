#ifndef _DRIVER_ADC_H
#define _DRIVER_ADC_H

#include <stdint.h>

int ADC_Light_Init(void);
uint16_t ADC_Light_Read(void);
uint16_t ADC_Light_Read_Filtered(void);

#endif /* _DRIVER_ADC_H */