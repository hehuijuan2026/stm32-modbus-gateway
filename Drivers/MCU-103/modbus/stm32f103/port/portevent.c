#include "port.h"
#include "mb.h"
#include "mbport.h"
#include "cmsis_os.h"

/* FreeModbus ????:EV_READY?EV_FRAME_RECEIVED?EV_EXECUTE?EV_FRAME_SENT */
static osEventFlagsId_t eMBEvents = NULL;

/* ============================================================
 *  ?????
 * ============================================================ */
BOOL xMBPortEventInit(void)
{
    eMBEvents = osEventFlagsNew(NULL);
    return (eMBEvents != NULL);
}

/* ============================================================
 *  事件发布(在中断上下文调用, CMSIS-RTOS2 的 osEventFlagsSet 内部自动处理 ISR)
 * ============================================================ */
BOOL xMBPortEventPost(eMBEventType eEvent)
{
    osEventFlagsSet(eMBEvents, (1UL << eEvent));
    return TRUE;
}

/* ============================================================
 *  ????(? Modbus_Task ???)
 * ============================================================ */
BOOL xMBPortEventGet(eMBEventType *eEvent)
{
    uint32_t flags = osEventFlagsWait(eMBEvents, 0x0F, osFlagsWaitAny, osWaitForever);

    if (flags & (1UL << EV_READY))               *eEvent = EV_READY;
    else if (flags & (1UL << EV_FRAME_RECEIVED)) *eEvent = EV_FRAME_RECEIVED;
    else if (flags & (1UL << EV_EXECUTE))        *eEvent = EV_EXECUTE;
    else if (flags & (1UL << EV_FRAME_SENT))     *eEvent = EV_FRAME_SENT;

    osEventFlagsClear(eMBEvents, (1UL << *eEvent));
    return TRUE;
}