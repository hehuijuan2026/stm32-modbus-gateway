#include "driver_usart.h"
#include "usart.h"
#include <string.h>

extern UART_HandleTypeDef huart1;   /* CubeMX????? */

int Usart_SendString(const char *str)
{
    uint16_t len = 0;

    if (str == NULL)
    {
        return -1;
    }
    len = strlen(str);
    if (HAL_UART_Transmit(&huart1, (uint8_t *)str, len, 100) != HAL_OK)
    {
        return -2;
    }
    return 0;
}

int Usart_Test(void)
{
    while (1)
    {
        Usart_SendString("Hello from STM32\r\n");
        mdelay(1000);
    }
    return 0;
}