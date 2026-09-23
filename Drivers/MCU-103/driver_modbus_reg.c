#include "driver_modbus_reg.h"
#include <string.h>

/* ========== ?????:16?,??16? ========== */
#define REG_HOLDING_SIZE    16
static uint16_t usHolding[REG_HOLDING_SIZE];

/* ========== ?????:16?,??16? ========== */
#define REG_INPUT_SIZE      16
static uint16_t usInput[REG_INPUT_SIZE];

/* ========== ??? ========== */
void ModbusReg_Init(void)
{
    memset(usHolding, 0, sizeof(usHolding));
    memset(usInput,   0, sizeof(usInput));
}

/* ========== ? ========== */
void ModbusReg_SetInput(uint16_t index, uint16_t value)
{
    if (index < REG_INPUT_SIZE) {
        usInput[index] = value;
    }
}

void ModbusReg_SetHolding(uint16_t index, uint16_t value)
{
    if (index < REG_HOLDING_SIZE) {
        usHolding[index] = value;
    }
}

/* ========== ? ========== */
uint16_t ModbusReg_GetInput(uint16_t index)
{
    return (index < REG_INPUT_SIZE) ? usInput[index] : 0;
}

uint16_t ModbusReg_GetHolding(uint16_t index)
{
    return (index < REG_HOLDING_SIZE) ? usHolding[index] : 0;
}
