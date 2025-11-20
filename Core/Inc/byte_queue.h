#include <stdint.h>

#define qSIZE 1024

typedef struct
{
    volatile uint16_t head;
    volatile uint16_t tail;
    uint8_t data[qSIZE];
} byte_queue_t;

void byte_queue_init(byte_queue_t *);

void byte_queue_push(byte_queue_t *, uint8_t);

int byte_queue_pop(byte_queue_t *);