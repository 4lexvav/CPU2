#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "step_utils.h"

void switchState();
void doubleClick();
void changeState();
void trackDelay();

// состояние изменяется в обработчике прерывания, а в главном цикле лишь зажигаются светодиоды определенным образом в зависимости от состояния
volatile int state = 0;
volatile int start = 0;

void Led_Init()
{
    // структуры инициализации портов и их пинов
    GPIO_InitTypeDef GPIOG_Init;

    __GPIOG_CLK_ENABLE();

    GPIOG_Init.Pin   = GPIO_PIN_14 | GPIO_PIN_13;
    GPIOG_Init.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIOG_Init.Pull  = GPIO_PULLDOWN;
    GPIOG_Init.Speed = GPIO_SPEED_LOW;

    HAL_GPIO_Init(GPIOG, &GPIOG_Init);
}

void Btn_Init()
{
    GPIO_InitTypeDef GPIOA_Init;

    // Включаем питание и тактирование на портах G и A
    __GPIOA_CLK_ENABLE();

    // PORTA, PIN_0 (PA0) - наша кнопка на плате (синяя)
    GPIOA_Init.Pin   = GPIO_PIN_0;
    // https://ru.wikipedia.org/wiki/Фронт_сигнала
    // GPIOA_Init.Mode  = GPIO_MODE_IT_RISING; // событие будет сгенерировано в момент нажатия кнопки (восходящий, передний фронт)
    // GPIOA_Init.Mode = GPIO_MODE_IT_FALLING; //- по заднему фронту (спадающий)
    GPIOA_Init.Mode = GPIO_MODE_IT_RISING_FALLING; // - на каждое нажатие кнопки будет сгенерировано два события, и по переднему и по заднему фронту
    GPIOA_Init.Pull  = GPIO_PULLDOWN; // подтягиваем ее к земле
    GPIOA_Init.Speed = GPIO_SPEED_LOW;

    HAL_GPIO_Init(GPIOA, &GPIOA_Init); // - инициализируем порт, передаем структуру
}

void init()
{
    SystemInit();
    HAL_Init();
    STEP_Init();

    Led_Init();
    Btn_Init();

    // Говорим нашему контроллеру прерываний (NVIC), что мы хотим обрабатывать прерывание EXTI0_IRQn
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

int main(void)
{
    init();
    switchState();
}

void switchState()
{
    int oldState = 0;
    while (1)
    {
        // если состояние не изменилось, то пропускаем итерацию цикла
        if (oldState == state) continue;

        // запоминаем новое состояние
        oldState = state;

        // зажигаем светодиоды в соответствии со значением state
        switch(state) {
        case 0:
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, 0);
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, 0);
            break;

        case 1:
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, 1);
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, 0);
            break;

        case 2:
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, 0);
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, 1);
            break;

        case 3:
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, 1);
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, 1);
            break;
        }
    }
}

// название обработчика прерывания аналогично прерыванию (EXTI0_IRQn) только вместо "n" - "Handler"
void EXTI0_IRQHandler(void)
{
    // Тут выполняется ваш код, обработчик прерывания
    // doubleClick();
    trackDelay();


    // Очищаем флаги прерывания. Говорим, что мы его обработали.
    HAL_NVIC_ClearPendingIRQ(EXTI0_IRQn);
    // нужно для работы нашей библиотеки HAL
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

/**
 * Double click within 200 ms to change state
 */
void doubleClick()
{
    char arr[40];

    if (start == 0) {
        start = HAL_GetTick();
    } else {
        sprintf(arr, "Time: %d", HAL_GetTick() - start);
        STEP_Println(arr);

        if (HAL_GetTick() - start <= 200) {
            changeState();
        }
        start = 0;
    }
}

/**
 * Detect duration of pressing the button
 * < 500 - green
 * > 500 - red
 */
void trackDelay()
{
    char msg[50];
    if (start == 0) {
        start = HAL_GetTick();
    } else {
        sprintf(msg, "Pressing delay is %d", HAL_GetTick() - start);
        STEP_Println(msg);

        state = (HAL_GetTick() - start < 500) ? 1 : 2;
        start = 0;
    }
}

void changeState()
{
    state++;
    state %= 4; // state будет принимать значения только от 0 до 3.
}

void SysTick_Handler()
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}
