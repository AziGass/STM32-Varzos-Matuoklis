/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Pagrindinis programos kūnas
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>
#include "oled.h"

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define VREF 3.3f
#define ADC_MAX 4095.0f
#define RREF1 10540.0f  // Pirmo kanalo referencinio rezistoriaus varža
#define RREF2 10430.0f  // Antro kanalo referencinio rezistoriaus varža
#define SAMPLES 20      // Matavimų skaičius vidurkiui
/* USER CODE END PD */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);

/* USER CODE BEGIN PFP */
/* Lietuviški funkcijų prototipai */
float Nuskaityti_ADC(uint32_t kanalas);
float Nuskaityti_ADC_Vidurki(uint32_t kanalas);
float Skaiciuoti_Varza(float adc_verte, float rref);
void Siusti_UART(float r1, float r2);
/* USER CODE END PFP */

/**
  * @brief  Programos pradžios taškas.
  */
int main(void)
{
  /* Periferijos inicializacija */
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_ADC_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  // ADC kalibravimas ir paleidimas
  HAL_ADCEx_Calibration_Start(&hadc, ADC_SINGLE_ENDED);
  HAL_ADC_Start(&hadc);

  HAL_Delay(100); // Leidžiame OLED ekranui pilnai įsijungti
  OLED_Init();

  // Išvalome visą ekraną
  for(int i = 0; i < 8; i++) {
      OLED_ClearPage(i);
  }
  /* USER CODE END 2 */

  /* Begalinis ciklas */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // 1. Nuskaitome ADC vidurkius iš abiejų kanalų
      float adc1 = Nuskaityti_ADC_Vidurki(ADC_CHANNEL_0);
      float adc2 = Nuskaityti_ADC_Vidurki(ADC_CHANNEL_1);

      // 2. Skaičiuojame varžas
      float r1 = Skaiciuoti_Varza(adc1, RREF1);
      float r2 = Skaiciuoti_Varza(adc2, RREF2);

      // 3. R1 OLED atvaizdavimas
      if (r1 < 1.0f) {
          OLED_PrintRes(0, 1, 0);         // Rodo R1=0 (trumpas sujungimas)
      } else if (r1 > 100000.0f) {
          OLED_PrintOL(0, 1);             // Rodo R1= OL (viršyta riba)
      } else {
          OLED_PrintRes(0, 1, (int)r1);   // Rodo normalią varžą
      }

      // 4. R2 OLED atvaizdavimas
      if (r2 < 1.0f) {
          OLED_PrintRes(2, 2, 0);
      } else if (r2 > 100000.0f) {
          OLED_PrintOL(2, 2);
      } else {
          OLED_PrintRes(2, 2, (int)r2);
      }

      // 5. UART Siuntimas į kompiuterį
      Siusti_UART(r1, r2);

      // Išlaikome 1s atnaujinimo periodą
      HAL_Delay(900);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/* USER CODE BEGIN 4 */

/**
  * @brief  Vienas ADC nuskaitymas iš nurodyto kanalo
  */
float Nuskaityti_ADC(uint32_t kanalas)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    HAL_ADC_Stop(&hadc);

    /* Išvalome visų anksčiau pasirinktų kanalų registrą (STM32L0 specifika) */
    hadc.Instance->CHSELR = 0;

    sConfig.Channel = kanalas;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    HAL_ADC_ConfigChannel(&hadc, &sConfig);

    HAL_ADC_Start(&hadc);
    HAL_ADC_PollForConversion(&hadc, HAL_MAX_DELAY);

    uint32_t adc_reiksme = HAL_ADC_GetValue(&hadc);

    return (float)adc_reiksme;
}

/**
  * @brief  ADC matavimų vidurkio skaičiavimas triukšmams sumažinti
  */
float Nuskaityti_ADC_Vidurki(uint32_t kanalas)
{
    float suma = 0;

    for(int i = 0; i < SAMPLES; i++)
    {
        suma += Nuskaityti_ADC(kanalas);
        HAL_Delay(2);
    }

    return suma / SAMPLES;
}

/**
  * @brief  Varžos apskaičiavimas
  */
float Skaiciuoti_Varza(float adc_verte, float rref)
{
    // 1. NULIO KALIBRAVIMAS (Offset) ADC lygyje
    float adc_offset = 0.0f; 
    float koreguotas_adc = adc_verte - adc_offset;
    
    if (koreguotas_adc < 0) koreguotas_adc = 0;

    // 2. STIPRINIMO KALIBRAVIMAS (Gain) ADC lygyje
    float adc_gain = 1.0f;
    koreguotas_adc = koreguotas_adc * adc_gain; 

    // 3. Apsauga nuo begalybės (atvira grandinė)
    if(koreguotas_adc >= ADC_MAX - 1) return 1000000.0f;

    // 4. Skaičiuojame varžą TIK su sutvarkytu ADC skaičiumi
    float galutine_varza = rref * (koreguotas_adc / (ADC_MAX - koreguotas_adc));

    return galutine_varza;
}

/**
  * @brief  Duomenų formatavimas ir siuntimas per UART
  */
void Siusti_UART(float r1, float r2)
{
    char msg[128];
    char r1_str[32];
    char r2_str[32];

    // Tikriname R1 ribas
    if (r1 < 1.0f) {
        strcpy(r1_str, "< 1 ohm");
    } else if (r1 > 100000.0f) {
        strcpy(r1_str, "> 100k ohm");
    } else {
        sprintf(r1_str, "%.0f ohm", r1);
    }

    // Tikriname R2 ribas
    if (r2 < 1.0f) {
        strcpy(r2_str, "< 1 ohm");
    } else if (r2 > 100000.0f) {
        strcpy(r2_str, "> 100k ohm");
    } else {
        sprintf(r2_str, "%.0f ohm", r2);
    }

    // Suformuojame galutinę žinutę ir išsiunčiame
    sprintf(msg, "R1=%s, R2=%s\r\n", r1_str, r2_str);
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
}
/* USER CODE END 4 */

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_5;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialization Function
  */
static void MX_ADC_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc.Instance = ADC1;
  hadc.Init.OversamplingMode = DISABLE;
  hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.SamplingTime = ADC_SAMPLETIME_160CYCLES_5;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerFrequencyMode = ENABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_1;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  */
static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00000608;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
}

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */