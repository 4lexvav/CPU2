#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "step_utils.h"

void switchState();
void doubleClick();
void changeState();
void trackDelay();
void pwdSwitch();
void resetBtnStates();

void checkBtn(int);

// состояние изменяется в обработчике прерывания, а в главном цикле лишь зажигаются светодиоды определенным образом в зависимости от состояния
volatile int state       = 0;
volatile int start       = 0;

volatile int button      = 0;
volatile int isCorrect   = 2;
volatile int state_btn_1 = 0;
volatile int state_btn_2 = 0;
volatile int state_btn_3 = 0;

volatile int index   = 0;
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
    GPIOA_Init.Mode  = GPIO_MODE_IT_RISING_FALLING; // - на каждое нажатие кнопки будет сгенерировано два события, и по переднему и по заднему фронту
    GPIOA_Init.Pull  = GPIO_PULLDOWN; // подтягиваем ее к земле
    GPIOA_Init.Speed = GPIO_SPEED_LOW;

    HAL_GPIO_Init(GPIOA, &GPIOA_Init); // - инициализируем порт, передаем структуру
}

void BRD_Btn_B4_Init()
{
    GPIO_InitTypeDef GPIOB_Init;

    __GPIOB_CLK_ENABLE();

    GPIOB_Init.Pin   = GPIO_PIN_4;
    GPIOB_Init.Mode  = GPIO_MODE_IT_RISING;
    GPIOB_Init.Pull  = GPIO_PULLDOWN;
    GPIOB_Init.Speed = GPIO_SPEED_MEDIUM;

    HAL_GPIO_Init(GPIOB, &GPIOB_Init);
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
    // Btn_Init();

    BRD_Btn_B4_Init();
    BRD_Btn_B7_Init();
    BRD_Btn_C3_Init();

    // Говорим нашему контроллеру прерываний (NVIC), что мы хотим обрабатывать прерывание EXTI0_IRQn
    // HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    // Handle breadboard interruption
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);
}

int main(void)
{
    init();

    // switchState();
    pwdSwitch();

    return 0;
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

void EXTI4_IRQHandler(void)
{
    button = 1;
    state_btn_1++;

    char msg[25];
    sprintf(msg, "Button: %d, count: %d", button, state_btn_1);
    STEP_Println(msg);

    HAL_NVIC_ClearPendingIRQ(EXTI4_IRQn);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

void EXTI9_5_IRQHandler(void)
{
    button = 2;
    state_btn_2++;

    char msg[25];
    sprintf(msg, "Button: %d, count: %d", button, state_btn_2);
    STEP_Println(msg);


    HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
}

void EXTI3_IRQHandler(void)
{
    button = 3;
    state_btn_3++;

    char msg[25];
    sprintf(msg, "Button: %d, count: %d", button, state_btn_3);
    STEP_Println(msg);

    HAL_NVIC_ClearPendingIRQ(EXTI3_IRQn);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

/**
 * Double click within 200 ms to change state
 */
void doubleClick()
{
    char msg[40];

    if (start == 0) {
        start = HAL_GetTick();
    } else {
        sprintf(msg, "Time: %d", HAL_GetTick() - start);
        STEP_Println(msg);

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
    char msg[40];
    if (start == 0) {
        start = HAL_GetTick();
    } else {
        sprintf(msg, "Pressing delay is %d", HAL_GetTick() - start);
        STEP_Println(msg);

        state = (HAL_GetTick() - start < 500) ? 1 : 2;
        start = 0;
    }
}

/**
 * Validate password, a combination of pressing three buttons
 * true  - green
 * false - red
 */
void pwdSwitch()
{
    int pwd_btn_1 = 3, pwd_btn_2 = 1, pwd_btn_3 = 2;

    while(1) {
        if (state_btn_1 > pwd_btn_1 ||
            state_btn_2 > pwd_btn_2 ||
            state_btn_3 > pwd_btn_3
        ) {
            isCorrect = 0;
        }

        if (isCorrect == 1) {
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, 1);
            isCorrect = 2;
            resetBtnStates();
            continue;
        } else if(isCorrect == 2) {
            if (button != 0) {
                HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, 0);
            }
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, 0);
            switch(button) {
                case 1:
                    if (state_btn_1 < pwd_btn_1) continue;
                    break;
                case 2:
                    if (state_btn_1 < pwd_btn_1) {
                        isCorrect = 0;
                    } else if (state_btn_2 < pwd_btn_2) {
                        continue;
                    }
                    break;
                case 3:
                    if (state_btn_2 < pwd_btn_2) {
                        isCorrect = 0;
                    } else if (state_btn_3 < pwd_btn_3) {
                        continue;
                    } else if (state_btn_3 == pwd_btn_3) {
                        isCorrect = 1;
                    }
                    break;
            }
        } else {
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, 1);
            HAL_Delay(200);
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, 0);
            HAL_Delay(150);
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, 1);
            HAL_Delay(200);
            resetBtnStates();
            isCorrect = 2;
        }
    }
}

/**
 * Reset buttons states
 */
void resetBtnStates()
{
    state_btn_1 = state_btn_2 = state_btn_3 = button = 0;
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
