#ifndef _DRIVER_MODBUS_REG_H
#define _DRIVER_MODBUS_REG_H

#include <stdint.h>

/* ????? */
#define REG_TEMP     0
#define REG_HUMI     1
#define REG_LIGHT    2
#define REG_ALARM    3

/* ??? */
void ModbusReg_Init(void);

/* ???? */
void ModbusReg_SetInput(uint16_t index, uint16_t value);
void ModbusReg_SetHolding(uint16_t index, uint16_t value);

/* ???? */
uint16_t ModbusReg_GetInput(uint16_t index);
uint16_t ModbusReg_GetHolding(uint16_t index);

#endif /* _DRIVER_MODBUS_REG_H */
