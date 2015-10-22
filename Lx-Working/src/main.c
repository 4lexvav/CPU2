#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "step_utils.h"
#include <stdio.h>
#include <string.h>


/* Include my libraries here */
#include "defines.h"
#include "tm_stm32_disco.h"
#include "tm_stm32_delay.h"
#include "tm_stm32_rtc.h"
#include "tm_stm32_lcd.h"

#define EEPROM_WRITE
#define EEPROM_DEVICE_ADDRESS   0x50
#define EEPROM_ACCESS_TIMEOUT   1000
#define EEPROM_ACCESS_TRY_CNT   5


GPIO_InitTypeDef GPIOG_Init;

I2C_HandleTypeDef hi2c3;

uint8_t EEPROM_writeData(uint16_t addr, uint8_t *data, uint16_t len);
uint8_t EEPROM_readData(uint16_t addr, uint8_t *data, uint16_t len);

I2C_HandleTypeDef hI2C;

/* RTC structure */
TM_RTC_t RTCD;

/* RTC Alarm structure */
TM_RTC_AlarmTime_t RTCA;

/* String variable for LCD */
char str[100];

/* Current seconds value */
uint8_t sec = 0;

int main(void) {
    /* Init system clock for maximum system speed */
    TM_RCC_InitSystem();

    /* Init HAL layer */
    HAL_Init();

    /* Init leds */
    TM_DISCO_LedInit();

    /* Init button */
    TM_DISCO_ButtonInit();

    /* Init LCD */
    TM_LCD_Init();

#if defined(STM32F429_DISCOVERY)
    /* Rotate LCD */
    TM_LCD_SetOrientation(2);
#endif

    /* Set write location */
    TM_LCD_SetXY(10, 10);

    /* Init RTC */
    if (TM_RTC_Init(TM_RTC_ClockSource_External)) {
        /* RTC was already initialized and time is running */
        TM_LCD_Puts("RTC already initialized!");
    } else {
        /* RTC was now initialized */
        /* If you need to set new time, now is the time to do it */
        TM_LCD_Puts("RTC initialized first time!");
    }

    /* Wakeup IRQ every 1 second */
    TM_RTC_Interrupts(TM_RTC_Int_1s);

    while (1) {
        /* Read RTC */
        TM_RTC_GetDateTime(&RTCD, TM_RTC_Format_BIN);

        /* Check difference */
        if (RTCD.Seconds != sec) {
            sec = RTCD.Seconds;

            /* Format date */
            sprintf(str, "Date: %02d.%02d.%04d", RTCD.Day, RTCD.Month, RTCD.Year + 2000);

            /* Set LCD location */
            TM_LCD_SetXY(10, 50);

            /* Print time to LCD */
            TM_LCD_Puts(str);

            /* Format time */
            sprintf(str, "Time: %02d.%02d.%02d", RTCD.Hours, RTCD.Minutes, RTCD.Seconds);

            /* Set LCD location */
            TM_LCD_SetXY(10, 70);

            /* Print time to LCD */
            TM_LCD_Puts(str);
        }

        /* Check if button pressed */
        if (TM_DISCO_ButtonPressed()) {
            /* Set new date and time */
            RTCD.Day = 4;
            RTCD.Month = 5;
            RTCD.Year = 15;
            RTCD.WeekDay = 5;
            RTCD.Hours = 23;
            RTCD.Minutes = 59;
            RTCD.Seconds = 50;

            /* Save new date and time */
            TM_RTC_SetDateTime(&RTCD, TM_RTC_Format_BIN);

            /* Trigger alarm now after new time was set */
            RTCA.Type = TM_RTC_AlarmType_DayInWeek;
            RTCA.Hours = RTCD.Hours;
            RTCA.Minutes = RTCD.Minutes;
            RTCA.Seconds = RTCD.Seconds + 5;
            RTCA.Day = RTCD.Day;

            /* Enable alarm */
            TM_RTC_EnableAlarm(TM_RTC_Alarm_A, &RTCA, TM_RTC_Format_BIN);
        }
    }
}

/* Called when wakeup interrupt happens */
void TM_RTC_WakeupHandler(void) {
    /* Toggle leds */
    TM_DISCO_LedToggle(LED_ALL);
}

/* Handle Alarm A event */
void TM_RTC_AlarmAHandler(void) {
    /* Print to LCD */
    TM_LCD_SetXY(10, 90);
    TM_LCD_Puts("ALARM TRIGGERED!");

    /* Disable alarm */
    TM_RTC_DisableAlarm(TM_RTC_Alarm_A);
}

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

/*int main(void)
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
}*/

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
