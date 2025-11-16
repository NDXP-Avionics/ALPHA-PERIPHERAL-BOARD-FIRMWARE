#include "XPLink.h"
#include <stdio.h>
#include <string.h>

// vars for unpacking
static uint8_t BUFFER[256];
static uint8_t BUFF_HEAD = 0;
static uint8_t PREV_END = 0;

int XPLINK_PACK(uint8_t *output, xp_packet_t *xppack)
{
    uint8_t packet[10];

    // create struct without cobs
    for (int i = 0; i < 10; i++)
    {
        packet[i] = 0;
    }

    // Sender ID
    packet[0] = SYS_ID;

    // Packet type
    packet[1] = xppack->type;

    // Pack 7 least significant bytes of the value (little-endian)
    for (int i = 0; i < 7; i++)
    {
        packet[2 + i] = (xppack->data >> (i * 8)) & 0xFF;
    }

    // get sum of packet
    int sum = 0;
    for (int i = 0; i < 9; i++)
    {
        sum += packet[i];
    }

    // checksum
    packet[9] = sum % 256;

    /*
    printf("INITIAL PACKET: \n");
    for (int i = 0; i < 10; i++)
    {
        printf("%x ", packet[i]);
    }
    */

    printf(
        "\n\n");

    COBS_PACK(output, packet, 10, END_BYTE);

    return 1;
}

uint8_t XPLINK_UNPACK(xp_packet_t *output, uint8_t byte)
{

    // check for start of new packet
    if (PREV_END)
    {
        PREV_END = 0;
        // reset buffer
        BUFF_HEAD = 0;
    }

    // add byte to buffer
    BUFFER[BUFF_HEAD] = byte;
    BUFF_HEAD++;

    // if end byte, collect buffer
    if (byte == END_BYTE)
    {
        PREV_END = 1;

        // unpack the buffer
        uint8_t raw[11];

        COBS_UNPACK(raw, BUFFER, BUFF_HEAD, END_BYTE);

        // Checksum
        int sum = 0;
        for (int i = 0; i < 9; i++)
        {
            sum += raw[i];
        }
        uint8_t calculated_checksum = sum % 256;

        if (calculated_checksum != raw[9])
        {
            // Checksum failed, packet is corrupt
            return 0;
        }

        output->sender_id = raw[0];
        output->type = raw[1];
        output->data = 0; // Clear old data
        for (int i = 0; i < 7; i++)
        {
            // get data
            output->data |= ((uint64_t)raw[2 + i] << (i * 8));
        }
        return 1;
    }

    return 0;
}

uint8_t *COBS_PACK(uint8_t *output, uint8_t *input, uint8_t size, uint8_t end_byte)
{

    uint8_t curr_idx = 0;

    for (int i = 0; i < size; i++)
    {

        if (input[i] == end_byte)
        {
            output[curr_idx] = (i - curr_idx) + 1;
            curr_idx = i + 1;
        }
        else
        {
            output[i + 1] = input[i];
        }

        if (i == size - 1)
        {
            output[curr_idx] = (i - curr_idx) + 2;
        }
    }

    output[size + 1] = end_byte;

    return output;
}

uint8_t *COBS_UNPACK(uint8_t *out, uint8_t *in, uint8_t insize, uint8_t end_byte)
{

    uint8_t read_head = 0;
    uint8_t write_head = 0;

    uint8_t read_step = in[read_head];

    while (read_step != end_byte)
    {
        memcpy(&out[write_head], &in[read_head + 1], read_step - 1);
        // increment
        read_head += read_step;
        write_head += read_step;
        read_step = in[read_head];

        // add additional zero
        out[write_head - 1] = 0;
    }

    return out;
}
