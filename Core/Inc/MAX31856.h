#ifndef MAX31856_H
#define MAX31856_H

#include <stdint.h>
#include "main.h"

#define FAULT_OPEN 10000
#define FAULT_VOLTAGE 10001
#define NO_MAX31856 10002

#define CELSIUS 0
#define FAHRENHEIT 1

#define NUM_REGISTERS 12

// Register constants
#define READ_OPERATION(x) ((x) & 0x7F)
#define WRITE_OPERATION(x) ((x) | 0x80)

typedef struct
{
    uint8_t registers[NUM_REGISTERS];

    HAL_StatusTypeDef (*spi_tx)(SPI_HandleTypeDef *hspi,
                                const uint8_t *pData,
                                uint16_t Size,
                                uint32_t Timeout);

    HAL_StatusTypeDef (*spi_rx)(SPI_HandleTypeDef *hspi,
                                uint8_t *pData,
                                uint16_t Size,
                                uint32_t Timeout);

    SPI_HandleTypeDef *s;

} MAX31856_t;

#ifdef __cplusplus
extern "C"
{
#endif

    void max31856_init(MAX31856_t *dev);
    void max31856_write_register(MAX31856_t *dev, uint8_t reg, uint8_t value, uint8_t n);

    uint8_t tcswitch(uint8_t n, uint8_t dir);

    uint32_t max31856_read_thermocouple(MAX31856_t *dev, uint8_t n);
    double max31856_read_junction(MAX31856_t *dev, uint8_t unit, uint8_t n);

#ifdef __cplusplus
}
#endif

#endif
