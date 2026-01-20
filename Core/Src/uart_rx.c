#include "uart_rx.h"
#include "XPLink.h"
#include "dmacirc.h"
#include "byte_queue.h"
#include "Alpha.h"
#include "state_machine.h"

UART_HandleTypeDef *handle;
uint8_t RX_BUFFER[200];
uint32_t rd_ptr = 0;

byte_queue_t q;

typedef enum COMMANDS
{
    S1_ON,
    S1_OFF,
    S2_ON,
    S2_OFF,
    S3_ON,
    S3_OFF,
    S4_ON,
    S4_OFF,
    FIRE,
    RST,
} COMMANDS;

void RX_HANDLER(Alpha *a)
{

    static xp_packet_t pkt;

    uint32_t remaining = __HAL_DMA_GET_COUNTER(handle->hdmarx);
    uint32_t wr_ptr = 200 - remaining;

    // return if no new data
    if (rd_ptr == wr_ptr)
        return;

    // read while data in buffer
    while (rd_ptr != (200 - __HAL_DMA_GET_COUNTER(handle->hdmarx)))
    {
        // unpack
        if (XPLINK_UNPACK(&pkt, RX_BUFFER[rd_ptr]))
        {

            if (pkt.type == CMD)
            {
                switch (pkt.data)
                {
                case S1_ON:
                    ALPHA_SET_SOLENOID(a, 1, 1);
                    break;
                case S1_OFF:
                    ALPHA_SET_SOLENOID(a, 1, 0);
                    break;
                case S2_ON:
                    ALPHA_SET_SOLENOID(a, 2, 1);
                    break;
                case S2_OFF:
                    ALPHA_SET_SOLENOID(a, 2, 0);
                    break;
                case S3_ON:
                    ALPHA_SET_SOLENOID(a, 3, 1);
                    break;
                case S3_OFF:
                    ALPHA_SET_SOLENOID(a, 3, 0);
                    break;
                case S4_ON:
                    ALPHA_SET_SOLENOID(a, 4, 1);
                    break;
                case S4_OFF:
                    ALPHA_SET_SOLENOID(a, 4, 0);
                    break;
                case FIRE:
                    if (a->state == STANDBY)
                    {
                        SM_SET_STATE(a, FIRE_RECEIVED);
                    }
                    break;
                case RST:
                    SM_SET_STATE(a, STANDBY);
                    break;
                default:
                    break;
                }
            }
        }

        // Increment and Wrap
        rd_ptr = (rd_ptr + 1) % 200;
    }
}

void uart_rx_init(UART_HandleTypeDef *n)
{

    // DMA solution
    handle = n;
    // initialize a DMA receive
    HAL_UART_Receive_DMA(n, RX_BUFFER, 200);
}