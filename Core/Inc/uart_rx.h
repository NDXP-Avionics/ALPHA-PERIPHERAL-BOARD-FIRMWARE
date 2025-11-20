#ifndef UART_RX_H
#define UART_RX_H
#include <stdint.h>
#include "main.h"
#include "Alpha.h"

void RX_HANDLER(Alpha *a);

void uart_rx_init(UART_HandleTypeDef *);

#endif