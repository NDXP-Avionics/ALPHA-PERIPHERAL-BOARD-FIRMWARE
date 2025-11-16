#ifndef DMACIRC_H
#define DMACIRC_H

#include "main.h"

#include <stdio.h>
#include <stdint.h>

extern UART_HandleTypeDef huart2;

void dmasendinit(UART_HandleTypeDef *);

void dmasend(uint8_t *, uint16_t);

#endif