#include "ADS7828.h"
#include <stdlib.h>
#include <string.h>

// STM32 HAL wrapper for I2C write
int stm32_i2c_write(void *handle, uint8_t dev_addr, const uint8_t *data, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)handle;
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hi2c, dev_addr, (uint8_t *)data, len, 100);
    return (status == HAL_OK) ? 0 : -1;
}

// STM32 HAL wrapper for I2C read
int stm32_i2c_read(void *handle, uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)handle;
    HAL_StatusTypeDef status = HAL_I2C_Master_Receive(hi2c, dev_addr, data, len, 100);
    return (status == HAL_OK) ? 0 : -1;
}

// Helper functions for circular buffer
static float ads7828_circ_buf_average(ads7828_circ_buf_t *buf)
{
    float sum = 0;
    for (uint8_t i = 0; i < buf->n; i++)
    {
        sum += buf->data[i];
    }
    return sum / buf->n;
}

static void ads7828_circ_buf_append(ads7828_circ_buf_t *buf, uint16_t value)
{
    buf->data[buf->w_index++] = value;

    // Circular buffer rollover
    if (buf->w_index >= buf->n)
    {
        buf->w_index = 0;
    }
}

// Internal initialization helper
static void ads7828_init_common(ads7828_t *dev)
{
    // Initialize all buffers
    memset(dev->buffers, 0, sizeof(dev->buffers));

    // Reset scaling for all channels
    ads7828_reset_scaling_all(dev);
}

/**
 * Initialize ADS7828 device with internal reference voltage
 *
 * @param dev Pointer to ADS7828 device structure
 * @param i2c_handle User-provided I2C handle (will be passed to I2C functions)
 * @param address I2C address of the device (default is 0x48 for AD0 = AD1 = 0)
 * @param i2c_write User-provided I2C write function
 * @param i2c_read User-provided I2C read function
 */
void ads7828_init(ads7828_t *dev, void *i2c_handle, uint8_t address,
                  ads7828_i2c_write_fn i2c_write, ads7828_i2c_read_fn i2c_read)
{
    dev->i2c_handle = i2c_handle;
    dev->address = address;
    dev->i2c_write = i2c_write;
    dev->i2c_read = i2c_read;
    dev->ref_voltage = 2.5f; // Internal reference voltage

    ads7828_init_common(dev);

    // Set the default power mode for internal ref voltage
    ads7828_set_power_mode(dev, ADS7828_REF_ON_AD_ON);
}

/**
 * Initialize ADS7828 device with external reference voltage
 * Implicitly changes the power down mode to disable the internal voltage reference!
 *
 * @param dev Pointer to ADS7828 device structure
 * @param i2c_handle User-provided I2C handle (will be passed to I2C functions)
 * @param address I2C address of the device (default is 0x48 for AD0 = AD1 = 0)
 * @param i2c_write User-provided I2C write function
 * @param i2c_read User-provided I2C read function
 * @param external_ref_voltage The external reference voltage (in Volts), should be between 0.05V and 5V
 */
void ads7828_init_external_ref(ads7828_t *dev, void *i2c_handle, uint8_t address,
                               ads7828_i2c_write_fn i2c_write, ads7828_i2c_read_fn i2c_read,
                               float external_ref_voltage)
{
    dev->i2c_handle = i2c_handle;
    dev->address = address;
    dev->i2c_write = i2c_write;
    dev->i2c_read = i2c_read;

    ads7828_init_common(dev);
    ads7828_set_ref_voltage_external(dev, external_ref_voltage);
}

/**
 * Deinitialize ADS7828 device and free allocated memory
 *
 * @param dev Pointer to ADS7828 device structure
 */
void ads7828_deinit(ads7828_t *dev)
{
#ifdef ADS7828_DYNAMIC_MEM
    for (uint8_t c = 0; c < ADS7828_CHANNELS; c++)
    {
        if (dev->buffers[c].data != NULL)
        {
            free(dev->buffers[c].data);
            dev->buffers[c].data = NULL;
        }
    }
#endif
}

/**
 * Reads the voltage of a specified channel configuration.
 * Conversion from digits to voltage is done with the set reference voltage!
 * Returns voltage scaled by 10000 (e.g., 5.0V returns 50000)
 *
 * @param dev Pointer to ADS7828 device structure
 * @param channel The channel configuration you want the voltage from
 * @return Measured ADC Voltage * 10000 (e.g., 5.0V = 50000, 1.234V = 12340)
 */
uint32_t ads7828_read_voltage(ads7828_t *dev, ads7828_channel_t channel)
{
    float voltage = ads7828_read_voltage_float(dev, channel);
    return (uint32_t)(voltage * 10000.0f);
}

/**
 * Reads the voltage of a specified channel configuration as a float.
 * Conversion from digits to voltage is done with the set reference voltage!
 *
 * @param dev Pointer to ADS7828 device structure
 * @param channel The channel configuration you want the voltage from
 * @return Measured ADC Voltage [V] of given channel configuration
 */
float ads7828_read_voltage_float(ads7828_t *dev, ads7828_channel_t channel)
{
    return (ads7828_read_digit(dev, channel) / 4095.0f * dev->ref_voltage * dev->scaling[channel]);
}

/**
 * Reads the digit of a specified channel configuration.
 * ADS7828 has 12 Bit resolution, so values from 0 - 4095!
 *
 * @param dev Pointer to ADS7828 device structure
 * @param channel The channel configuration you want the digit from
 * @return Measured ADC digit (0 - 4095) of given channel configuration
 */
uint16_t ads7828_read_digit(ads7828_t *dev, ads7828_channel_t channel)
{
    uint8_t command = 0x00;

    command |= (((uint8_t)channel) << 4);
    command |= (((uint8_t)dev->pd_mode) << 2);

    uint8_t data[2] = {0};

    dev->i2c_write(dev->i2c_handle, (dev->address << 1), &command, 1);
    dev->i2c_read(dev->i2c_handle, (dev->address << 1), data, 2);

    uint16_t digit = (uint16_t)((data[0] << 8) + data[1]);

    // No averaging
    if (dev->buffers[channel].n <= 1)
    {
        return digit;
    }

    // Update the buffer and calculate average
    ads7828_circ_buf_append(&dev->buffers[channel], digit);
    return (uint16_t)ads7828_circ_buf_average(&dev->buffers[channel]);
}

/**
 * Set your external reference voltage for operation without the internal reference.
 * Implicitly switches the power down mode to turn the internal reference OFF!
 *
 * @param dev Pointer to ADS7828 device structure
 * @param ref_voltage External reference voltage in [V]
 */
void ads7828_set_ref_voltage_external(ads7828_t *dev, float ref_voltage)
{
    dev->ref_voltage = ref_voltage;

    if (dev->pd_mode == ADS7828_REF_OFF)
    {
        return;
    }

    // If you choose an external voltage reference we have to change the mode accordingly
    ads7828_set_power_mode_update(dev, ADS7828_REF_OFF, true);
}

/**
 * Set the reference voltage back to internal (2.5V).
 * Implicitly switches the power down mode to turn the internal reference ON!
 *
 * @param dev Pointer to ADS7828 device structure
 */
void ads7828_set_ref_voltage_internal(ads7828_t *dev)
{
    dev->ref_voltage = 2.5f;

    // If you choose an internal voltage reference we have to change the mode accordingly
    ads7828_set_power_mode_update(dev, ADS7828_REF_ON_AD_ON, true);
}

/**
 * Set the ADC power down mode (see datasheet for more info).
 * Implicitly switches the reference voltage to internal (2.5V) for modes with REF_ON_x
 *
 * @param dev Pointer to ADS7828 device structure
 * @param mode The mode you want to switch to
 * @param update_now If true, the mode is switched instantly by sending a command. Otherwise mode is changed with next read request!
 */
void ads7828_set_power_mode_update(ads7828_t *dev, ads7828_pd_mode_t mode, bool update_now)
{
    dev->pd_mode = mode;

    // If you choose a mode with internal reference we have to set the voltage back
    if (mode == ADS7828_REF_ON_AD_OFF || mode == ADS7828_REF_ON_AD_ON)
    {
        dev->ref_voltage = 2.5f;
    }

    // To update the mode we have to transmit the command, so just do a random request
    if (update_now)
    {
        ads7828_read_digit(dev, ADS7828_CHANNEL_0_COM);
    }
}

/**
 * Set the ADC power down mode (see datasheet for more info).
 * Implicitly switches the reference voltage to internal (2.5V) for modes with REF_ON_x
 *
 * @param dev Pointer to ADS7828 device structure
 * @param mode The mode you want to switch to
 */
void ads7828_set_power_mode(ads7828_t *dev, ads7828_pd_mode_t mode)
{
    ads7828_set_power_mode_update(dev, mode, true);
}

/**
 * Set the scaling for a channel voltage so that read_voltage returns voltage * scaling
 *
 * @param dev Pointer to ADS7828 device structure
 * @param channel The Channel to set the scaling for
 * @param scaling Scaling Factor that will be multiplied with the voltage
 */
void ads7828_set_scaling(ads7828_t *dev, ads7828_channel_t channel, float scaling)
{
    dev->scaling[channel] = scaling;
}

/**
 * Get the current voltage scaling for a channel
 *
 * @param dev Pointer to ADS7828 device structure
 * @param channel The Channel to get the scaling for
 * @return The current scaling factor of the channel
 */
float ads7828_get_scaling(ads7828_t *dev, ads7828_channel_t channel)
{
    return dev->scaling[channel];
}

/**
 * Reset the scaling for a channel voltage back to 1
 *
 * @param dev Pointer to ADS7828 device structure
 * @param channel The Channel to reset the scaling for
 */
void ads7828_reset_scaling_channel(ads7828_t *dev, ads7828_channel_t channel)
{
    ads7828_set_scaling(dev, channel, 1.0f);
}

/**
 * Reset the scaling for all channels back to 1
 *
 * @param dev Pointer to ADS7828 device structure
 */
void ads7828_reset_scaling_all(ads7828_t *dev)
{
    for (uint8_t c = 0; c < ADS7828_CHANNELS; c++)
    {
        ads7828_reset_scaling_channel(dev, (ads7828_channel_t)c);
    }
}

/**
 * Enables averaging for a certain channel.
 * Whenever read_digit or read_voltage is called, the result will be the average of the last N values.
 *
 * @param dev Pointer to ADS7828 device structure
 * @param channel The channel to enable averaging for
 * @param n Number of values to average
 */
void ads7828_set_averaging(ads7828_t *dev, ads7828_channel_t channel, uint8_t n)
{
    // Averaging over 1 value is useless
    if (n == 1)
    {
        return;
    }

#ifdef ADS7828_DYNAMIC_MEM
    // Free existing memory if allocated
    if (dev->buffers[channel].data != NULL)
    {
        free(dev->buffers[channel].data);
    }

    // Reserve memory to store n last values
    dev->buffers[channel].n = n;
    dev->buffers[channel].data = (uint16_t *)calloc(n, sizeof(uint16_t));
    dev->buffers[channel].w_index = 0;
#else
    dev->buffers[channel].n = (n > ADS7828_AVG_MAX) ? ADS7828_AVG_MAX : n;
    ads7828_clear_averaging(dev, channel);
#endif
}

/**
 * Clears all current values of the channel and sets them to 0
 *
 * @param dev Pointer to ADS7828 device structure
 * @param channel The channel to clear the old values for
 */
void ads7828_clear_averaging(ads7828_t *dev, ads7828_channel_t channel)
{
    for (uint8_t n = 0; n < dev->buffers[channel].n; n++)
    {
        dev->buffers[channel].data[n] = 0;
    }
    dev->buffers[channel].w_index = 0;
}

/**
 * Disables the averaging and deletes all stored values
 *
 * @param dev Pointer to ADS7828 device structure
 * @param channel The channel to disable averaging for
 */
void ads7828_disable_averaging(ads7828_t *dev, ads7828_channel_t channel)
{
    // Averaging is already disabled
    if (dev->buffers[channel].n == 1)
    {
        return;
    }

    dev->buffers[channel].n = 1;

#ifdef ADS7828_DYNAMIC_MEM
    if (dev->buffers[channel].data != NULL)
    {
        free(dev->buffers[channel].data);
        dev->buffers[channel].data = NULL;
    }
#else
    ads7828_clear_averaging(dev, channel);
#endif
}