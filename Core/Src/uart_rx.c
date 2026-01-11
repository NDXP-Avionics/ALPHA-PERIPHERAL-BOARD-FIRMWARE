#include "uart_rx.h"
#include "XPLink.h"
#include "dmacirc.h"
#include "byte_queue.h"
#include "Alpha.h"
#include "state_machine.h"

UART_HandleTypeDef *handle;
uint8_t RX_BUFFER[1];

byte_queue_t q;

static uint32_t start_time;

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
                        SM_SET_STATE(a, FIRE_RECIEVED);
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
    }
}

void uart_rx_init(UART_HandleTypeDef *n)
{

    handle = n;
    byte_queue_init(&q);
    HAL_UART_Receive_IT(handle, RX_BUFFER, 1);

    __HAL_UART_CLEAR_OREFLAG(handle);
    __HAL_UART_CLEAR_NEFLAG(handle);
    __HAL_UART_CLEAR_FEFLAG(handle);

    start_time = HAL_GetTick();
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == handle->Instance)
    {
        // The HAL usually clears the flags automatically when calling this,
        // or when you call Receive_IT again.
        HAL_UART_Receive_IT(handle, RX_BUFFER, 1);
    }
}

// handle uart interrupt
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == handle->Instance) // Check if the interrupt belongs to your UART
    {

        // Re-enable the UART receive interrupt for the next byte

        // only read data if it has been 0.5 seconds since plugging in
        if ((HAL_GetTick() - start_time) > 500)
        {
            byte_queue_push(&q, RX_BUFFER[0]);
        }

        HAL_UART_Receive_IT(handle, RX_BUFFER, 1);
    }
}