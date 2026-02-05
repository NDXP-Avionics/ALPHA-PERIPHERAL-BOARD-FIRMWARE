#include "Alpha.h"
#include "XPLink.h"
#include "dmacirc.h"
#include "uart_rx.h"
#include "state_machine.h"

#define TESTDATA false

uint8_t ALPHA_STATE_INIT(Alpha *a)
{
    a->going = 1;

    a->s1 = 0;
    a->s2 = 0;
    a->s3 = 0;
    a->s4 = 0;

    SM_SET_STATE(a, STANDBY);

    return 0;
}

uint8_t ALPHA_SENSORS_INIT(Alpha *a)
{

    // seta all sensors to not attatched
    a->attatched_sensors = 0x00;

    // initialize TC
    a->tc1.spi_tx = HAL_SPI_Transmit;
    a->tc1.spi_rx = HAL_SPI_Receive;
    a->tc1.s = &hspi1;
    a->tc2.spi_tx = HAL_SPI_Transmit;
    a->tc2.spi_rx = HAL_SPI_Receive;
    a->tc2.s = &hspi1;
    a->tc3.spi_tx = HAL_SPI_Transmit;
    a->tc3.spi_rx = HAL_SPI_Receive;
    a->tc3.s = &hspi1;
    a->tc4.spi_tx = HAL_SPI_Transmit;
    a->tc4.spi_rx = HAL_SPI_Receive;
    a->tc4.s = &hspi1;

    max31856_init(&(a->tc1));
    max31856_init(&(a->tc2));
    max31856_init(&(a->tc3));
    max31856_init(&(a->tc4));
    // set thermocouple type
    max31856_write_register(&(a->tc1), 0x01, 0b00000011, 1);
    max31856_write_register(&(a->tc2), 0x01, 0b00000011, 2);
    max31856_write_register(&(a->tc3), 0x01, 0b00000011, 3);
    max31856_write_register(&(a->tc4), 0x01, 0b00000011, 4);
    // start conversion
    max31856_write_register(&(a->tc1), 0x00, 0b10010000, 1);
    max31856_write_register(&(a->tc2), 0x00, 0b10010000, 2);
    max31856_write_register(&(a->tc3), 0x00, 0b10010000, 3);
    max31856_write_register(&(a->tc4), 0x00, 0b10010000, 4);

    // initialize pressure sensors
    ads7828_init_external_ref(&(a->ads1), &hi2c1, 0x48, stm32_i2c_write, stm32_i2c_read, 5.0f);
    ads7828_init_external_ref(&(a->ads2), &hi2c1, 0x49, stm32_i2c_write, stm32_i2c_read, 5.0f);

    // initialize Load Cell
    ADS1231_Init(&(a->load_cell),
                 LC_SCLK_GPIO_Port, LC_SCLK_Pin,            // Clock pin
                 LC_DRDY_DOUT_GPIO_Port, LC_DRDY_DOUT_Pin); // Data pin
    if (ADS1231_Begin(&(a->load_cell)) != ADS1231_SUCCESS)
    {
        // Handle initialization error if needed
        // Could set an error flag or halt
    }
    a->load_cell_value = 0; // Initialize reading to 0

    // initialize BNO055
    a->bno055.bus_read = STM32_BUS_READ;
    a->bno055.bus_write = STM32_BUS_WRITE;
    a->bno055.dev_addr = BNO055_I2C_ADDR1;
    a->bno055.delay_msec = STM32_DELAY_MSEC;

    HAL_Delay(1000);

    if (bno055_init(&(a->bno055)) == BNO055_SUCCESS)
    {
        HAL_Delay(500);

        bno055_set_operation_mode(BNO055_OPERATION_MODE_CONFIG);

        HAL_Delay(100);

        bno055_set_power_mode(BNO055_POWER_MODE_NORMAL);
        HAL_Delay(100);

        bno055_set_operation_mode(BNO055_OPERATION_MODE_NDOF);
        HAL_Delay(100);
        a->attatched_sensors |= (1 << SENSOR_ACC);
    }

    return 0;
}

uint8_t ALPHA_COMMS_INIT(Alpha *a)
{
    // initialize DMA
    dmasendinit(&huart1);

    // initialize uart rx
    uart_rx_init(&huart1);

    return 0;
}

uint8_t ALPHA_SET_PYRO(Alpha *a, uint8_t val)
{
    HAL_GPIO_WritePin(PYRO1_GPIO_Port, PYRO1_Pin, val);
    a->pyro1 = val;

    return 0;
}

uint8_t ALPHA_READ_TEMP(Alpha *a)
{

    // read raw temp data (in degrees C * 1e4)
    uint32_t temp_1_raw = max31856_read_thermocouple(&(a->tc1), 1);
    uint32_t temp_2_raw = max31856_read_thermocouple(&(a->tc2), 2);
    uint32_t temp_3_raw = max31856_read_thermocouple(&(a->tc3), 3);
    uint32_t temp_4_raw = max31856_read_thermocouple(&(a->tc4), 4);

    // convert to degrees F * 1e5
    a->temp_1 = (uint32_t)(((uint64_t)temp_1_raw * 9 * 10) / 5 + 3200000UL);
    a->temp_2 = (uint32_t)(((uint64_t)temp_2_raw * 9 * 10) / 5 + 3200000UL);
    a->temp_3 = (uint32_t)(((uint64_t)temp_3_raw * 9 * 10) / 5 + 3200000UL);
    a->temp_4 = (uint32_t)(((uint64_t)temp_4_raw * 9 * 10) / 5 + 3200000UL);

    if (TESTDATA)
    {
        int testtemp = 500e5;
        a->temp_1 = testtemp;
        a->temp_2 = testtemp;
        a->temp_3 = testtemp;
        a->temp_4 = testtemp;
    }

    return 0;
}

uint8_t ALPHA_READ_PRESSURE(Alpha *a)
{

    uint32_t p_raw_1 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_2_COM);
    uint32_t p_raw_2 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_3_COM);
    uint32_t p_raw_3 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_4_COM);
    uint32_t p_raw_4 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_5_COM);
    uint32_t p_raw_5 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_6_COM);
    uint32_t p_raw_6 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_7_COM);
    uint32_t p_raw_7 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_0_COM);
    uint32_t p_raw_8 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_1_COM);
    uint32_t p_raw_9 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_2_COM);
    uint32_t p_raw_10 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_3_COM);
    uint32_t p_raw_11 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_4_COM);
    uint32_t p_raw_12 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_5_COM);

    // convert to PSI * 1e5
    a->p1 = (uint32_t)((((int64_t)p_raw_1 * 5) - 2048) * (1000 * 100000LL / 4) / 4096);
    a->p2 = (uint32_t)((((int64_t)p_raw_2 * 5) - 2048) * (1000 * 100000LL / 4) / 4096);
    a->p3 = (uint32_t)((((int64_t)p_raw_3 * 5) - 2048) * (1000 * 100000LL / 4) / 4096);
    a->p4 = (uint32_t)((((int64_t)p_raw_4 * 5) - 2048) * (5000 * 100000LL / 4) / 4096); // 5000 for different sensor
    a->p5 = (uint32_t)((((int64_t)p_raw_5 * 5) - 2048) * (1000 * 100000LL / 4) / 4096);
    a->p6 = (uint32_t)((((int64_t)p_raw_6 * 5) - 2048) * (1000 * 100000LL / 4) / 4096);
    a->p7 = (uint32_t)((((int64_t)p_raw_7 * 5) - 2048) * (1000 * 100000LL / 4) / 4096);
    a->p8 = (uint32_t)((((int64_t)p_raw_8 * 5) - 2048) * (1000 * 100000LL / 4) / 4096);
    a->p9 = (uint32_t)((((int64_t)p_raw_9 * 5) - 2048) * (1000 * 100000LL / 4) / 4096);
    a->p10 = (uint32_t)((((int64_t)p_raw_10 * 5) - 2048) * (1000 * 100000LL / 4) / 4096);
    a->p11 = (uint32_t)((((int64_t)p_raw_11 * 5) - 2048) * (1000 * 100000LL / 4) / 4096);
    a->p12 = (uint32_t)((((int64_t)p_raw_12 * 5) - 2048) * (1000 * 100000LL / 4) / 4096);

    if (TESTDATA)
    {
        int testpressure = 600e5;
        a->p1 = testpressure;
        a->p2 = testpressure;
        a->p3 = testpressure;
        a->p4 = testpressure;
        a->p5 = testpressure;
        a->p6 = testpressure;
        a->p7 = testpressure;
        a->p8 = testpressure;
        a->p9 = testpressure;
        a->p10 = testpressure;
        a->p11 = testpressure;
        a->p12 = testpressure;
    }

    return 0;
}

uint8_t ALPHA_READ_KEYS(Alpha *a)
{
    a->k1 = HAL_GPIO_ReadPin(K1_GPIO_Port, K1_Pin);

    return 0;
}

uint8_t ALPHA_READ_BW(Alpha *a)
{
    a->bw1 = HAL_GPIO_ReadPin(BW1_GPIO_Port, BW1_Pin);

    return 0;
}

uint8_t ALPHA_READ_LOADCELL(Alpha *a)
{

    if (!ADS1231_IsReady(&(a->load_cell)))
    {
        return 0;
    }

    int32_t value = 0;
    int8_t result = ADS1231_GetValue(&(a->load_cell), &value);

    if (result == ADS1231_SUCCESS)
    {
        // load cell value * 1e5
        a->load_cell_value = (int32_t)(((int64_t)value * 250000000) / 32212256);
    }

    return result;
}

uint8_t ALPHA_READ_ACC(Alpha *a) // Still named ACC in your code
{

    // check if sensor is attatched
    if (!(a->attatched_sensors & (1 << SENSOR_ACC)))
    {
        return 0;
    }

    struct bno055_euler_t euler;

    if (bno055_read_euler_hrp(&euler) != BNO055_SUCCESS)
    {
        // Mark as error
        a->rot.x = 0xAAAA;
        a->rot.y = 0xAAAA;
        a->rot.z = 0xAAAA;
        return 1;
    }
    else
    {
        a->rot.x = euler.h;
        a->rot.y = euler.r;
        a->rot.z = euler.p;
    }

    return 0;
}

uint8_t ALPHA_SEND_10HZ(Alpha *a)
{
    // send all temp data
    xp_packet_t pkt1;
    pkt1.data = (uint64_t)(a->temp_1);
    pkt1.type = TEMP1;
    xp_packet_t pkt2;
    pkt2.data = (uint64_t)(a->temp_2);
    pkt2.type = TEMP2;
    xp_packet_t pkt3;
    pkt3.data = (uint64_t)(a->temp_3);
    pkt3.type = TEMP3;
    xp_packet_t pkt4;
    pkt4.data = (uint64_t)(a->temp_4);
    pkt4.type = TEMP4;

    uint8_t packet[12];

    XPLINK_PACK(packet, &pkt1);
    dmasend(packet, 12);
    XPLINK_PACK(packet, &pkt2);
    dmasend(packet, 12);
    XPLINK_PACK(packet, &pkt3);
    dmasend(packet, 12);
    XPLINK_PACK(packet, &pkt4);
    dmasend(packet, 12);

    // send keys & burn wire data
    xp_packet_t pkt;
    pkt.data = a->k1 << 8 | a->bw1;
    pkt.type = SWITCHES;

    XPLINK_PACK(packet, &pkt);
    dmasend(packet, 12);

    return 0;
}

uint8_t Alpha_Send_100HZ(Alpha *a)
{

    // send the current state
    xp_packet_t pkt = {0};
    pkt.data = a->state;
    pkt.type = XP_STATE;
    uint8_t packet[12];
    XPLINK_PACK(packet, &pkt);
    dmasend(packet, 12);

    // send pressure data
    uint32_t vals[] = {a->p1,
                       a->p2,
                       a->p3,
                       a->p4,
                       a->p5,
                       a->p6,
                       a->p7,
                       a->p8,
                       a->p9,
                       a->p10,
                       a->p11,
                       a->p12};

    for (int i = 0; i < 12; i++)
    {
        xp_packet_t pkt;
        pkt.data = vals[i];
        pkt.type = PRESSURE1 + i;

        uint8_t packet[12];

        XPLINK_PACK(packet, &pkt);
        dmasend(packet, 12);
    }

    // send load cell data
    xp_packet_t pkt_lc;
    pkt_lc.data = (uint64_t)(a->load_cell_value);
    pkt_lc.type = THRUST;
    XPLINK_PACK(packet, &pkt_lc);
    dmasend(packet, 12);

    // send solenoid data
    xp_packet_t pkt_s;
    pkt_s.data = a->s1 << 24 | a->s2 << 16 | a->s3 << 8 | a->s4;
    pkt_s.type = SOLENOID;
    XPLINK_PACK(packet, &pkt_s);
    dmasend(packet, 12);

    // send accelerometer data
    pkt.data = ((uint64_t)(uint16_t)a->rot.x << 32) |
               ((uint64_t)(uint16_t)a->rot.y << 16) |
               ((uint64_t)(uint16_t)a->rot.z);
    pkt.type = ACC;
    XPLINK_PACK(packet, &pkt);
    dmasend(packet, 12);

    return 0;
}

uint8_t ALPHA_SET_SOLENOID(Alpha *a, uint8_t s, uint8_t val)
{
    switch (s)
    {
    case 1:
        a->s1 = val;
        HAL_GPIO_WritePin(S1_GPIO_Port, S1_Pin, val);
        break;
    case 2:
        a->s2 = val;
        HAL_GPIO_WritePin(S2_GPIO_Port, S2_Pin, val);
        break;
    case 3:
        a->s3 = val;
        HAL_GPIO_WritePin(S3_GPIO_Port, S3_Pin, val);
        break;
    case 4:
        a->s4 = val;
        HAL_GPIO_WritePin(S4_GPIO_Port, S4_Pin, val);
        break;
    default:
        return 1; // Error
    }

    return 0;
}

uint8_t ALPHA_RX(Alpha *a)
{
    RX_HANDLER(a);

    return 0;
}
