#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "step_utils.h"
#include <stdio.h>
#include <string.h>

GPIO_InitTypeDef GPIOG_Init;

UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1;
RingBuffer rBuffer;

uint32_t ADC_GetValue(uint32_t adc_channel);


// PWM initialization
TIM_HandleTypeDef htim6;

TIM_HandleTypeDef htim9;
TIM_OC_InitTypeDef sConfigOC; // структура для настройки каналов таймера

void TimersInit(){
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
    GPIO_InitTypeDef GPIOE_Init;

    GPIOE_Init.Pin   = GPIO_PIN_5;
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
    //sConfigOC.Pulse = 2000; // 2000 микросекунд
    sConfigOC.Pulse = 2500; // 2000 микросекунд
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    HAL_TIM_PWM_ConfigChannel(&htim9, &sConfigOC, TIM_CHANNEL_1);

    /*sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 1440; // 1440 микросекунд
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    HAL_TIM_PWM_ConfigChannel(&htim9, &sConfigOC, TIM_CHANNEL_2);*/
}

// End of PWM initialization

// структура для хранения соответствий Команда - Функция
struct Command
{
    char * name; // здесь будет текст команды
    void (*func)(void); // функция, которую надо выполнить
};

struct Command commands[5]; // у нас 3 команды в задаче

void toggleGreen()
{
    HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_13);
}

void toggleRed()
{
    HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_14);
}

void lock()
{
    setPWM(2500);
}

void unlock()
{
    setPWM(500);
}

void sendLum()
{
    char buf[50];
    int value = ADC_GetValue(ADC_CHANNEL_13);
    sprintf(buf, "ADC = %d\r\n", value);
    HAL_UART_Transmit(&huart1, buf, strlen(buf), 100);
}

void AddActions() // заполняем массив нашими командами
{
    commands[0].name = "led1"; // команда
    commands[0].func = toggleGreen; // функция

    commands[1].name = "led2";
    commands[1].func = toggleRed;

    commands[2].name = "lum";
    commands[2].func = sendLum;

    commands[3].name = "lock";
    commands[3].func = lock;

    commands[4].name = "unlock";
    commands[4].func = unlock;
}

void MakeAction(char * cmd)
{
    int i = 0;
    for (i = 0; i <= 4; i++) // проходимся по всем командам в массиве
    {
        if (strcmp(commands[i].name, cmd) == 0) // нашли команду
        {
            commands[i].func(); // вызываем ее функцию
        }
    }
}

void UART_Init()
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __GPIOA_CLK_ENABLE();
    __USART1_CLK_ENABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 9600;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);

    NVIC_EnableIRQ(USART1_IRQn); // Включаем прерывания UART1
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
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

void LedInit()
{
    __GPIOG_CLK_ENABLE();

    GPIOG_Init.Pin   = GPIO_PIN_14 | GPIO_PIN_13;
    GPIOG_Init.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIOG_Init.Pull  = GPIO_NOPULL;
    GPIOG_Init.Speed = GPIO_SPEED_LOW;

    HAL_GPIO_Init(GPIOG, &GPIOG_Init);
}

void PrintBuf()
{
    char str[128];
    int i = 0;
    while(!RingBuffer_IsEmpty(&rBuffer))
    {
        char c = RingBuffer_GetByte(&rBuffer);
        if (c == '\r') continue;
        str[i++ %128] = c;
        if (c == '\n' || i>=36)
        {
            if (c == '\n') str[i-1] = 0;
            else str[i] = 0;
            i=0;
            STEP_Println(str);
            MakeAction(str); // реагируем на команду
        }
    }
}

void setPWM(int pulse)
{
    char msg[100];
    sprintf(msg, "Current pulse: %d ; new: %d", TIM9->CCR1, pulse);
    STEP_Println(msg);
    TIM9->CCR1 = pulse; // записываем значение напрямую в регистр таймера
    //TIM9->CCR2 = pulse;
}


int main(void)
{
    SystemInit();
    HAL_Init();
    STEP_Init();
    LedInit();
    AddActions();
    ADC1_Init();
    TimersInit();
    STEP_DBG_Osc();

    UART_Init();

    HAL_TIM_Base_Start_IT(&htim6); // запускаем таймер в режиме прерываний

    HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1); // запускаем генерацию ШИМ сигнала на 1-м канале
    //HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2); // ... на 2-м канале

    while (1)
    {
        PrintBuf();
        HAL_Delay(1000);
    }
}

void USART1_IRQHandler()
{
    STEP_UART_Receive_IT(&huart1, &rBuffer);
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
