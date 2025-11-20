// Library for Interfacing with the ADS7828 Analog to Digital Converter over I2C
#ifndef ADS7828_H
#define ADS7828_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

// Number of ADS7828 channel combinations
#define ADS7828_CHANNELS 16

// If defined, dynamic memory allocation with malloc/free is done for averaging
#define ADS7828_DYNAMIC_MEM
#ifndef ADS7828_DYNAMIC_MEM
// Number of values stored for every active channel for averaging
#define ADS7828_AVG_MAX 20
#endif

// Circular buffer structure for averaging
typedef struct
{
    uint8_t w_index; // Write index
    uint8_t n;       // Number of elements
#ifdef ADS7828_DYNAMIC_MEM
    uint16_t *data; // Buffer data
#else
    uint16_t data[ADS7828_AVG_MAX]; // Buffer data
#endif
} ads7828_circ_buf_t;

// Defines the command bits for every possible channel selection (Datasheet Table 2)
// Choice between "Differential" for voltage between two channels or "Single Ended" for voltage to COM
typedef enum
{
    ADS7828_CHANNEL_0_COM = 0b1000, // Channel 0 to COM
    ADS7828_CHANNEL_1_COM = 0b1100, // Channel 1 to COM
    ADS7828_CHANNEL_2_COM = 0b1001, // Channel 2 to COM
    ADS7828_CHANNEL_3_COM = 0b1101, // Channel 3 to COM
    ADS7828_CHANNEL_4_COM = 0b1010, // Channel 4 to COM
    ADS7828_CHANNEL_5_COM = 0b1110, // Channel 5 to COM
    ADS7828_CHANNEL_6_COM = 0b1011, // Channel 6 to COM
    ADS7828_CHANNEL_7_COM = 0b1111, // Channel 7 to COM
    ADS7828_CHANNEL_0_1 = 0b0000,   // Channel 0 to 1
    ADS7828_CHANNEL_2_3 = 0b0001,   // Channel 2 to 3
    ADS7828_CHANNEL_4_5 = 0b0010,   // Channel 4 to 5
    ADS7828_CHANNEL_6_7 = 0b0011,   // Channel 6 to 7
    ADS7828_CHANNEL_1_0 = 0b0100,   // Channel 1 to 0
    ADS7828_CHANNEL_3_2 = 0b0101,   // Channel 3 to 2
    ADS7828_CHANNEL_5_4 = 0b0110,   // Channel 5 to 4
    ADS7828_CHANNEL_7_6 = 0b0111    // Channel 7 to 6
} ads7828_channel_t;

// Sets AD Power Down Mode
// See Datasheet (Table 1) for PD Mode Selection
typedef enum
{
    ADS7828_POWER_DOWN = 0b00,    // Power Down Between A/D Converter Conversions
    ADS7828_REF_OFF = 0b01,       // Internal Reference Voltage OFF and A/D Converter ON
    ADS7828_REF_ON_AD_OFF = 0b10, // Internal Reference ON and A/D Converter OFF
    ADS7828_REF_ON_AD_ON = 0b11   // Internal Reference ON and A/D Converter ON
} ads7828_pd_mode_t;

// I2C function type definitions
typedef int (*ads7828_i2c_write_fn)(void *handle, uint8_t dev_addr, const uint8_t *data, uint16_t len);
typedef int (*ads7828_i2c_read_fn)(void *handle, uint8_t dev_addr, uint8_t *data, uint16_t len);

// Main ADS7828 structure
typedef struct
{
    float scaling[ADS7828_CHANNELS];              // Channel Voltage Scaling
    float ref_voltage;                            // Reference voltage (2.5V internal by default)
    ads7828_pd_mode_t pd_mode;                    // Current Power Down Mode
    ads7828_circ_buf_t buffers[ADS7828_CHANNELS]; // Circular buffers for averaging

    uint8_t address;                // I2C Address
    void *i2c_handle;               // User-provided I2C handle
    ads7828_i2c_write_fn i2c_write; // User-provided I2C write function
    ads7828_i2c_read_fn i2c_read;   // User-provided I2C read function
} ads7828_t;

// Initialization functions
void ads7828_init(ads7828_t *dev, void *i2c_handle, uint8_t address,
                  ads7828_i2c_write_fn i2c_write, ads7828_i2c_read_fn i2c_read);
void ads7828_init_external_ref(ads7828_t *dev, void *i2c_handle, uint8_t address,
                               ads7828_i2c_write_fn i2c_write, ads7828_i2c_read_fn i2c_read,
                               float external_ref_voltage);
void ads7828_deinit(ads7828_t *dev);

// Reading functions
uint32_t ads7828_read_voltage(ads7828_t *dev, ads7828_channel_t channel);
float ads7828_read_voltage_float(ads7828_t *dev, ads7828_channel_t channel);
uint16_t ads7828_read_digit(ads7828_t *dev, ads7828_channel_t channel);

// Reference voltage functions
void ads7828_set_ref_voltage_external(ads7828_t *dev, float ref_voltage);
void ads7828_set_ref_voltage_internal(ads7828_t *dev);

// Power mode functions
void ads7828_set_power_mode(ads7828_t *dev, ads7828_pd_mode_t mode);
void ads7828_set_power_mode_update(ads7828_t *dev, ads7828_pd_mode_t mode, bool update_now);

// Scaling functions
void ads7828_set_scaling(ads7828_t *dev, ads7828_channel_t channel, float scaling);
float ads7828_get_scaling(ads7828_t *dev, ads7828_channel_t channel);
void ads7828_reset_scaling_channel(ads7828_t *dev, ads7828_channel_t channel);
void ads7828_reset_scaling_all(ads7828_t *dev);

// Averaging functions
void ads7828_set_averaging(ads7828_t *dev, ads7828_channel_t channel, uint8_t n);
void ads7828_clear_averaging(ads7828_t *dev, ads7828_channel_t channel);
void ads7828_disable_averaging(ads7828_t *dev, ads7828_channel_t channel);

#endif // ADS7828_H

int stm32_i2c_write(void *handle, uint8_t dev_addr, const uint8_t *data, uint16_t len);

int stm32_i2c_read(void *handle, uint8_t dev_addr, uint8_t *data, uint16_t len);
