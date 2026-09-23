#include "driver_adc.h"
#include "stm32f1xx_hal.h"

extern ADC_HandleTypeDef hadc1;

#define FILTER_N 10
static uint16_t filter_buf[FILTER_N] = {0};
static uint8_t  filter_idx = 0;

int ADC_Light_Init(void)
{
    return 0;
}

uint16_t ADC_Light_Read(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    return (uint16_t)HAL_ADC_GetValue(&hadc1);
}

uint16_t ADC_Light_Read_Filtered(void)
{
    uint32_t sum = 0;

    filter_buf[filter_idx] = ADC_Light_Read();
    filter_idx = (filter_idx + 1) % FILTER_N;

    for (uint8_t i = 0; i < FILTER_N; i++)
    {
        sum += filter_buf[i];
    }
    return sum / FILTER_N;
}