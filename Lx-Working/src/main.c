#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "step_utils.h"

void switchState();
void checkBtn(int);

// состояние изменяется в обработчике прерывания, а в главном цикле лишь зажигаются светодиоды определенным образом в зависимости от состояния
volatile int index   = 0;
volatile int error   = 0;
volatile int code[4] = {1, 3, 1, 2};

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

void BRD_Btn_B4_Init()
{
    GPIO_InitTypeDef GPIOB_Init;

    // Включаем питание и тактирование на портах B
    __GPIOB_CLK_ENABLE();

    GPIOB_Init.Pin   = GPIO_PIN_4;
    // https://ru.wikipedia.org/wiki/Фронт_сигнала
    // GPIOB_Init.Mode = GPIO_MODE_IT_FALLING; //- по заднему фронту (спадающий)
    // GPIOA_Init.Mode = GPIO_MODE_IT_RISING_FALLING; // - на каждое нажатие кнопки будет сгенерировано два события, и по переднему и по заднему фронту
    GPIOB_Init.Mode  = GPIO_MODE_IT_RISING; // событие будет сгенерировано в момент нажатия кнопки (восходящий, передний фронт)
    GPIOB_Init.Pull  = GPIO_PULLDOWN; // подтягиваем ее к земле
    GPIOB_Init.Speed = GPIO_SPEED_MEDIUM;

    HAL_GPIO_Init(GPIOB, &GPIOB_Init); // - инициализируем порт, передаем структуру
}

void BRD_Btn_B7_Init()
{
    GPIO_InitTypeDef GPIOB_Init;

    __GPIOB_CLK_ENABLE();

    GPIOB_Init.Pin   = GPIO_PIN_7;
    GPIOB_Init.Mode  = GPIO_MODE_IT_RISING;
    GPIOB_Init.Pull  = GPIO_PULLDOWN;
    GPIOB_Init.Speed = GPIO_SPEED_MEDIUM;

    HAL_GPIO_Init(GPIOB, &GPIOB_Init);
}

void BRD_Btn_C3_Init()
{
    GPIO_InitTypeDef GPIOC_Init;

    __GPIOC_CLK_ENABLE();

    GPIOC_Init.Pin   = GPIO_PIN_3;
    GPIOC_Init.Mode  = GPIO_MODE_IT_RISING;
    GPIOC_Init.Pull  = GPIO_PULLDOWN;
    GPIOC_Init.Speed = GPIO_SPEED_LOW;

    HAL_GPIO_Init(GPIOC, &GPIOC_Init);
}

void init()
{
    SystemInit();
    HAL_Init();
    STEP_Init();

    Led_Init();

    BRD_Btn_B4_Init();
    BRD_Btn_B7_Init();
    BRD_Btn_C3_Init();

    // Говорим нашему контроллеру прерываний (NVIC), что мы хотим обрабатывать прерывание EXTI4_IRQn
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);
}

int main(void)
{
    init();

    switchState();

    return 0;
}

// название обработчика прерывания аналогично прерыванию (EXTI0_IRQn) только вместо "n" - "Handler"
void EXTI4_IRQHandler(void)
{
    // Тут выполняется ваш код, обработчик прерывания
    int btn = 1;

    checkBtn(btn);

    char msg[25];
    sprintf(msg, "Button: %d, count: %d", btn, index);
    STEP_Println(msg);

    // Тут выполняется ваш код, обработчик прерывания
    HAL_NVIC_ClearPendingIRQ(EXTI4_IRQn);
    // Тут выполняется ваш код, обработчик прерывания
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

void EXTI9_5_IRQHandler(void)
{
    int btn = 2;

    checkBtn(btn);

    char msg[25];
    sprintf(msg, "Button: %d, count: %d", btn, index);
    STEP_Println(msg);

    HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
}

void EXTI3_IRQHandler(void)
{
    int btn = 3;

    checkBtn(btn);

    char msg[25];
    sprintf(msg, "Button: %d, index: %d", btn, index);
    STEP_Println(msg);

    HAL_NVIC_ClearPendingIRQ(EXTI3_IRQn);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

void switchState()
{
    while (1) {
        if (index == 4) {
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, 1);
            continue;
        } else {
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, 0);
            continue;
        }
    }
}

void checkBtn(int btnNum)
{
    if (code[index] == btnNum) {
        index++;
    } else {
        index = 0;
    }
}

void SysTick_Handler()
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}
