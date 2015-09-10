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
            //MakeAction(str); // реагируем на команду
        }
    }
}

void sendUART(char * cmd)
{
    HAL_UART_Transmit(&huart1, cmd, strlen(cmd), 100);
}

#define WIFI_SSID            "ITStep" //"ITStep" // WiFi SSID
#define WIFI_KEY             "stepstudent"   // WiFi key (password)
#define THINGSPEAK_HOST_IP   "184.106.153.149"           // ThingSpeak host IP
#define THINGSPEAK_HOST_PORT  80                         // ThingSpeak host port
#define THINGSPEAK_API_KEY   "P5MHRH8PXY71GGJV"          // ThingSpeak API key 54584

/**
 * Available Commands: https://github.com/espressif/esp8266_at/wiki/AT_Description
 *
 */
void connectWiFi(void)
{
    sendUART("AT\r\n");

    HAL_Delay(100);
    PrintBuf();

    sendUART("AT+CWMODE=1\r\n");    // Переключение режима WiFi (1=Station, 2=AP, 3=Station+AP)
    HAL_Delay(100);
    PrintBuf();

    char buf[200];
    sprintf(buf, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_KEY);
    sendUART(buf);

    HAL_Delay(6000);
    PrintBuf();

    sendUART("AT+CIFSR\r\n");       // Отобразить IP адрес, который получили от AP

    HAL_Delay(1000);
    PrintBuf();

    sendUART("AT+CIPMUX=0\r\n");    // Выбор режима одиночных или множественных подключений (0=одиночные подключение, 1=множественные подключения)

    HAL_Delay(100);
    PrintBuf();

}

void disconnectWiFi(void)
{
    sendUART("AT+CWQAP\r\n");

    HAL_Delay(2000);
    PrintBuf();
}

void sendData(char *fieldName, const char *fieldValue)
{
    char buf[200];
    sprintf(buf, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", THINGSPEAK_HOST_IP, THINGSPEAK_HOST_PORT);
    sendUART(buf);

    HAL_Delay(1000);
    PrintBuf();

    char dbuf[2048];
    sprintf(dbuf, "GET /update?key=%s&%s=%d\r\n", THINGSPEAK_API_KEY, fieldName, fieldValue);

    // Count of data length
    int dbufLength = 0;
    while(dbuf[dbufLength] != '\n'){
      dbufLength++;
    }
    dbufLength++;

    sprintf(buf, "AT+CIPSEND=%d\r\n", dbufLength);
    sendUART(buf);

    HAL_Delay(20);

    sendUART(dbuf);

    HAL_Delay(100);
    //PrintBuf();

    sendUART("AT+CIPCLOSE\r\n");
    //HAL_Delay(2000);
    //PrintBuf();
}

int main(void)
{
    SystemInit();
    HAL_Init();
    STEP_Init();
    LedInit();
    ADC1_Init();
    //STEP_DBG_Osc();

    UART_Init();

    connectWiFi();

    sendData("field1", 9);

    //disconnectWiFi();


    //STEP_Println("Connected?");

    //sendUART("AT+CWLAP\r\n");

    //HAL_Delay(1000);

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

void SysTick_Handler()
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}
