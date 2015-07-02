#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdio.h>
#include "step_utils.h"a

#define DOWN 0
#define UP   1

// структуры инициализации портов и их пинов
GPIO_InitTypeDef GPIOG_Init;
TIM_HandleTypeDef htim6;

volatile int mode = DOWN;

void TimersInit()
{
// BASIC Timer init
    __TIM6_CLK_ENABLE();
    htim6.Instance = TIM6;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP; // считает от 0 вверх
    htim6.Init.Prescaler = 8400-1; // Предделитель
    htim6.Init.Period = 10000-1; // Период
    HAL_TIM_Base_Init(&htim6); // Инициализируем

    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn); // Разрешаем прерывание таймера.У 7-го - TIM7_IRQn
}

void LedInit()
{
    __GPIOG_CLK_ENABLE();

    // светодиоды настраиваем на выход
    GPIOG_Init.Pin   = GPIO_PIN_14 | GPIO_PIN_13; // 13 - зеленый, 14 - красный
    GPIOG_Init.Mode  = GPIO_MODE_OUTPUT_PP;
    // GPIOG_Init.Pull  = GPIO_PULLDOWN;
    GPIOG_Init.Pull  = GPIO_NOPULL;
    GPIOG_Init.Speed = GPIO_SPEED_LOW;

    HAL_GPIO_Init(GPIOG, &GPIOG_Init); // инизиализируем порт, передаем структуру

}

int main(void)
{
    SystemInit();
    HAL_Init();
    STEP_Init();
    LedInit();
    // TimersInit();
    STEP_DBG_Osc();

    int change = 1, delay = 0, base = 25;

    //HAL_TIM_Base_Start_IT(&htim6); // запускаем таймер в режиме прерываний
    while (1)
    {
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, 1);
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, 0);
        HAL_Delay(delay);

        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, 1);
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, 0);
        HAL_Delay(base - delay);

        delay += change;
        if (delay >= base) change = -1;
        else if (delay <= 0) change = 1;
    }
}

// Обработчик прерывания таймера
void TIM6_DAC_IRQHandler()
{
    HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_13);

    int step = 500;

    char msg[30];
    sprintf(msg, "Period = %d", TIM6->ARR);
    STEP_Println(msg);

    switch (mode) {
        case UP:
            TIM6->ARR += step;
            break;

        case DOWN:
            TIM6->ARR -= step;
            break;
    }

    TIM6->CNT = 0;

    if (TIM6->ARR >= 10000 - 1) {
        mode = DOWN;
    } else if (TIM6->ARR <= 1000) {
        mode = UP;
    }

    HAL_TIM_IRQHandler(&htim6); // нужно для работы библиотеки HAL
}

void SysTick_Handler()
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}
