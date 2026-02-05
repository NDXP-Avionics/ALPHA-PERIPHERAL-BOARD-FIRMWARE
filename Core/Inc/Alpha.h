#ifndef ALPHA_H
#define ALPHA_H

#include <stdint.h>
#include "MAX31856.h"
#include "ADS7828.h"
#include "ADS1231.h"
#include "BNO055.h"
#include "state_machine.h"

typedef struct rotation
{

    int16_t x;
    int16_t y;
    int16_t z;

} rotation;

typedef enum sensors
{
    SENSOR_ACC,
} sensors;

typedef struct Alpha
{

    // Temp sensor handles
    MAX31856_t tc1;
    MAX31856_t tc2;
    MAX31856_t tc3;
    MAX31856_t tc4;

    // Pressure Sensor Handles
    ads7828_t ads1;
    ads7828_t ads2;

    // Temp Data
    uint32_t temp_1;
    uint32_t temp_2;
    uint32_t temp_3;
    uint32_t temp_4;

    // Temp Pressure
    uint32_t p1;
    uint32_t p2;
    uint32_t p3;
    uint32_t p4;
    uint32_t p5;
    uint32_t p6;
    uint32_t p7;
    uint32_t p8;
    uint32_t p9;
    uint32_t p10;
    uint32_t p11;
    uint32_t p12;

    // load cell
    ADS1231_t load_cell;
    int32_t load_cell_value;

    // pyro
    uint8_t pyro1;

    // solenoids
    uint8_t s1;
    uint8_t s2;
    uint8_t s3;
    uint8_t s4;

    // keys
    uint8_t k1;

    // Burn Wire
    uint8_t bw1;

    // accelerometer
    struct bno055_t bno055;
    rotation rot;

    // state variables
    uint8_t going;

    // global state variable
    STATE state;

    // attatched sensors bitmap (used only for accelerometer at this point)
    uint8_t attatched_sensors;

} Alpha;

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
extern I2C_HandleTypeDef hi2c1;

uint8_t ALPHA_STATE_INIT(Alpha *a);

uint8_t ALPHA_SENSORS_INIT(Alpha *a);

uint8_t ALPHA_COMMS_INIT(Alpha *a);

uint8_t ALPHA_SET_PYRO(Alpha *a, uint8_t val);

uint8_t ALPHA_READ_TEMP(Alpha *a);

uint8_t ALPHA_READ_PRESSURE(Alpha *a);

uint8_t ALPHA_READ_KEYS(Alpha *a);

uint8_t ALPHA_READ_BW(Alpha *a);

uint8_t ALPHA_READ_LOADCELL(Alpha *a);

uint8_t ALPHA_READ_ACC(Alpha *a);

uint8_t ALPHA_SEND_10HZ(Alpha *a);

uint8_t Alpha_Send_100HZ(Alpha *a);

uint8_t ALPHA_SET_SOLENOID(Alpha *a, uint8_t s, uint8_t val);

uint8_t ALPHA_RX(Alpha *a);

#endif /* ALPHA_H */