/*
 * Library for the TI ADS1231 24-Bit Analog-to-Digital Converter
 * Compatible with STM32 HAL Library
 */
#include "ADS1231.h"

/*
 * Initialize the ADS1231 structure
 */
void ADS1231_Init(ADS1231_t* ads, GPIO_TypeDef* clk_port, uint16_t clk_pin, 
                  GPIO_TypeDef* data_port, uint16_t data_pin) {
    ads->clk_port = clk_port;
    ads->clk_pin = clk_pin;
    ads->data_port = data_port;
    ads->data_pin = data_pin;
    ads->last_read_ms = 0;
}

/*
 * Configure GPIO pins and initialize hardware
 */
int8_t ADS1231_Begin(ADS1231_t* ads) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Validate that pins are different
    if (ads->clk_port == ads->data_port && ads->clk_pin == ads->data_pin) {
        return ADS1231_ERROR_INVALID_PINS;
    }
    
    // Configure CLK pin as output
    GPIO_InitStruct.Pin = ads->clk_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ads->clk_port, &GPIO_InitStruct);
    
    // Set CLK low to bring ADS1231 out of suspend
    HAL_GPIO_WritePin(ads->clk_port, ads->clk_pin, GPIO_PIN_RESET);
    
    // Configure DATA pin as input with pull-up
    GPIO_InitStruct.Pin = ads->data_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ads->data_port, &GPIO_InitStruct);
    
    return ADS1231_SUCCESS;
}

/*
 * Read 24-bit ADC value (blocking call)
 * Returns: ADS1231_SUCCESS on success, negative error code on failure
 * val: pointer to store the signed 32-bit result
 */
int8_t ADS1231_GetValue(ADS1231_t* ads, int32_t* val) {
    uint32_t start_tick;
    uint32_t raw_val = 0;
    
    *val = 0; // Initialize output
    
    /* Wait for data pin to go HIGH (if it's not already) */
    start_tick = HAL_GetTick();
    while (HAL_GPIO_ReadPin(ads->data_port, ads->data_pin) != GPIO_PIN_SET) {
        if ((HAL_GetTick() - start_tick) > 150) {
            return ADS1231_ERROR_HIGH_TIMEOUT;
        }
    }
    
    /* Wait for HIGH to LOW transition indicating data ready */
    start_tick = HAL_GetTick();
    while (HAL_GPIO_ReadPin(ads->data_port, ads->data_pin) != GPIO_PIN_RESET) {
        if ((HAL_GetTick() - start_tick) > 150) {
            return ADS1231_ERROR_LOW_TIMEOUT;
        }
    }
    
    ads->last_read_ms = HAL_GetTick();
    
    /* Read 24 bits (MSB first) */
    for (int8_t i = 23; i >= 0; i--) {
        HAL_GPIO_WritePin(ads->clk_port, ads->clk_pin, GPIO_PIN_SET);
        raw_val = (raw_val << 1) | (HAL_GPIO_ReadPin(ads->data_port, ads->data_pin) ? 1 : 0);
        HAL_GPIO_WritePin(ads->clk_port, ads->clk_pin, GPIO_PIN_RESET);
    }
    
    /* Convert from 24-bit two's complement to 32-bit signed integer */
    if (raw_val & 0x800000) {  // If sign bit is set (negative number)
        *val = (int32_t)(raw_val | 0xFF000000);  // Sign extend to 32 bits
    } else {
        *val = (int32_t)raw_val;
    }
    
    /* Send one more clock pulse to prepare for next conversion */
    HAL_GPIO_WritePin(ads->clk_port, ads->clk_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ads->clk_port, ads->clk_pin, GPIO_PIN_RESET);
    
    return ADS1231_SUCCESS;
}

/*
 * Get timestamp of last successful reading
 */
uint32_t ADS1231_GetLastReadMillis(ADS1231_t* ads) {
    return ads->last_read_ms;
}