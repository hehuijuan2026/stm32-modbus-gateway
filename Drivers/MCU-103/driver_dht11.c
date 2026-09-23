#include "driver_dht11.h"
#include "driver_usart.h"
#include "driver_timer.h"
#include "driver_led.h"
#include "driver_adc.h"
#include "driver_modbus_reg.h"
#include <stdio.h>
#include <string.h>

extern void Modbus_Poll(void);

/* ============================================================
 * 全局变量
 * ============================================================ */
DHT11_Data g_dht11_data = {0};

osMessageQueueId_t g_dht11_queue = NULL;
osMutexId_t        g_dht11_mutex = NULL;
osMutexId_t        g_usart_mutex = NULL;

/* LED 告警闪烁计数 */
static volatile int g_led_blink_count = 0;

/* ============================================================
 * DWT 延时（DHT11 时序需要微秒级）
 * ============================================================ */
static void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void DWT_Delay_us(uint32_t us)
{
    uint32_t start  = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < cycles);
}

/* ============================================================
 * GPIO 模式切换
 * ============================================================ */
static void DHT11_SetOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

static void DHT11_SetInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

/* ============================================================
 * 等待指定电平，超时返回-1
 * ============================================================ */
static int DHT11_WaitForLevel(GPIO_PinState level, uint32_t timeout_us)
{
    uint32_t start  = DWT->CYCCNT;
    uint32_t cycles = timeout_us * (SystemCoreClock / 1000000U);
    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) != level) {
        if ((DWT->CYCCNT - start) > cycles) return -1;
    }
    return 0;
}

/* ============================================================
 * 读一个字节（MSB 先出）
 * ============================================================ */
static uint8_t DHT11_ReadByte(void)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        if (DHT11_WaitForLevel(GPIO_PIN_SET, 100) != 0) return 0;

        DWT_Delay_us(40);
        if (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET) {
            byte |= (1 << (7 - i));
            if (DHT11_WaitForLevel(GPIO_PIN_RESET, 100) != 0) return 0;
        }
    }
    return byte;
}

/* ============================================================
 * DHT11 初始化
 * ============================================================ */
int Dht11_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    DWT_Init();

    GPIO_InitStruct.Pin   = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);

    return 0;
}

/* ============================================================
 * 读 DHT11
 * 返回 0 成功，-1 超时，-2 校验失败
 * ============================================================ */
int Dht11_Read(DHT11_Data *data)
{
    uint8_t buf[5] = {0};

    if (data == NULL) return -1;

    /* 1. 主机拉低 20ms 以上 */
    DHT11_SetOutput();
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_RESET);
    osDelay(20);
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
    DWT_Delay_us(30);

    /* 2. 切输入，等 DHT11 响应 */
    DHT11_SetInput();

    if (DHT11_WaitForLevel(GPIO_PIN_RESET, 100) != 0) return -1;
    if (DHT11_WaitForLevel(GPIO_PIN_SET,   100) != 0) return -1;
    if (DHT11_WaitForLevel(GPIO_PIN_RESET, 100) != 0) return -1;

    /* 3. 读 5 字节 */
    for (int i = 0; i < 5; i++) {
        buf[i] = DHT11_ReadByte();
    }

    /* 4. 释放总线 */
    DHT11_SetOutput();
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);

    /* 5. 校验 */
    uint8_t checksum = buf[0] + buf[1] + buf[2] + buf[3];
    if (checksum != buf[4]) return -2;

    data->humidity    = buf[0];
    data->temperature = buf[2];
    data->checksum    = buf[4];

    return 0;
}

/* ============================================================
 * 带互斥锁的 DHT11 读取
 * ============================================================ */
int Dht11_ReadSafe(DHT11_Data *data)
{
    int ret;

    if (g_dht11_mutex == NULL) {
        return Dht11_Read(data);
    }

    osMutexAcquire(g_dht11_mutex, osWaitForever);
    ret = Dht11_Read(data);
    osMutexRelease(g_dht11_mutex);

    return ret;
}

/* ============================================================
 * 带互斥锁的串口打印
 * ============================================================ */
static void Usart_SendString_Safe(const char *s)
{
    if (g_usart_mutex == NULL) {
        Usart_SendString(s);
        return;
    }
    osMutexAcquire(g_usart_mutex, osWaitForever);
    Usart_SendString(s);
    osMutexRelease(g_usart_mutex);
}

/* ============================================================
 * 队列 + 互斥锁 + 寄存器初始化
 * 在 MX_FREERTOS_Init() 里调用
 * ============================================================ */
int Dht11_QueueInit(void)
{
    /* 0. 初始化 Modbus 寄存器 */
    ModbusReg_Init();

    /* 1. 创建互斥锁 */
    g_dht11_mutex = osMutexNew(NULL);
    g_usart_mutex = osMutexNew(NULL);
    if (g_dht11_mutex == NULL || g_usart_mutex == NULL) return -1;

    /* 2. 创建队列 */
    g_dht11_queue = osMessageQueueNew(16, sizeof(DHT11_Msg), NULL);
    if (g_dht11_queue == NULL) return -2;

    /* 3. 初始化外设 */
    Dht11_Init();
    Led_Init();
    ADC_Light_Init();

    return 0;
}

/* ============================================================
 * DHT11 采集任务
 * 由 StartTask02 调用
 * ============================================================ */
void Dht11_Task(void *argument)
{
    DHT11_Msg msg;
    uint16_t light;
    (void)argument;

    for (;;)
    {
        msg.status = Dht11_ReadSafe(&msg.data);

        if (msg.status == DHT11_OK) {
            g_dht11_data = msg.data;

            /* 写入 Modbus 寄存器 */
            ModbusReg_SetHolding(REG_TEMP, msg.data.temperature);
            ModbusReg_SetHolding(REG_HUMI, msg.data.humidity);

            /* 读光敏电阻（滑动平均） */
            light = ADC_Light_Read_Filtered();
            ModbusReg_SetHolding(REG_LIGHT, light);

            /* 告警判断 */
            if (msg.data.temperature > 30 || light < 500) {
                ModbusReg_SetHolding(REG_ALARM, 1);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
            } else {
                ModbusReg_SetHolding(REG_ALARM, 0);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
            }

            /* 队列满则丢旧帧 */
            if (osMessageQueuePut(g_dht11_queue, &msg, 0, 0) != osOK) {
                DHT11_Msg dummy;
                osMessageQueueGet(g_dht11_queue, &dummy, NULL, 0);
                osMessageQueuePut(g_dht11_queue, &msg, 0, 0);
            }
        }
        osDelay(2000);
    }
}

/* ============================================================
 * DHT11 处理任务：打印 + 告警闪烁
 * 由 StartTask03 调用
 * ============================================================ */
void Dht11_ProcessTask(void *argument)
{
    DHT11_Msg msg;
    char buf[64];
    (void)argument;

    for (;;)
    {
        if (osMessageQueueGet(g_dht11_queue, &msg, NULL, 10) == osOK)
        {
            if (msg.status == DHT11_OK)
            {
                g_led_blink_count = 0;
                Led_Control(LED_GREEN, 1);

                snprintf(buf, sizeof(buf),
                         "DHT11 OK: Temp = %d C, Humi = %d %%RH\r\n",
                         msg.data.temperature, msg.data.humidity);
                Usart_SendString_Safe(buf);
            }
            else
            {
                g_led_blink_count = 8;
                Usart_SendString_Safe(msg.status == DHT11_ERR_CHECKSUM
                                      ? "DHT11 checksum error\r\n"
                                      : "DHT11 timeout\r\n");
            }
        }

        Modbus_Poll();   /* ← 加这行，每1ms轮询一次Modbus队列 */

        osDelay(1);
    }
}

/* ============================================================
 * LED 告警闪烁任务
 * ============================================================ */
void Led_BlinkTask(void *argument)
{
    (void)argument;

    for (;;)
    {
        if (g_led_blink_count > 0)
        {
            Led_Control(LED_GREEN, 1);
            osDelay(500);
            g_led_blink_count--;

            Led_Control(LED_GREEN, 0);
            osDelay(500);
            g_led_blink_count--;
        }
        else
        {
            osDelay(100);
        }
    }
}
