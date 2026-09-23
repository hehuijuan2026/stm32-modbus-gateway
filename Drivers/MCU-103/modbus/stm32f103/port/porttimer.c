#include "port.h"
#include "mb.h"
#include "mbport.h"
#include "usart.h"  
#include "tim.h"        /* CubeMX ??? htim3 */

extern TIM_HandleTypeDef htim3;

/* ============================================================
 *  ??????
 *  htim3 ?? CubeMX ????
 *  usTim1Timerout50us ? 50us ???
 * ============================================================ */
BOOL xMBPortTimersInit(USHORT usTim1Timerout50us)
{
    /* usTim1Timerout50us ? 50us ???
     * ?? 80 ?? 80 × 50us = 4ms
     * TIM3 ???? = 1MHz(? 1us ???)
     * ARR = usTim1Timerout50us × 50 - 1
     */
    __HAL_TIM_SET_PRESCALER(&htim3, 72 - 1);          /* 1MHz */
    __HAL_TIM_SET_AUTORELOAD(&htim3, usTim1Timerout50us * 50 - 1);
    return TRUE;
}
/* ============================================================
 *  ?????
 * ============================================================ */
void vMBPortTimersEnable(void)
{
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim3, TIM_IT_UPDATE);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    __HAL_TIM_ENABLE(&htim3);
}

/* ============================================================
 *  ?????
 * ============================================================ */
void vMBPortTimersDisable(void)
{
    __HAL_TIM_DISABLE_IT(&htim3, TIM_IT_UPDATE);
    __HAL_TIM_DISABLE(&htim3);
}

/* ============================================================
 *  vMBPortTimersDelay: needed by mb.c for Modbus ASCII mode
 * ============================================================ */
void vMBPortTimersDelay(USHORT usTimeOutMS)
{
    HAL_Delay(usTimeOutMS);
}

/* ============================================================
 *  ????:? stm32f1xx_it.c ?? TIM3_IRQHandler ??
 * ============================================================ */
void prvvTIMERExpiredISR(void)
{
    (void)pxMBPortCBTimerExpired();
}
