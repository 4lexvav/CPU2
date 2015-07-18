#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "step_utils.h"
#include <stdio.h>
#include <string.h>

ADC_HandleTypeDef hadc1;

GPIO_InitTypeDef GPIOG_Init;

UART_HandleTypeDef huart1;
RingBuffer rBuffer;

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
    huart1.Init.BaudRate = 9600; // скорость работы порта
    huart1.Init.WordLength = UART_WORDLENGTH_8B; // длинна пакета
    huart1.Init.StopBits = UART_STOPBITS_1; // длинна стоп-бита. У нас 1 или 2
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX; // Используем режим приемопередатчик TransmitX_ReceiveX
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16; // Частота опроса линии (проверка бита, его считывание)
    HAL_UART_Init(&huart1);

    NVIC_EnableIRQ(USART1_IRQn); // включаем прерывания UART1
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE); // Генерирование прерываний на прием сигнала
}

void ADC1_Init()
{
    GPIO_InitTypeDef portC;
    __GPIOC_CLK_ENABLE();
    portC.Pin = GPIO_PIN_3;
    portC.Mode = GPIO_MODE_ANALOG;
    portC.Pull = GPIO_NOPULL;
    portC.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(GPIOC, &portC);

    __ADC1_CLK_ENABLE();
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

void LedInit()
{
    __GPIOG_CLK_ENABLE();

    // ñâåòîäèîäû íàñòðàèâàåì íà âûõîä
    GPIOG_Init.Pin   = GPIO_PIN_14 | GPIO_PIN_13; // 13 - çåëåíûé, 14 - êðàñíûé
    GPIOG_Init.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIOG_Init.Pull  = GPIO_NOPULL;
    GPIOG_Init.Speed = GPIO_SPEED_LOW;

    HAL_GPIO_Init(GPIOG, &GPIOG_Init); // èíèçèàëèçèðóåì ïîðò, ïåðåäàåì ñòðóêòóðó
}

void brdLedInit()
{
    GPIO_InitTypeDef portB;

    __GPIOB_CLK_ENABLE();

    portB.Pin   = GPIO_PIN_7 | GPIO_PIN_4;
    portB.Mode  = GPIO_MODE_OUTPUT_PP;
    portB.Pull  = GPIO_NOPULL;
    portB.Speed = GPIO_SPEED_LOW;

    HAL_GPIO_Init(GPIOB, &portB);
}

uint32_t adcGetValue(uint32_t adc_channel)
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

void PrintBuf()
{
    char str[128]; // áóôåð äëÿ õðàíåíèÿ ñòðîêè íà îòïðàâêó
    int i =0;
    while(!RingBuffer_IsEmpty(&rBuffer))
    {
        char c = RingBuffer_GetByte(&rBuffer);
        if (c == '\r') continue;
        str[i++%128] = c;
        if (c == '\n' || i>=34)
        {
            if (c == '\n') str[i - 1] = 0;
            else str[i] = 0;

            STEP_Println(str);

            str[i] = '\r';
            str[i + 1] = '\n';
            str[i + 2] = 0;

            HAL_UART_Transmit(&huart1, str, strlen(str), 100);

            i=0;
        }
    }
}

void init()
{
    SystemInit();
    HAL_Init();
    STEP_Init();
    LedInit();
    brdLedInit();
    ADC1_Init();

    STEP_DBG_Osc();

    UART_Init();
}

void listenCommands()
{
    char command[128], c, msg[20];
    int i = 0, stop = 0, value;

    while (!RingBuffer_IsEmpty(&rBuffer)) {
        c = RingBuffer_GetByte(&rBuffer);
        if (c == '\r') continue;
        if (c == '\n') stop = 1;

        command[i++%128] = c;

        if (stop) {
            command[i - 1] = 0;
            sprintf(msg, "Command: %s", command);
            STEP_Println(msg);

            if (strcmpi(command, "led1on") == 0) {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
            } else if (strcmpi(command, "led1off") == 0) {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
            } else if (strcmpi(command, "led2on") == 0) {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
            } else if (strcmpi(command, "led2off") == 0) {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
            } else if (strcmpi(command, "lum") == 0) {
                value = adcGetValue(ADC_CHANNEL_13);
                sprintf(msg, "ADC value = %d", value);
                STEP_Println(msg);
                strcat(msg, "\r\n\0");
                HAL_UART_Transmit(&huart1, msg, strlen(msg), 100);
            }
        }

    }
}

int main(void)
{
    init();
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);

    while (1)
    {
        // PrintBuf();
        listenCommands();

        HAL_Delay(1000);
        //STEP_ClrScr();
    }
}

void USART1_IRQHandler()
{
    STEP_UART_Receive_IT(&huart1, &rBuffer);
}

void SysTick_Handler()
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}
