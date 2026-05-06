#include "main.h"
#include <string.h>
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;

#define OLED_ADDR 0x78

void OLED_Command(uint8_t cmd)
{
    uint8_t data[2];
    data[0] = 0x00;
    data[1] = cmd;
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, data, 2, 100);
}

void OLED_Init(void)
{
    HAL_Delay(100);

    OLED_Command(0xAE); // Išjungti ekrana

    OLED_Command(0x20); // Page Addressing Mode
    OLED_Command(0x02); 

    OLED_Command(0xA8); // Nustatyti Multiplex Ratio (eiluciu skaiciu)
    OLED_Command(0x2F); // 48 pikseliu aukšciui (48 - 1 = 47 -> 0x2F) SVARBIAUSIAS PAKEITIMAS

    OLED_Command(0xD3); // Display Offset
    OLED_Command(0x00);

    OLED_Command(0x40); // Display Start Line

    OLED_Command(0xA1); // Segment Remap (jei vaizdas veidrodinis, keisti i 0xA0)
    OLED_Command(0xC8); // COM Scan Direction (jei vaizdas aukštyn kojom, keisti i 0xC0)

    OLED_Command(0xDA); // COM Pins Hardware Configuration
    OLED_Command(0x12); 

    OLED_Command(0x81); // Kontrastas
    OLED_Command(0x7F); // Vidutinis

    OLED_Command(0xA4); // Rodyti RAM turini
    OLED_Command(0xA6); // Neinvertuotas (juodas fonas, balti skaiciai)

    OLED_Command(0x8D); // Charge Pump
    OLED_Command(0x14); // Ijungti

    OLED_Command(0xAF); // Ijungti ekrana
    HAL_Delay(100);
}

void OLED_TestPattern(void)
{
    uint8_t temp[129];

    temp[0] = 0x40;

    for(int i = 1; i < 129; i++)
        temp[i] = 0xFF;   // visi pikseliai ijungti

    for(int page = 0; page < 8; page++)
    {
        OLED_Command(0xB0 + page);
        OLED_Command(0x00);
        OLED_Command(0x10);

        HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, temp, 129, 100);
    }
}

const uint8_t font5x7[][5] = {
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}, // 9
    {0x7F,0x09,0x19,0x29,0x46}, // R (indeksas 10)
    {0x14,0x14,0x14,0x14,0x14}  // = (indeksas 11)
};

void OLED_PrintNumber(uint8_t page, int num)
{
    uint8_t temp[65];
    temp[0] = 0x40;

    // Išvalome eilutes buferi
    for(int i = 1; i < 65; i++)
        temp[i] = 0x00;

    char buf[10];
    sprintf(buf, "%d", num);

    int pos = 1;

    for(int k = 0; buf[k] != 0; k++)
    {
        int digit = buf[k] - '0';
        
        // Apsauga, jei bufere atsirastu ne skaicius
        if(digit >= 0 && digit <= 9) {
            for(int i = 0; i < 5; i++)
            {
                if (pos < 65) temp[pos++] = font5x7[digit][i];
            }
            if (pos < 65) temp[pos++] = 0x00; // Tarpas tarp skaiciu
        }
    }

    OLED_Command(0xB0 + page);
    
    // Nustatome stulpelio poslinki i 32 (0x20)
    OLED_Command(0x00); // Low Column
    OLED_Command(0x12); // High Column

    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, temp, 65, 100);
}

void OLED_ClearPage(uint8_t page)
{
    uint8_t temp[65];
    temp[0] = 0x40; // Duomenu rašymo komanda

    for(int i = 1; i < 65; i++)
        temp[i] = 0x00;

    OLED_Command(0xB0 + page); // Nustatome puslapi (0-5 jusu ekranui)
    
    // Nustatome stulpelio poslinki i 32 (0x20)
    OLED_Command(0x00); // Low Column Start Address (0x00)
    OLED_Command(0x12); // High Column Start Address (0x10 + 0x02)

    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, temp, 65, 100);
}

void OLED_SetCursor(uint8_t page, uint8_t col)
{
    OLED_Command(0xB0 + page);
    OLED_Command(0x00 + ((col + 32) & 0x0F));
    OLED_Command(0x10 + ((col + 32) >> 4));
}

void OLED_WriteData(uint8_t dataByte)
{
    uint8_t data[2];
    data[0] = 0x40;
    data[1] = dataByte;
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, data, 2, 100);
}

void OLED_WriteString(uint8_t page, uint8_t col, char *str)
{
    OLED_SetCursor(page, col);

    while(*str)
    {
        if(*str >= '0' && *str <= '9')
        {
            int digit = *str - '0';

            for(int i = 0; i < 5; i++)
                OLED_WriteData(font5x7[digit][i]);

            OLED_WriteData(0x00);
        }
        else if(*str == 'R') // paprasta R raide
        {
            uint8_t R[5] = {0x7E,0x09,0x19,0x29,0x46};
            for(int i=0;i<5;i++) OLED_WriteData(R[i]);
            OLED_WriteData(0x00);
        }
        else if(*str == '=')
        {
            uint8_t eq[5] = {0x14,0x14,0x14,0x14,0x14};
            for(int i=0;i<5;i++) OLED_WriteData(eq[i]);
            OLED_WriteData(0x00);
        }
        else if(*str == ' ')
        {
            OLED_WriteData(0x00);
        }

        str++;
    }
}

void OLED_PrintRes(uint8_t page, uint8_t id, int val)
{
    uint8_t temp[65];
    temp[0] = 0x40; // Data mode
    for(int i = 1; i < 65; i++) temp[i] = 0x00; // Isvalom buferi

    int pos = 1;

    // "R"
    for(int i = 0; i < 5; i++) temp[pos++] = font5x7[10][i];
    temp[pos++] = 0x00;

    // ID (1 arba 2)
    for(int i = 0; i < 5; i++) temp[pos++] = font5x7[id][i];
    temp[pos++] = 0x00;

    // "="
    for(int i = 0; i < 5; i++) temp[pos++] = font5x7[11][i];
    temp[pos++] = 0x00;
    temp[pos++] = 0x00; // Tarpas po lygybes

    // Skaicius (varza)
    char buf[10];
    sprintf(buf, "%d", val);
    for(int k = 0; buf[k] != 0; k++) {
        int digit = buf[k] - '0';
        if(digit >= 0 && digit <= 9) {
            for(int i = 0; i < 5; i++) if(pos < 65) temp[pos++] = font5x7[digit][i];
            if(pos < 65) temp[pos++] = 0x00;
        }
    }

    OLED_Command(0xB0 + page);
    OLED_Command(0x00); // Low Column Start (offset 32)
    OLED_Command(0x12); // High Column Start
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, temp, 65, 100);
}

void OLED_PrintOL(uint8_t page, uint8_t id)
{
    uint8_t temp[65];
    temp[0] = 0x40; // Data mode
    for(int i = 1; i < 65; i++) temp[i] = 0x00; // Išvalom buferi

    int pos = 1;

    // Nupiešiame "R"
    for(int i = 0; i < 5; i++) temp[pos++] = font5x7[10][i];
    temp[pos++] = 0x00;

    // Nupiešiame ID (1 arba 2)
    for(int i = 0; i < 5; i++) temp[pos++] = font5x7[id][i];
    temp[pos++] = 0x00;

    // Nupiešiame "="
    for(int i = 0; i < 5; i++) temp[pos++] = font5x7[11][i];
    temp[pos++] = 0x00;
    temp[pos++] = 0x00; // Tarpas po lygybes

    // Nupiešiame raide "O" (sugeneruotas 5x7 masyvas)
    uint8_t char_O[5] = {0x3E, 0x41, 0x41, 0x41, 0x3E};
    for(int i = 0; i < 5; i++) temp[pos++] = char_O[i];
    temp[pos++] = 0x00;

    // Nupiešiame raide "L" (sugeneruotas 5x7 masyvas)
    uint8_t char_L[5] = {0x7F, 0x40, 0x40, 0x40, 0x40};
    for(int i = 0; i < 5; i++) temp[pos++] = char_L[i];
    temp[pos++] = 0x00;

    OLED_Command(0xB0 + page);
    OLED_Command(0x00); // Low Column Start
    OLED_Command(0x12); // High Column Start
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, temp, 65, 100);
}