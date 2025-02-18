/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan2;
UART_HandleTypeDef huart2; // UART handle for UART2

/* USER CODE BEGIN PV */
int flame = 0;
int gas = 0;

CAN_TxHeaderTypeDef TxHeader;
uint8_t TxData[8];
uint32_t TxMailbox;
uint8_t uartData[2];  // Data to be sent to ESP8266 via UART
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN2_Init(void);
static void MX_USART2_UART_Init(void);  // UART initialization function
void CAN_Filter_Config(void);
void CAN_Transmit(void);
int ReadFlameSensor(void);
int ReadGasSensor(void);
void UART_Transmit(void);  // Function to transmit via UART
void Error_Handler(void);

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  MX_GPIO_Init();
  MX_CAN2_Init();
  MX_USART2_UART_Init();  // Initialize UART

  CAN_Filter_Config();

  if(HAL_CAN_Start(&hcan2) != HAL_OK)
  {
    Error_Handler();
  }

  while (1)
  {
    flame = ReadFlameSensor();
    gas = ReadGasSensor();
    CAN_Transmit();  // CAN transmit data
    UART_Transmit(); // Send data over UART to ESP8266
    HAL_Delay(500);  // Delay between iterations
  }
}

/* Reading Flame Sensor (HC-015) */
int ReadFlameSensor(void)
{
    return (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) ? 1 : 0;
}

/* Reading Gas Sensor (MQ-9) */
int ReadGasSensor(void)
{
    return (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) ? 1 : 0;
}

/* CAN Transmit Function */
void CAN_Transmit(void)
{
  TxHeader.StdId = 0x65D;
  TxHeader.RTR = CAN_RTR_DATA;
  TxHeader.IDE = CAN_ID_STD;
  TxHeader.DLC = 2;
  TxHeader.TransmitGlobalTime = DISABLE;

  TxData[0] = flame;
  TxData[1] = gas;

  if(HAL_CAN_AddTxMessage(&hcan2, &TxHeader, TxData, &TxMailbox) != HAL_OK)
  {
    Error_Handler();
  }
}

/* UART Transmit Function to ESP8266 */
void UART_Transmit(void)
{
  uartData[0] = flame;
  uartData[1] = gas;

  if (HAL_UART_Transmit(&huart2, uartData, 2, 1000) != HAL_OK)
  {
    Error_Handler();
  }
}

/* CAN Filter Configuration */
void CAN_Filter_Config(void)
{
  CAN_FilterTypeDef canfilterconfig;

  canfilterconfig.FilterActivation = CAN_FILTER_ENABLE;
  canfilterconfig.FilterBank = 0;
  canfilterconfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  canfilterconfig.FilterIdHigh = 0x0000;
  canfilterconfig.FilterIdLow = 0x0000;
  canfilterconfig.FilterMaskIdHigh = 0x0000;
  canfilterconfig.FilterMaskIdLow = 0x0000;
  canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;
  canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
  canfilterconfig.SlaveStartFilterBank = 14;

  if(HAL_CAN_ConfigFilter(&hcan2, &canfilterconfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/* GPIO Initialization */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* CAN2 Initialization */
static void MX_CAN2_Init(void)
{
  hcan2.Instance = CAN2;
  hcan2.Init.Prescaler = 2;
  hcan2.Init.Mode = CAN_MODE_NORMAL;
  hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan2.Init.TimeSeg1 = CAN_BS1_10TQ;
  hcan2.Init.TimeSeg2 = CAN_BS2_5TQ;
  hcan2.Init.TimeTriggeredMode = DISABLE;
  hcan2.Init.AutoBusOff = DISABLE;
  hcan2.Init.AutoWakeUp = DISABLE;
  hcan2.Init.AutoRetransmission = DISABLE;
  hcan2.Init.ReceiveFifoLocked = DISABLE;
  hcan2.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* UART2 Initialization for Communication with ESP8266 */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;  // Set baud rate for ESP8266
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* Error Handler */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

