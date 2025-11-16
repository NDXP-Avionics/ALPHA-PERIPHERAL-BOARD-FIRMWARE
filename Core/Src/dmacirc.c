#include "dmacirc.h"
#include <string.h>

#define TX_QUEUE_LEN 24 // 16
#define TX_ITEM_SIZE 36

// tx item
typedef struct
{
    uint8_t data[TX_ITEM_SIZE];
    uint64_t len;
} tx_item;

// uart object
UART_HandleTypeDef *u;

tx_item tx_queue[TX_QUEUE_LEN];
volatile uint8_t tx_head = 0;
volatile uint8_t tx_tail = 0;
volatile uint8_t dma_active = 0;

void dmasendinit(UART_HandleTypeDef *n)
{
    u = n;
}

void dmasend(uint8_t *data, uint16_t size)
{
    int next = (tx_head + 1) % TX_QUEUE_LEN;

    if (next == tx_tail)
    {
        // queue full
        return;
    }

    memcpy(tx_queue[tx_head].data, data, size);
    tx_queue[tx_head].len = size;

    uint8_t start_dma = 0;

    __disable_irq();
    tx_head = next; // Update head

    if (!dma_active) // Check if we need to start
    {
        dma_active = 1;
        start_dma = 1; // Use a flag to start outside the critical section
    }
    __enable_irq();

    if (start_dma)
    {
        HAL_UART_Transmit_DMA(u, tx_queue[tx_tail].data, tx_queue[tx_tail].len);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{

    if (huart != u)
        return;

    // move tail forward
    tx_tail = (tx_tail + 1) % TX_QUEUE_LEN;

    // check for continuing transmit
    if (tx_tail != tx_head)
    {
        HAL_UART_Transmit_DMA(u, tx_queue[tx_tail].data, tx_queue[tx_tail].len);
    }
    else
    {
        dma_active = 0;
    }
}