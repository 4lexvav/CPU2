#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "step_utils.h"

ADC_HandleTypeDef hadc1;

void LED_Init()
{
    GPIO_InitTypeDef portGinit;

    __GPIOG_CLK_ENABLE();
    portGinit.Pin   = GPIO_PIN_14 | GPIO_PIN_13;
    portGinit.Mode  = GPIO_MODE_OUTPUT_PP;
    portGinit.Pull  = GPIO_PULLDOWN;
    portGinit.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(GPIOG, &portGinit);

    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13 | GPIO_PIN_14, 0);
}

void ADC1_Init(void)
{
    GPIO_InitTypeDef portCinit;
    __ADC1_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();

    /**
    ADC1 GPIO Configuration
    Из документации прочитали, что
    пин PC0 используется, как 10 канал для АЦП1
    PC1 - 11
    PC2 - 12
    PC3 - 13
    */
    portCinit.Pin   = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    portCinit.Mode  = GPIO_MODE_ANALOG; // Выставляем его в аналоговый режим
    portCinit.Pull  = GPIO_NOPULL; // без подтягивающих резисторов
    HAL_GPIO_Init(GPIOC, &portCinit); // инициализируем порт С

    /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    */
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

uint32_t ADC_GetValue(uint32_t adc_channel)
{
    // Настраиваем канал
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

int main(void)
{
    SystemInit();
    HAL_Init();
    STEP_Init();

    LED_Init();
    ADC1_Init();

    char* msg[30];

    while (1)
    {
        HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_13); // моргаем зеленым светодиодом

        int value = ADC_GetValue(ADC_CHANNEL_13); // можно использовать каналы ADC_CHANNEL_11, 12 и 13

        sprintf(msg, "ADC value = %d", value);
        STEP_Println(msg);

        HAL_Delay(1000);
    }
}

void SysTick_Handler()
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}
