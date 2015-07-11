#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "step_utils.h"
#include <stdio.h>

// структуры инициализации портов и их пинов
ADC_HandleTypeDef hadc1;

void start();

volatile int adcValue;

void ADC1_Init(void)
{
    GPIO_InitTypeDef portCinit;

    __ADC1_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();

    portCinit.Pin   = GPIO_PIN_3;
    portCinit.Mode  = GPIO_MODE_ANALOG; // Выставляем его в аналоговый режим
    portCinit.Pull  = GPIO_NOPULL; // без подтягивающих резисторов
    HAL_GPIO_Init(GPIOC, &portCinit); // инициализируем порт С

    hadc1.Instance = ADC1; // указываем какой АЦП хотим использовать. Варианты ADC1, ADC2, ADC3
    hadc1.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV2; // Делитель частоты для АЦП
    hadc1.Init.Resolution = ADC_RESOLUTION12b; // точность (разрешение) АЦП. Выставили точность 12 бит (максимум)
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1); // инициализируем АЦП
}

void LED_Init()
{
    GPIO_InitTypeDef portGinit;

    __GPIOG_CLK_ENABLE();
    portGinit.Pin   = GPIO_PIN_14 | GPIO_PIN_13;
    portGinit.Mode  = GPIO_MODE_OUTPUT_PP;
    portGinit.Pull  = GPIO_NOPULL;
    portGinit.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(GPIOG, &portGinit);
}

uint32_t ADC_GetValue(uint32_t adc_channel)
{
    ADC_ChannelConfTypeDef sConfig;

    sConfig.Channel = adc_channel; // номер канала, который передали в функцию
    sConfig.Rank = 1; // его порядок в очереди. В нашем случае он один
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES; // сколько тактов на сэмплирование.
    HAL_ADC_ConfigChannel(&hadc1, &sConfig); // вызываем функцию настройки канала. Передаем ей структуру АЦП и структуру настроек канала

    // Считываем значение АЦП
    HAL_ADC_Start(&hadc1); // запускаем оцифровку сигнала
    HAL_ADC_PollForConversion(&hadc1, 1); // дожидаемся, пока не получим результат

    return HAL_ADC_GetValue(&hadc1);
}

void init()
{
    SystemInit();
    HAL_Init();
    STEP_Init();

    LED_Init();
    ADC1_Init();

    STEP_DBG_Osc();
}

int main(void)
{
    init();
    start();
}

void start()
{
    int width   = BSP_LCD_GetXSize();
    int height  = BSP_LCD_GetYSize();
    int baseAdc = ADC_GetValue(ADC_CHANNEL_13) / 10;
    int xPxPos  = width/2; // Base pixel position on the X axis
    int yPxPos  = 0;
    int currentAdc = baseAdc;

    while (1)
    {
        BSP_LCD_DrawPixel(xPxPos + baseAdc - currentAdc, yPxPos++, LCD_COLOR_YELLOW);

        if (yPxPos >= height)
        {
            BSP_LCD_Clear(LCD_COLOR_BLACK);
            yPxPos = 0;
        }

        currentAdc = ADC_GetValue(ADC_CHANNEL_13) / 10;

        HAL_Delay(50);
    }
}

void SysTick_Handler()
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}
