#include "port.h"
#include "mb.h"
#include "mbport.h"
#include "usart.h"      /* CubeMX ??? huart2 */
#include <string.h>

extern UART_HandleTypeDef huart2;
uint8_t rx_byte;

/* ============================================================
 *  ??/?? ????
 * ============================================================ */
void vMBPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable)
{
    /* 注意: 这里不能使用 HAL_UART_Receive_IT()。
     * USART2_IRQHandler 是手动处理中断的, HAL 的接收状态机(RxState)
     * 永远不会被推进到 READY, 第二次使能时会返回 HAL_BUSY,
     * 导致 RXNE 中断被永久关闭, 串口收一帧后就"没反应"。
     * 因此直接操作寄存器使能/禁止中断。 */
    if (xRxEnable) {
        __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
        __HAL_UART_ENABLE_IT(&huart2, UART_IT_ERR);
    } else {
        __HAL_UART_DISABLE_IT(&huart2, UART_IT_RXNE);
        __HAL_UART_DISABLE_IT(&huart2, UART_IT_ERR);
    }

    if (xTxEnable) {
        __HAL_UART_ENABLE_IT(&huart2, UART_IT_TXE);
    } else {
        __HAL_UART_DISABLE_IT(&huart2, UART_IT_TXE);
    }
}

/* ============================================================
 *  ?????
 *  huart2 ?? CubeMX ????,?????? TRUE
 * ============================================================ */
BOOL xMBPortSerialInit(UCHAR ucPORT, ULONG ulBaudRate,
                       UCHAR ucDataBits, eMBParity eParity)
{
    (void)ucPORT;
    (void)ulBaudRate;
    (void)ucDataBits;
    (void)eParity;
    return TRUE;
}

/* ============================================================
 *  ??????(??? DR)
 * ============================================================ */
BOOL xMBPortSerialPutByte(CHAR ucByte)
{
    /* 等待发送数据寄存器空, 防止覆盖上一个尚未移出的字节 */
    while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TXE) == RESET)
    {
    }
    huart2.Instance->DR = (UCHAR)ucByte;
    return TRUE;
}

/* ============================================================
 *  ??????(??? DR)
 * ============================================================ */
BOOL xMBPortSerialGetByte(CHAR *pucByte)
{
    *pucByte = (CHAR)(huart2.Instance->DR & 0xFF);
    return TRUE;
}

/* ============================================================
 *  ????:? stm32f1xx_it.c ?? USART2_IRQHandler ??
 * ============================================================ */
void prvvUARTTxReadyISR(void)
{
    //HAL_UART_Transmit(&huart2, (uint8_t*)"T", 1, 100);   /* ??????,??? 'T' */
    pxMBFrameCBTransmitterEmpty();
}

void prvvUARTRxISR(void)
{
     //HAL_UART_Transmit(&huart2, (uint8_t*)"R", 1, 100);   /* ??????,??? 'R' */
    pxMBFrameCBByteReceived();
}


