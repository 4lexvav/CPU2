#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "step_utils.h"
#include <stdio.h>

// структуры инициализации портов и их пинов
ADC_HandleTypeDef hadc1;
GPIO_InitTypeDef GPIOE_Init; // Pins for TIM9
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim9;
TIM_OC_InitTypeDef sConfigOC; // структура для настройки каналов таймера

volatile int adcBase;
volatile int adcCurrent;
volatile yPxPos = 0;

/**
 * 0 - base state, only red light, pulse = 3000
 * 1 - increase pulse brightness to max and during next 1 sec. check ADC
 * 2 - enable green, disable other - for 5 sec., then switch back
 */
volatile int state  = 0;

/**
 * Draw vertical line depending on value of photoresistor
 */
void drawLine();
void smartLights();

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

void TimersInit()
{
// BASIC Timer init
    __TIM6_CLK_ENABLE();
    htim6.Instance = TIM6;

    // Частота таймера - 84 MHz
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP; // считает от 0 вверх
    htim6.Init.Prescaler = 8400 / 50 - 1; // Предделитель
    htim6.Init.Period = 10000 - 1; // Период 10000 (счет идет от 0!) 8400*10000 = 84 000 000 (каждую секунду)
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
    // sConfigOC.Pulse = 2000; // 2000 микросекунд
    sConfigOC.Pulse = 3000; // 2000 микросекунд
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    HAL_TIM_PWM_ConfigChannel(&htim9, &sConfigOC, TIM_CHANNEL_1);
}

void BRD_Led_Init()
{
    GPIO_InitTypeDef portB;

    __GPIOB_CLK_ENABLE();
    portB.Pin = GPIO_PIN_4 | GPIO_PIN_7; // portB4 - yellow, portB7 - green
    portB.Mode = GPIO_MODE_OUTPUT_PP;
    portB.Pull = GPIO_NOPULL;
    portB.Speed = GPIO_SPEED_LOW;

    HAL_GPIO_Init(GPIOB, &portB);

    GPIO_InitTypeDef portA;

    __GPIOA_CLK_ENABLE();
    portA.Pin = GPIO_PIN_10; // portA10 - red
    portA.Mode = GPIO_MODE_OUTPUT_PP;
    portA.Pull = GPIO_NOPULL;
    portA.Speed = GPIO_SPEED_LOW;

    HAL_GPIO_Init(GPIOA, &portA);

}

void LED_Init()
{
    GPIO_InitTypeDef portGinit;

    __GPIOG_CLK_ENABLE();
    portGinit.Pin   = GPIO_PIN_14 | GPIO_PIN_13;
    portGinit.Mode  = GPIO_MODE_OUTPUT_PP;
    // portGinit.Pull  = GPIO_PULLDOWN;
    portGinit.Pull  = GPIO_NOPULL;
    portGinit.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(GPIOG, &portGinit);

    // HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13 | GPIO_PIN_14, 0);
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
    TimersInit();
    ADC1_Init();
    BRD_Led_Init();

    STEP_DBG_Osc();

    HAL_TIM_Base_Start_IT(&htim6); // Start timer in the interruption mode

    HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1); // запускаем генерацию ШИМ сигнала на 1-м канале
}

int main(void)
{
    init();

    smartLights();
}

void setPWM(int pulse)
{
    TIM9->CCR1 = pulse; // записываем значение напрямую в регистр таймера
}

void smartLights()
{
    adcBase = ADC_GetValue(ADC_CHANNEL_13);
    int delay;
    while (1) {
        switch (state) {
            case 0:
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET); // YELLOW
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET); // GREEN
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET); // RED
                setPWM(3000);
                continue;
            case 1:
                setPWM(10000);
                HAL_Delay(1000);
                if ((adcCurrent - adcBase) > 20) {
                    state = 2;
                } else {
                    state = 0;
                }
                setPWM(3000);
                continue;
            case 2:
                delay = 5;
                while (delay) {
                    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_4);
                    delay--;
                    HAL_Delay(400);
                }
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
                HAL_Delay(5000);

                delay = 5;
                while (delay) {
                    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);
                    delay--;
                    HAL_Delay(400);
                }
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
                state = 0;
                continue;
        }
    }
}

void drawLine()
{
    int width   = BSP_LCD_GetXSize();
    int height  = BSP_LCD_GetYSize();
    int xPxPos  = width/2; // Base pixel position on the X axis

    BSP_LCD_DrawPixel(xPxPos + (adcBase / 10) - (adcCurrent / 10), yPxPos++, LCD_COLOR_YELLOW);

    if (yPxPos >= height)
    {
        BSP_LCD_Clear(LCD_COLOR_BLACK);
        yPxPos = 0;
    }

    adcCurrent = ADC_GetValue(ADC_CHANNEL_13);
}

// Обработчик прерывания таймера
void TIM6_DAC_IRQHandler()
{
    drawLine();

    if ((adcCurrent - adcBase) > 20) {
        state = 1;
    }

    HAL_TIM_IRQHandler(&htim6);
}

void SysTick_Handler()
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}
