#ifndef _DRIVER_DHT11_H
#define _DRIVER_DHT11_H

#include "main.h"
#include "cmsis_os.h"

/* DHT11 ?? */
#define DHT11_GPIO_PORT     GPIOB
#define DHT11_GPIO_PIN      GPIO_PIN_12

/* DHT11 ??? */
#define DHT11_OK                0
#define DHT11_ERR_TIMEOUT      -1
#define DHT11_ERR_CHECKSUM     -2

/* DHT11 ???? */
typedef struct {
    uint8_t temperature;
    uint8_t humidity;
    uint8_t checksum;
} DHT11_Data;

/* ?????? */
typedef struct {
    DHT11_Data data;
    int        status;
} DHT11_Msg;

/* ????(? driver_dht11.c ???) */
extern DHT11_Data          g_dht11_data;
extern osMessageQueueId_t  g_dht11_queue;
extern osMutexId_t         g_dht11_mutex;
extern osMutexId_t         g_usart_mutex;

/* ???? */
int  Dht11_Init(void);
int  Dht11_Read(DHT11_Data *data);
int  Dht11_ReadSafe(DHT11_Data *data);
int  Dht11_QueueInit(void);

void Dht11_Task(void *argument);
void Dht11_ProcessTask(void *argument);
void Led_BlinkTask(void *argument);

#endif /* _DRIVER_DHT11_H */
