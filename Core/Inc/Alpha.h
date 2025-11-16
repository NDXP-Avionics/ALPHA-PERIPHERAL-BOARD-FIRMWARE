#ifndef ALPHA_H
#define ALPHA_H

#include <stdint.h>
#include "MAX31856.h"

typedef struct
{

    // sensor handles
    MAX31856_t tc1;
    MAX31856_t tc2;
    MAX31856_t tc3;
    MAX31856_t tc4;

    // data variables
    uint32_t temp_1;
    uint32_t temp_2;
    uint32_t temp_3;
    uint32_t temp_4;

    // safety variables

} Alpha;

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;

uint8_t ALPHA_SENSORS_INIT(Alpha *a);

uint8_t ALPHA_COMMS_INIT(Alpha *a);

uint8_t ALPHA_READ_TEMP(Alpha *a);

uint8_t ALPHA_SEND_10HZ(Alpha *a);

#endif