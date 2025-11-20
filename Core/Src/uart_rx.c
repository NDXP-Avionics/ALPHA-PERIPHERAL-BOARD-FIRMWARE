#include "uart_rx.h"
#include "XPLink.h"
#include "dmacirc.h"
#include "byte_queue.h"

UART_HandleTypeDef *handle;
uint8_t RX_BUFFER[1];

byte_queue_t q;

void RX_HANDLER(Alpha *a)
{

    static xp_packet_t pkt;
    static int curr;

    while ((curr = byte_queue_pop(&q)) > -1)
    {

        if (XPLINK_UNPACK(&pkt, (uint8_t)curr))
        {
            // small delay to fix bug
            uint32_t cycles = 200 * (SystemCoreClock / 1000000);
            while (cycles--)
            {
                __NOP(); // No operation - one CPU cycle
            }

            // declaration
            if (pkt.data == 10)
            {
                a->going = 0;
            }
            if (pkt.data == 333)
            {
                a->going = 1;
            }
            if (pkt.data == 404)
            {
                HAL_GPIO_WritePin(PYRO1_GPIO_Port, PYRO1_Pin, 1);
            }
            if (pkt.data == 405)
            {
                HAL_GPIO_WritePin(PYRO1_GPIO_Port, PYRO1_Pin, 0);
            }
        }
    }
}

void uart_rx_init(UART_HandleTypeDef *n)
{
    handle = n;
    byte_queue_init(&q);
    HAL_UART_Receive_IT(handle, RX_BUFFER, 1);
}

// handle uart interrupt
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == handle->Instance) // Check if the interrupt belongs to your UART
    {

        // Re-enable the UART receive interrupt for the next byte
        byte_queue_push(&q, RX_BUFFER[0]);
        HAL_UART_Receive_IT(handle, RX_BUFFER, 1);
    }
}