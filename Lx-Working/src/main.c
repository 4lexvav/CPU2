#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "step_utils.h"
#include <stdio.h>
#include <string.h>

#define EEPROM_WRITE
#define EEPROM_DEVICE_ADDRESS   0x50
#define EEPROM_ACCESS_TIMEOUT   1000
#define EEPROM_ACCESS_TRY_CNT   5


GPIO_InitTypeDef GPIOG_Init;

I2C_HandleTypeDef hi2c3;

uint8_t EEPROM_writeData(uint16_t addr, uint8_t *data, uint16_t len);
uint8_t EEPROM_readData(uint16_t addr, uint8_t *data, uint16_t len);

I2C_HandleTypeDef hI2C;

void I2C_Init()
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /**I2C3 GPIO Configuration
    PC9     ------> I2C3_SDA
    PA8     ------> I2C3_SCL
    */

    __GPIOA_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    __I2C3_CLK_ENABLE();

    hI2C.Instance = I2C3;
    hI2C.Init.ClockSpeed = 400000;
    hI2C.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hI2C.Init.OwnAddress1 = 0xEE;
    hI2C.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hI2C.Init.DualAddressMode = I2C_DUALADDRESS_DISABLED;
    hI2C.Init.OwnAddress2 = 0xEF;
    hI2C.Init.GeneralCallMode = I2C_GENERALCALL_DISABLED;
    hI2C.Init.NoStretchMode = I2C_NOSTRETCH_DISABLED;

    HAL_I2C_Init(&hI2C);
    HAL_I2CEx_AnalogFilter_Config(&hI2C, I2C_ANALOGFILTER_DISABLED);
}

int main(void)
{
    SystemInit();
    HAL_Init();
    STEP_Init();

    STEP_DBG_Osc();

    I2C_Init();


    char buf[256];

    #ifdef EEPROM_WRITE

    uint16_t writeAddr = 0x0;
    uint8_t writeData[8] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7};
    uint8_t writeStatus = EEPROM_writeData(writeAddr, &writeData, 8);

    sprintf(buf, "Write: %d, %d, %d, %d, %d, %d, %d, %d", writeData[0], writeData[1], writeData[2], writeData[3], writeData[4], writeData[5], writeData[6], writeData[7]);
    STEP_Println(buf);
    sprintf (buf, "Write status: %d", writeStatus);
    STEP_Println(buf);

    //while (1);
    #endif

    uint16_t readAddr = 0x0;
    uint8_t readData[8];
    uint8_t readStatus = EEPROM_readData(readAddr, &readData, sizeof(readData));

    sprintf(buf, "Read: %d, %d, %d, %d, %d, %d, %d, %d", readData[0], readData[1], readData[2], readData[3], readData[4], readData[5], readData[6], readData[7]);
    STEP_Println(buf);
    sprintf (buf, "Read status: %d", readStatus);
    STEP_Println(buf);

    while (1)
    {
    }
}

void SysTick_Handler()
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}

uint8_t EEPROM_writeData(uint16_t addr, uint8_t *data, uint16_t len)
{
    uint32_t tryCount = EEPROM_ACCESS_TRY_CNT;

    while(HAL_I2C_IsDeviceReady(&hI2C, EEPROM_DEVICE_ADDRESS << 1, EEPROM_ACCESS_TRY_CNT, EEPROM_ACCESS_TIMEOUT) != HAL_OK);

    while(HAL_I2C_Mem_Write(&hI2C, EEPROM_DEVICE_ADDRESS << 1, addr, 2, (uint8_t *)data, len, EEPROM_ACCESS_TIMEOUT) != HAL_OK)
    {
        if (HAL_I2C_GetError(&hI2C) != HAL_I2C_ERROR_AF)
        {
            return 1;
        }
        else
        {
            if(tryCount-- == 0)
                return 0;
        }
    }

    return 1;
}


uint8_t EEPROM_readData(uint16_t addr, uint8_t *data, uint16_t len)
{
    uint32_t tryCount = EEPROM_ACCESS_TRY_CNT;

    while(HAL_I2C_IsDeviceReady(&hI2C, EEPROM_DEVICE_ADDRESS << 1, EEPROM_ACCESS_TRY_CNT, EEPROM_ACCESS_TIMEOUT) != HAL_OK)
    {
    }
    while(HAL_I2C_Mem_Read(&hI2C, EEPROM_DEVICE_ADDRESS << 1, addr, 2, (uint8_t *)data, len, EEPROM_ACCESS_TIMEOUT) != HAL_OK)
    {
        if (HAL_I2C_GetError(&hI2C) != HAL_I2C_ERROR_AF)
        {
          return 1;
        }
        else
        {
            if(tryCount-- == 0)
                return 0;
        }
    }
    return 1;
}
