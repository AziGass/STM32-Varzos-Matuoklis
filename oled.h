#ifndef OLED_H
#define OLED_H

#include "stm32l0xx_hal.h"

// Funkcijos, kurias naudoja tavo main.c
void OLED_Init(void);
void OLED_ClearPage(uint8_t page);
void OLED_PrintRes(uint8_t x, uint8_t y, int res);
void OLED_PrintOL(uint8_t page, uint8_t id); // <--- PRIDETA ŠI EILUTE

#endif