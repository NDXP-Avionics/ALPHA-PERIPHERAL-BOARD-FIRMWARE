#include "byte_queue.h"
#include "main.h"

void byte_queue_init(byte_queue_t *q)
{
    q->head = 0;
    q->tail = 0;
}

void byte_queue_push(byte_queue_t *q, uint8_t byte)
{
    __disable_irq();

    uint16_t next_head = (q->head + 1) % qSIZE;
    if (next_head == q->tail)
    {
        __enable_irq();
        return;
    }

    q->data[q->head] = byte;
    q->head = next_head;

    __enable_irq();
}

int byte_queue_pop(byte_queue_t *q)
{
    int byte = -1; // Default to -1 (empty)

    // --- Critical Section Start ---
    __disable_irq(); // Disable all interrupts

    // empty check
    if (q->head != q->tail)
    {
        byte = q->data[q->tail];
        q->tail = (q->tail + 1) % qSIZE;
    }

    __enable_irq(); // Re-enable interrupts
    // --- Critical Section End ---

    return byte;
}