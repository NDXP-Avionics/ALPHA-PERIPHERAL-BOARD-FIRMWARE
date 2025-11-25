#ifndef ALPHA_H
#define ALPHA_H

#include <stdint.h>
#include "MAX31856.h"
#include "ADS7828.h"

// State enumeration 
typedef enum {
    STATE_TESTING_FILL,
    STATE_IGNTTION,
    STATE_BURN_WIRE_CUT,
    STATE_STEADY_STATE,
    STATE_SHUTDOWN,
    STATE_POWER_OFF
} AlphaState;

#define MAX_MISSION_TIME_MS 50000

# define SHUTDOWN_WAIT_TIME_MS 600000

typedef struct {

    AlphaState current_state;
    uint32_t mission_start_time;
    uint32_t igntition_start_time;
    uint32_t steady_start_time;
    uint32_t shutdown_start_time;
    uint8_t both_keys_pressed;
    uint8_t ignition_serial_recieved;
    uint8_t burn_wire_cut;
    uint8_t shutdown_timer_satisfied;
    uint8_t time_limit_exceeded

    // Sensor streaming control
    uint8_t high_rate_logging;

} Alpha ;

//Function Prototypes

uint8-t ALPHA_STATE_MACHINE(Alpha *a);
uint8_t ALPHA_CHECK_SAFETY(Alpha *a);
uint8_t ALPHA_CONTROL_VALVES(Alpha *a, uint8_t valve_state);
uint8_t ALPHA_CHECK_BURN_WIRE(Alpha *a);


typedef struct
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
    uint16_t p1;
    uint16_t p2;
    uint16_t p3;
    uint16_t p4;
    uint16_t p5;
    uint16_t p6;
    uint16_t p7;
    uint16_t p8;
    uint16_t p9;
    uint16_t p10;
    uint16_t p11;
    uint16_t p12;

    // state variables
    uint8_t going;

} Alpha;

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
extern I2C_HandleTypeDef hi2c1;

uint8_t ALPHA_STATE_INIT(Alpha *a);

uint8_t ALPHA_SENSORS_INIT(Alpha *a);

uint8_t ALPHA_COMMS_INIT(Alpha *a);

uint8_t ALPHA_READ_TEMP(Alpha *a);

uint8_t ALPHA_READ_PRESSURE(Alpha *a);

uint8_t ALPHA_SEND_10HZ(Alpha *a);

uint8_t Alpha_Send_100HZ(Alpha *a);

uint8_t ALPHA_RX(Alpha *a);

#endif /* ALPHA_H */