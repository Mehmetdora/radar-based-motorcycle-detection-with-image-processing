#include "main.h"

#include "arm_math.h"



#include "HELPER.h"
#include "UARTDriver.h"
#include "TimerDriver.h"

#include "IIR_DSP.h"
#include "FFT_DSP.h"
#include "RadarSignalHelper.h"
#include "ADC_DMA_Config.h"






void SystemClock_Config(void);
static void MX_GPIO_Init(void);
uint8_t check_motor_detected_count(uint8_t *detections);



volatile uint16_t analog_val = 0;

uint16_t adc_buffer[FFT_SIZE];      	// DMA buraya yazar
uint16_t adc_fft_buffer[FFT_SIZE];	// FFT buraya uygulanır
volatile uint8_t fft_ready_flag = 0;



volatile float debug_raw_sample;
volatile float debug_filtered_sample;
volatile float debug_moto_power;
volatile float debug_yaya_power;

static char radar_uart_message[RADAR_UART_MESSAGE_MAX_LEN];




int main(void)
{



  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();



  uart_init();


  adc1_init();
  dma2_init();

  TimerDriver_init(SAMPLE_RATE);	// 200


  uint8_t motor_confirmed_list[4] = {0};


  while (1)
  {



	  if(fft_ready_flag) {
		  fft_ready_flag = 0;

		  DetectionInfo result = fft_process(adc_fft_buffer);
		  RadarSignalReport report = radar_create_report_from_detection(&result);

		  if(report.motion_detected) {


			  // önce ölçülen değerler her seferinde gönderildin izlemek için
			  radar_format_uart_message(&report, radar_uart_message, sizeof(radar_uart_message));
			  uart_send_string(radar_uart_message);





			  if(report.object_class == DETECT_MOTORSIKLET){
				  motor_confirmed_list[3] = motor_confirmed_list[2];
				  motor_confirmed_list[2] = motor_confirmed_list[1];
				  motor_confirmed_list[1] = motor_confirmed_list[0];
				  motor_confirmed_list[0] = 1;
			  }else if(report.object_class == DETECT_YAYA){
				  motor_confirmed_list[3] = motor_confirmed_list[2];
				  motor_confirmed_list[2] = motor_confirmed_list[1];
				  motor_confirmed_list[1] = motor_confirmed_list[0];
				  motor_confirmed_list[0] = 0;
			  }


			  if(check_motor_detected_count(motor_confirmed_list)){
				  //uart_send_string("MOTOR TESPIT EDILDI +++++\r\n");
				  memset(motor_confirmed_list, 0 , sizeof(motor_confirmed_list));

				  radar_format_final_uart_message(&report, radar_uart_message, sizeof(radar_uart_message), MOTOR_CONFIRM_COUNT);
				  uart_send_string(radar_uart_message);

				  /*
				   * Bu noktada esp32 tarafına gönderilecek mesajda artık kameranın tetiklenmesi
				   * başlatılması söylenecek.
				   *
				   * Esp32 tarafı uart ile gönderilen bu mesajları işleyerek ilgili komut
				   * geldi mi diye kontrol etmesi gerekiyor.
				   */


			  }


		  }

	  }

  }
}


uint8_t check_motor_detected_count(uint8_t *detections){

	uint8_t counts = 0;
	for(int i = 0; i<4;i++){
		if(detections[i] == 1){
			counts++;
		}
	}

	if(counts >= MOTOR_CONFIRM_COUNT){
		return 1;
	}else{
		return 0;
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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

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
#ifdef USE_FULL_ASSERT
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
