/*
 * Library for the TI ADS1231 24-Bit Analog-to-Digital Converter
 * Compatible with STM32 HAL Library
 */
#include "ADS1231.h"
#include "stm32f103xb.h"

/*
 * Initialize the ADS1231 structure
 */

void ADS1231_Init(ADS1231_t *ads, GPIO_TypeDef *clk_port, uint16_t clk_pin,
                  GPIO_TypeDef *data_port, uint16_t data_pin)
{
    ads->clk_port = clk_port;
    ads->clk_pin = clk_pin;
    ads->data_port = data_port;
    ads->data_pin = data_pin;
    ads->last_read_ms = 0;
}

/*
 * Configure GPIO pins and initialize hardware
 */
int8_t ADS1231_Begin(ADS1231_t *ads)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Validate that pins are different
    if (ads->clk_port == ads->data_port && ads->clk_pin == ads->data_pin)
    {
        return ADS1231_ERROR_INVALID_PINS;
    }

    // Configure CLK pin as output
    GPIO_InitStruct.Pin = ads->clk_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ads->clk_port, &GPIO_InitStruct);

    // Set CLK low initially
    HAL_GPIO_WritePin(ads->clk_port, ads->clk_pin, GPIO_PIN_RESET);

    // Configure DATA pin as input (NO PULL-UP - library sets it LOW)
    GPIO_InitStruct.Pin = ads->data_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ads->data_port, &GPIO_InitStruct);

    // Power cycle the ADS1231 (10ms reset pulse)
    // Note: This assumes you want to keep the chip powered
    // The original library had a separate power pin
    HAL_Delay(10);

    return ADS1231_SUCCESS;
}

/*
 * Check if ADS1231 has data ready
 * Returns: 1 if data ready (DATA pin LOW), 0 if not ready
 */
uint8_t ADS1231_IsReady(ADS1231_t *ads)
{
    // Data ready when DATA pin is LOW (opposite of what you had!)
    if (HAL_GPIO_ReadPin(ads->data_port, ads->data_pin) == GPIO_PIN_RESET)
    {
        return 1; // Ready
    }
    return 0; // Not ready
}

/*
 * Read 24-bit ADC value (blocking call)
 * Returns: ADS1231_SUCCESS on success, negative error code on failure
 * val: pointer to store the signed 32-bit result
 */
int8_t ADS1231_GetValue(ADS1231_t *ads, int32_t *val)
{
    uint32_t start_tick;
    long raw_val = 0;
    uint8_t input_status;

    *val = 0; // Initialize output

    /*
     * Per datasheet Figure 19: Wait for DRDY/DOUT to go LOW
     * Data ready is indicated by LOW signal
     */
    start_tick = HAL_GetTick();
    while (HAL_GPIO_ReadPin(ads->data_port, ads->data_pin) == GPIO_PIN_SET)
    {
        if ((HAL_GetTick() - start_tick) > 1000)
        {
            return ADS1231_ERROR_LOW_TIMEOUT;
        }
    }

    ads->last_read_ms = HAL_GetTick();

    /*
     * Per datasheet page 13: t_DS = 0ns min (DRDY/DOUT low to first SCLK rising)
     * But small delay helps with noise immunity
     */
    for (volatile int i = 0; i < 10; i++)
        ;

    /*
     * Read 24 bits (NOT 25!)
     * Datasheet Figure 19 shows 24 SCLKs for data retrieval
     * The 25th clock is OPTIONAL to force DRDY/DOUT high
     */
    for (uint8_t count = 0; count < 24; count++)
    {
        // SCLK rising edge
        HAL_GPIO_WritePin(ads->clk_port, ads->clk_pin, GPIO_PIN_SET);

        /*
         * Per datasheet: t_PD = 50ns max (propagation delay)
         * At 72MHz, need at least 4 cycles = ~56ns
         * Use 100ns to be safe
         */
        for (volatile int i = 0; i < 8; i++)
            ; // ~112ns delay

        // Read data pin AFTER rising edge
        input_status = (HAL_GPIO_ReadPin(ads->data_port, ads->data_pin) == GPIO_PIN_SET) ? 1 : 0;

        // Build 24-bit value MSB first (bits 23 down to 0)
        raw_val = (raw_val << 1) | input_status;

        // SCLK falling edge
        HAL_GPIO_WritePin(ads->clk_port, ads->clk_pin, GPIO_PIN_RESET);

        /*
         * Per datasheet: t_SCLK = 100ns min pulse width
         * Need same delay for LOW period
         */
        for (volatile int i = 0; i < 8; i++)
            ; // ~112ns delay
    }

    /*
     * Convert 24-bit two's complement to 32-bit signed
     * Check if bit 23 (sign bit) is set
     */
    if (raw_val & 0x800000)
    {
        // Negative number: sign extend
        *val = (int32_t)(raw_val | 0xFF000000);
    }
    else
    {
        // Positive number
        *val = (int32_t)raw_val;
    }

    /*
     * Optional: Send 25th clock pulse to force DRDY/DOUT high
     * Per datasheet Figure 20, this is useful for polling
     */
    HAL_GPIO_WritePin(ads->clk_port, ads->clk_pin, GPIO_PIN_SET);
    for (volatile int i = 0; i < 8; i++)
        ;
    HAL_GPIO_WritePin(ads->clk_port, ads->clk_pin, GPIO_PIN_RESET);

    return ADS1231_SUCCESS;
}

/*
 * Read and calibrate (26 clock pulses total)
 * This matches the calibrate() function in the original library
 */
int8_t ADS1231_Calibrate(ADS1231_t *ads, int32_t *val)
{
    int8_t result;

    // Read data (25 pulses)
    result = ADS1231_GetValue(ads, val);

    if (result == ADS1231_SUCCESS)
    {
        // Send one additional clock pulse for calibration
        HAL_GPIO_WritePin(ads->clk_port, ads->clk_pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(ads->clk_port, ads->clk_pin, GPIO_PIN_RESET);
    }

    return result;
}

/*
 * Get timestamp of last successful reading
 */
uint32_t ADS1231_GetLastReadMillis(ADS1231_t *ads)
{
    return ads->last_read_ms;
}