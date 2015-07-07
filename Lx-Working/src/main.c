#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "step_utils.h"
#include <stdio.h>

// структуры инициализации портов и их пинов
ADC_HandleTypeDef hadc1;
GPIO_InitTypeDef GPIOG_Init;
GPIO_InitTypeDef GPIOE_Init;
TIM_HandleTypeDef htim6;

TIM_HandleTypeDef htim9;
TIM_OC_InitTypeDef sConfigOC; // структура для настройки каналов таймера

void ADC1_Init()
{
    GPIO_InitTypeDef portCinit;

    __ADC1_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();

    portCinit.Pin  = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    portCinit.Mode = GPIO_MODE_ANALOG;
    portCinit.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &portCinit);

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV2;
    hadc1.Init.Resolution            = ADC_RESOLUTION12b;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection          = EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);
}

void TimersInit()
{
// BASIC Timer init
    __TIM6_CLK_ENABLE();
    htim6.Instance = TIM6;

    // Частота таймера - 168 MHz
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP; // считает от 0 вверх
    htim6.Init.Prescaler = 4 - 1; // Предделитель 4 (счет идет от 0!)
    htim6.Init.Period = 420 - 1; // Период 420 (счет идет от 0!) 4*420 = 1680 (каждые 10 микросекунд)
    HAL_TIM_Base_Init(&htim6); // Инициализируем

    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn); // Разрешаем прерывание таймера

// GP Timer init
    __TIM9_CLK_ENABLE();
    __GPIOE_CLK_ENABLE();

    // PE5 - TIM9_CH1
    // PE6 - TIM9_CH2
    GPIOE_Init.Pin   = GPIO_PIN_5 | GPIO_PIN_6;
    GPIOE_Init.Mode  = GPIO_MODE_AF_PP; // альтернативная функция
    GPIOE_Init.Alternate = GPIO_AF3_TIM9; // название альтернативной функции
    GPIOE_Init.Pull  = GPIO_NOPULL;
    GPIOE_Init.Speed = GPIO_SPEED_LOW;

    HAL_GPIO_Init(GPIOE, &GPIOE_Init);

    htim9.Instance = TIM9;
// сервоприводы работают на частоте 50 Гц
    htim9.Init.Prescaler = 168 - 1; // 168 MHz / 168 = 1 MHz
    htim9.Init.Period = 20000 - 1; // 50 Hz = 20 ms
    htim9.Init.CounterMode = TIM_COUNTERMODE_UP;

    HAL_TIM_PWM_Init(&htim9);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 2000; // 2000 микросекунд
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    HAL_TIM_PWM_ConfigChannel(&htim9, &sConfigOC, TIM_CHANNEL_1);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 1440; // 1440 микросекунд
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    HAL_TIM_PWM_ConfigChannel(&htim9, &sConfigOC, TIM_CHANNEL_2);
}

void LedInit()
{
    __GPIOG_CLK_ENABLE();

    // светодиоды настраиваем на выход
    GPIOG_Init.Pin   = GPIO_PIN_14 | GPIO_PIN_13; // 13 - зеленый, 14 - красный
    GPIOG_Init.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIOG_Init.Pull  = GPIO_PULLDOWN;
    GPIOG_Init.Speed = GPIO_SPEED_HIGH;

    HAL_GPIO_Init(GPIOG, &GPIOG_Init); // инизиализируем порт, передаем структуру
}

void setPWM(int pulse)
{
    TIM9->CCR1 = pulse; // записываем значение напрямую в регистр таймера
    TIM9->CCR2 = pulse;
}

uint32_t ADC_GetValue(uint32_t adc_channel)
{
    ADC_ChannelConfTypeDef sConfig;

    sConfig.Channel      = adc_channel;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 1);

    return HAL_ADC_GetValue(&hadc1);
}

int main(void)
{
    SystemInit();
    HAL_Init();
    STEP_Init();
    STEP_DBG_Osc();

    LedInit();
    TimersInit();
    ADC1_Init();

    char* msg[30];
    int value;

    while (1) {

        HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_13);

        value = ADC_GetValue(ADC_CHANNEL_13);

        sprintf(msg, "ADC value = %d", msg);
        STEP_Println(msg);

        HAL_Delay(400);
    }
}

// Обработчик прерывания таймера
void TIM6_DAC_IRQHandler()
{
    HAL_TIM_IRQHandler(&htim6);
}

void SysTick_Handler()
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}
