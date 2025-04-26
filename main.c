#include "stm32f1xx_hal.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <stdio.h>
#include <string.h>

// I2C Handle
I2C_HandleTypeDef hi2c1;

// I2C Addresses
#define MPU6050_ADDR     0xD0
#define MAX30102_ADDR    0xAE

// Function Prototypes
void SystemClock_Config(void);
void GPIO_Init(void);
void I2C1_Init(void);
void MPU6050_Init(void);
void MAX30102_Init(void);
void Read_MPU6050(int16_t *accel);
uint32_t Read_MAX30102_IR(void);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    I2C1_Init();
    MPU6050_Init();
    MAX30102_Init();
    SSD1306_Init();

    while (1) {
        int16_t accel[3];
        uint32_t irValue;

        Read_MPU6050(accel);
        irValue = Read_MAX30102_IR();

        char buffer[32];
        SSD1306_Fill(SSD1306_COLOR_BLACK);

        sprintf(buffer, "AX: %d", accel[0]);
        SSD1306_GotoXY(0, 0);
        SSD1306_Puts(buffer, &Font_7x10, SSD1306_COLOR_WHITE);

        sprintf(buffer, "AY: %d", accel[1]);
        SSD1306_GotoXY(0, 12);
        SSD1306_Puts(buffer, &Font_7x10, SSD1306_COLOR_WHITE);

        sprintf(buffer, "AZ: %d", accel[2]);
        SSD1306_GotoXY(0, 24);
        SSD1306_Puts(buffer, &Font_7x10, SSD1306_COLOR_WHITE);

        sprintf(buffer, "IR: %lu", irValue);
        SSD1306_GotoXY(0, 36);
        SSD1306_Puts(buffer, &Font_7x10, SSD1306_COLOR_WHITE);

        SSD1306_UpdateScreen();
        HAL_Delay(1000);
    }
}

void MPU6050_Init(void) {
    uint8_t data[2] = {0x6B, 0x00};
    HAL_I2C_Master_Transmit(&hi2c1, MPU6050_ADDR, data, 2, HAL_MAX_DELAY);
}

void Read_MPU6050(int16_t *accel) {
    uint8_t data[6];
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, 0x3B, I2C_MEMADD_SIZE_8BIT, data, 6, HAL_MAX_DELAY);
    accel[0] = (int16_t)(data[0] << 8 | data[1]);
    accel[1] = (int16_t)(data[2] << 8 | data[3]);
    accel[2] = (int16_t)(data[4] << 8 | data[5]);
}

void MAX30102_Init(void) {
    uint8_t data;

    // Reset
    data = 0x40;
    HAL_I2C_Mem_Write(&hi2c1, MAX30102_ADDR, 0x09, I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY);
    HAL_Delay(100);

    // Set SpO2 mode
    data = 0x03;
    HAL_I2C_Mem_Write(&hi2c1, MAX30102_ADDR, 0x09, I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY);

    // LED Pulse Amplitudes
    data = 0x1F;
    HAL_I2C_Mem_Write(&hi2c1, MAX30102_ADDR, 0x0C, I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY); // LED1 (RED)
    HAL_I2C_Mem_Write(&hi2c1, MAX30102_ADDR, 0x0D, I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY); // LED2 (IR)
}

uint32_t Read_MAX30102_IR(void) {
    uint8_t data[6];
    HAL_I2C_Mem_Read(&hi2c1, MAX30102_ADDR, 0x07, I2C_MEMADD_SIZE_8BIT, data, 6, HAL_MAX_DELAY);
    uint32_t ir = ((uint32_t)(data[0] & 0x03) << 16) | ((uint32_t)data[1] << 8) | data[2];
    return ir;
}

void I2C1_Init(void) {
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = DISABLE;
    hi2c1.Init.GeneralCallMode = DISABLE;
    hi2c1.Init.NoStretchMode = DISABLE;
    HAL_I2C_Init(&hi2c1);
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

void GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}
