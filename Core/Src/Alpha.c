#include "Alpha.h"
#include "XPLink.h"
#include "dmacirc.h"

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
    return 0;
}

uint8_t ALPHA_COMMS_INIT(Alpha *a)
{
    // initialize DMA
    dmasendinit(&huart1);

    return 0;
}

uint8_t ALPHA_READ_TEMP(Alpha *a)
{
    // read and send temp data
    a->temp_1 = max31856_read_thermocouple(&(a->tc1), 1);

    return 0;
}

uint8_t ALPHA_SEND_10HZ(Alpha *a)
{
    // send all temp data
    xp_packet_t pkt1;
    pkt1.data = (uint64_t)(a->temp_1);
    pkt1.end_byte = 0x00;
    pkt1.sender_id = 0x33;
    pkt1.type = TEMP1;
    xp_packet_t pkt2;
    pkt2.data = (uint64_t)(a->temp_2);
    pkt2.end_byte = 0x00;
    pkt2.sender_id = 0x33;
    pkt2.type = TEMP2;
    xp_packet_t pkt3;
    pkt3.data = (uint64_t)(a->temp_3);
    pkt3.end_byte = 0x00;
    pkt3.sender_id = 0x33;
    pkt3.type = TEMP3;
    xp_packet_t pkt4;
    pkt4.data = (uint64_t)(a->temp_1);
    pkt4.end_byte = 0x00;
    pkt4.sender_id = 0x33;
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

    return 0;
}
