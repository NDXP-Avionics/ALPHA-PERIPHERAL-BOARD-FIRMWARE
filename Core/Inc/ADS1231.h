/*
 * Library for the TI ADS1231 24-Bit Analog-to-Digital Converter
 * Compatible with STM32 HAL Library
 */
#ifndef ADS1231_H
#define ADS1231_H

#include "main.h"  // Replace xxxx with your STM32 family (f1, f4, etc.)
#include <stdint.h>
#include <stdbool.h>

/* Error codes */
#define ADS1231_SUCCESS 0
#define ADS1231_ERROR_HIGH_TIMEOUT -1
#define ADS1231_ERROR_LOW_TIMEOUT -2
#define ADS1231_ERROR_INVALID_PINS -3

/* ADS1231 structure */
typedef struct {
    GPIO_TypeDef* clk_port;
    uint16_t clk_pin;
    GPIO_TypeDef* data_port;
    uint16_t data_pin;
    uint32_t last_read_ms;
} ADS1231_t;

/* Function prototypes */
void ADS1231_Init(ADS1231_t* ads, GPIO_TypeDef* clk_port, uint16_t clk_pin, 
                  GPIO_TypeDef* data_port, uint16_t data_pin);
int8_t ADS1231_Begin(ADS1231_t* ads);
int8_t ADS1231_GetValue(ADS1231_t* ads, int32_t* val);
uint32_t ADS1231_GetLastReadMillis(ADS1231_t* ads);

#endif // ADS1231_H