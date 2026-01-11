// By: Luke Scholler and Sebastian Brock

#include "Alpha.h"
#include "XPLink.h"
#include "dmacirc.h"
#include "uart_rx.h"
#include "state_machine.h"

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
}

uint8_t ALPHA_READ_TEMP(Alpha *a)
{

    // read and send temp data
    a->temp_1 = max31856_read_thermocouple(&(a->tc1), 1);
    a->temp_2 = max31856_read_thermocouple(&(a->tc2), 2);
    a->temp_3 = max31856_read_thermocouple(&(a->tc3), 3);
    a->temp_4 = max31856_read_thermocouple(&(a->tc4), 4);

    return 0;
}

uint8_t ALPHA_READ_PRESSURE(Alpha *a)
{

    a->p1 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_2_COM);
    a->p2 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_3_COM);
    a->p3 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_4_COM);
    a->p4 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_5_COM);
    a->p5 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_6_COM);
    a->p6 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_7_COM);

    a->p7 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_0_COM);
    a->p8 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_1_COM);
    a->p9 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_2_COM);
    a->p10 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_3_COM);
    a->p11 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_4_COM);
    a->p12 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_5_COM);

    return 0;
}

uint8_t ALPHA_READ_KEYS(Alpha *a)
{
    a->k1 = HAL_GPIO_ReadPin(K1_GPIO_Port, K1_Pin);
}

uint8_t ALPHA_READ_BW(Alpha *a)
{
    a->bw1 = HAL_GPIO_ReadPin(BW1_GPIO_Port, BW1_Pin);
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
        a->load_cell_value = value;
    }

    return result;
}

uint8_t ALPHA_READ_ACC(Alpha *a) // Still named ACC in your code
{
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
    uint16_t vals[] = {a->p1,
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
