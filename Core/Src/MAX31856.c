// MAX31856.c - Corrected SPI Read Implementation

#include "MAX31856.h"
#include "main.h"
#include <stdio.h>

// ---------------- HELPER FUNCTIONS ----------------

uint8_t tcswitch(uint8_t n, uint8_t dir)
{
    switch (n)
    {
    case 1:
        HAL_GPIO_WritePin(TC1_CS_GPIO_Port, TC1_CS_Pin, dir);
        break;
    case 2:
        HAL_GPIO_WritePin(TC2_CS_GPIO_Port, TC2_CS_Pin, dir);
        break;
    case 3:
        HAL_GPIO_WritePin(TC3_CS_GPIO_Port, TC3_CS_Pin, dir);
        break;
    case 4:
        HAL_GPIO_WritePin(TC4_CS_GPIO_Port, TC4_CS_Pin, dir);
        break;
    }

    return 0;
}

static double verify_device(MAX31856_t *dev, uint8_t n)
{
    uint8_t tx_buf[5] = {READ_OPERATION(0), 0, 0, 0, 0};
    uint8_t rx_buf[5];

    tcswitch(n, 0);
    HAL_Delay(1);
    HAL_SPI_TransmitReceive(dev->s, tx_buf, rx_buf, 5, HAL_MAX_DELAY);
    tcswitch(n, 1);

    uint32_t data = ((uint32_t)rx_buf[1] << 24) |
                    ((uint32_t)rx_buf[2] << 16) |
                    ((uint32_t)rx_buf[3] << 8) |
                    rx_buf[4];

    if (data == 0xFFFFFFFF)
        return NO_MAX31856;

    uint32_t expected =
        ((uint32_t)dev->registers[0] << 24) |
        ((uint32_t)dev->registers[1] << 16) |
        ((uint32_t)dev->registers[2] << 8) |
        dev->registers[3];

    if (expected == data)
        return 0;

    // Re-write registers
    uint8_t out[1 + NUM_REGISTERS];
    out[0] = WRITE_OPERATION(0);
    for (int i = 0; i < NUM_REGISTERS; i++)
        out[1 + i] = dev->registers[i];

    tcswitch(n, 0);
    dev->spi_tx(dev->s, out, sizeof(out), HAL_MAX_DELAY);
    tcswitch(n, 1);

    return NO_MAX31856;
}

// ---------------- PUBLIC API ----------------
void max31856_init(MAX31856_t *dev)
{
    // Default REGISTER shadow table
    uint8_t defaults[NUM_REGISTERS] =
        {0x00, 0x03, 0xff, 0x7f, 0xc0, 0x7f, 0xff, 0x80, 0, 0, 0, 0};
    for (int i = 0; i < NUM_REGISTERS; i++)
        dev->registers[i] = defaults[i];
}

void max31856_write_register(MAX31856_t *dev, uint8_t reg, uint8_t value, uint8_t n)
{
    if (reg >= NUM_REGISTERS)
        return;

    uint8_t out[2] = {WRITE_OPERATION(reg), value};
    tcswitch(n, 0);
    dev->spi_tx(dev->s, out, 2, HAL_MAX_DELAY);
    tcswitch(n, 1);
    dev->registers[reg] = value;
}

uint32_t max31856_read_thermocouple(MAX31856_t *dev, uint8_t n)
{
    uint8_t tx_buf[4] = {READ_OPERATION(0x0C), 0x00, 0x00, 0x00};
    uint8_t rx_buf[4];

    tcswitch(n, 0);
    HAL_SPI_TransmitReceive(dev->s, tx_buf, rx_buf, 4, HAL_MAX_DELAY);
    tcswitch(n, 1);

    // Data is in rx_buf[1], rx_buf[2], rx_buf[3]
    uint8_t high_byte = rx_buf[1]; // MSB (LTCBH) - Register 0x0C
    uint8_t mid_byte = rx_buf[2];  // Mid (LTCBM) - Register 0x0D
    uint8_t low_byte = rx_buf[3];  // LSB (LTCBL) - Register 0x0E

    // Assemble 24-bit value: HIGH << 16 | MID << 8 | LOW
    uint32_t temp_bytes = ((((uint32_t)high_byte & 0x7F) << 16) |
                           ((uint32_t)mid_byte << 8) |
                           (uint32_t)low_byte) >>
                          5;

    // Check sign bit (bit 7 of high_byte)
    int32_t signed_temp;
    if (high_byte & 0x80)
    {
        signed_temp = (int32_t)temp_bytes - 262144; // 2^18
    }
    else
    {
        signed_temp = (int32_t)temp_bytes;
    }

    int32_t scaled = (signed_temp * 625) / 8;
    return (uint32_t)scaled;
}

double max31856_read_junction(MAX31856_t *dev, uint8_t unit, uint8_t n)
{
    uint8_t tx_buf[5] = {READ_OPERATION(8), 0, 0, 0, 0};
    uint8_t rx_buf[5];

    tcswitch(n, 0);
    HAL_SPI_TransmitReceive(dev->s, tx_buf, rx_buf, 5, HAL_MAX_DELAY);
    tcswitch(n, 1);

    // Data starts at rx_buf[1]
    uint32_t data = ((uint32_t)rx_buf[1] << 24) |
                    ((uint32_t)rx_buf[2] << 16) |
                    ((uint32_t)rx_buf[3] << 8) |
                    rx_buf[4];

    if (data == 0xFFFFFFFF)
        return NO_MAX31856;

    if (data == 0 && verify_device(dev, n) == NO_MAX31856)
        return NO_MAX31856;

    int16_t offset = (data >> 16) & 0xFF;
    if (offset & 0x80)
        offset |= 0xFF00;

    int16_t tval = (data & 0xFFFF);
    if (tval & 0x8000)
        tval |= 0xFFFF0000;

    tval >>= 2;

    double t = (tval + offset) * 0.015625;
    if (unit == FAHRENHEIT)
        t = (t * 9.0 / 5.0) + 32;

    return t;
}