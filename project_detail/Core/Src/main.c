/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define BMP180_ADDR 0x77 << 1
#define BMP180_REG_CONTROL 0xF4
#define BMP180_REG_RESULT  0xF6
#define BMP180_CMD_TEMP    0x2E
#define BMP180_CMD_PRESSURE 0x34

#define BMP180_CAL_AC1 0xAA
#define BMP180_CAL_AC2 0xAC
#define BMP180_CAL_AC3 0xAE
#define BMP180_CAL_AC4 0xB0
#define BMP180_CAL_AC5 0xB2
#define BMP180_CAL_AC6 0xB4
#define BMP180_CAL_B1 0xB6
#define BMP180_CAL_B2 0xB8
#define BMP180_CAL_MB 0xBA
#define BMP180_CAL_MC 0xBC
#define BMP180_CAL_MD 0xBE

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
//ADC_HandleTypeDef hadc1;
CAN_HandleTypeDef hcan2;
I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */
int16_t AC1, AC2, AC3, B1, B2, MB, MC, MD;
uint16_t AC4, AC5, AC6;

float temperature = 0;
int motion = 0;
int heat = 0;
int gas = 0;

CAN_TxHeaderTypeDef TxHeader;
CAN_RxHeaderTypeDef RxHeader;
uint8_t TxData[8];
uint32_t TxMailbox;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN2_Init(void);
//static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
int ReadHeatFlameSensor(void);
int ReadMotionSensor(void);
//int ReadSmokeSensor(void);
int CheckSmokeAndControlLED(void);
//float ReadADCValue(void);


void BMP180_Init(void);
void BMP180_ReadCalibrationData(void);
int16_t BMP180_ReadRawTemperature(void);
//int32_t BMP180_ReadRawPressure(void);
float BMP180_GetTemperature(void);
//float BMP180_GetPressure(void);

void CAN_Filter_Config(void);
void CAN_Transmit(void);
void Error_Handler(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  HAL_Init();
  //SystemClock_Config();

  MX_GPIO_Init();
  MX_CAN2_Init();
  //MX_ADC1_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  BMP180_Init();

  CAN_Filter_Config();

  if(HAL_CAN_Start(&hcan2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  while (1)
  {
    /* USER CODE END WHILE */
    motion = ReadMotionSensor();
    heat = ReadHeatFlameSensor();
    temperature = BMP180_GetTemperature();
    gas =  CheckSmokeAndControlLED();


    CAN_Transmit();
    HAL_Delay(500); // Transmit every half second
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
q


/* Reading Heat/Flame Sensor */
int ReadHeatFlameSensor(void)
{
    int mess = 0;

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
    {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
        mess = 1;
    }
    else
    {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
        mess = 0;
    }

    HAL_Delay(100);
    return mess;
}



/* Reading Motion Sensor */
int ReadMotionSensor(void)
{
    int state = 0;
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == 1)
    {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
        state = 1;
    }
    else
    {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);
        state = 0;
    }

    HAL_Delay(100);
    return state;
}






int CheckSmokeAndControlLED(void)
{
    int smokeDetected = 0;

    // Read the state of the D0 pin
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET)  // Replace GPIO_PIN_0 with the correct pin number for D0
    {
        // Smoke detected (D0 pin is HIGH)
        smokeDetected = 1;
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);  // Turn on LED for indication (optional)
    }
    else
    {
        // No smoke detected (D0 pin is LOW)
        smokeDetected = 0;
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);  // Turn off LED (optional)
    }

    return smokeDetected;
}

/* BMP180 Initialization */
void BMP180_Init(void)
{
  BMP180_ReadCalibrationData();
}

/* Reading Calibration Data */
void BMP180_ReadCalibrationData(void)
{
  uint8_t data[22];
  HAL_I2C_Mem_Read(&hi2c1, BMP180_ADDR, BMP180_CAL_AC1, I2C_MEMADD_SIZE_8BIT, data, 22, HAL_MAX_DELAY);

  AC1 = (data[0] << 8) | data[1];
  AC2 = (data[2] << 8) | data[3];
  AC3 = (data[4] << 8) | data[5];
  AC4 = (data[6] << 8) | data[7];
  AC5 = (data[8] << 8) | data[9];
  AC6 = (data[10] << 8) | data[11];
  B1  = (data[12] << 8) | data[13];
  B2  = (data[14] << 8) | data[15];
  MB  = (data[16] << 8) | data[17];
  MC  = (data[18] << 8) | data[19];
  MD  = (data[20] << 8) | data[21];
}

/* Reading Raw Temperature from BMP180 */
int16_t BMP180_ReadRawTemperature(void)
{
  uint8_t cmd = BMP180_CMD_TEMP;
  HAL_I2C_Mem_Write(&hi2c1, BMP180_ADDR, BMP180_REG_CONTROL, I2C_MEMADD_SIZE_8BIT, &cmd, 1, HAL_MAX_DELAY);
  HAL_Delay(5);

  uint8_t data[2];
  HAL_I2C_Mem_Read(&hi2c1, BMP180_ADDR, BMP180_REG_RESULT, I2C_MEMADD_SIZE_8BIT, data, 2, HAL_MAX_DELAY);
  return (data[0] << 8) | data[1];
}

/* Getting Temperature from BMP180 */
float BMP180_GetTemperature(void)
{
  int32_t UT = BMP180_ReadRawTemperature();

  int32_t X1 = (UT - AC6) * AC5 / (1 << 15);
  int32_t X2 = (MC * (1 << 11)) / (X1 + MD);
  int32_t B5 = X1 + X2;
  float temperature = (B5 + 8) / 16.0 / 10.0;

  return temperature;
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

/* Transmit CAN Message */
void CAN_Transmit(void)
{
  // Transmitting Temperature Data
  TxHeader.StdId = 0x65D;
  TxHeader.ExtId = 0x01;
  TxHeader.RTR = CAN_RTR_DATA;
  TxHeader.IDE = CAN_ID_STD;
  TxHeader.DLC = 5;
  TxHeader.TransmitGlobalTime = DISABLE;

  // Packing the data
  TxData[0] = (uint8_t)temperature;
 // TxData[1] = (uint8_t)(temperature * 100) % 100; // Two decimal precision
  TxData[1] = motion;
  TxData[2] = heat;
  TxData[3] = gas;



  if(HAL_CAN_AddTxMessage(&hcan2, &TxHeader, TxData, &TxMailbox) != HAL_OK)
  {
    Error_Handler();
  }
}
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN2_Init(void)
{

  /* USER CODE BEGIN CAN2_Init 0 */

  /* USER CODE END CAN2_Init 0 */

  /* USER CODE BEGIN CAN2_Init 1 */

  /* USER CODE END CAN2_Init 1 */
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
  /* USER CODE BEGIN CAN2_Init 2 */

  /* USER CODE END CAN2_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0 PA1 PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PD12 PD13 PD14 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
