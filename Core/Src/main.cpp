/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.cpp
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 u-blox
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  * http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "string.h"
#include "cmsis_os.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

#ifndef NO_UBX_LIB_PRESENT
// Bring in all of the ubxlib public header files
#include "ubxlib.h"
// Bring in the application settings
#include "u_cfg_app_platform_specific.h"
#endif //NO_UBX_LIB_PRESENT

#ifndef HAL_DRIVERS_ONLY
#include "model_tflite_micro.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"


#ifndef NO_UBX_LIB_PRESENT
// Echo server URL and port number
// HTTPS server URL: this is a test server that accepts PUT/POST
// requests and GET/HEAD/DELETE requests on port 8081; there is
// also an HTTP server on port 8080.
#define MY_SERVER_NAME "ubxlib.redirectme.net:8081"

// Some data to PUT and GET with the server.
const char *gpMyData = "Hello world!";
#define MY_SERVER_PORT 5055

// Cellular configuration.
// Set U_CFG_TEST_CELL_MODULE_TYPE to your module type,
// chosen from the values in cell/api/u_cell_module_type.h
//
// Note that the pin numbers are those of the MCU: if you
// are using an MCU inside a u-blox module the IO pin numbering
// for the module is likely different that from the MCU: check
// the data sheet for the module to determine the mapping.

// DEVICE i.e. module/chip configuration: in this case a cellular
// module connected via UART
static const uDeviceCfg_t gDeviceCfg = {
		0, U_DEVICE_TYPE_CELL,
		{
		  {
		    0,
			U_CFG_TEST_CELL_MODULE_TYPE,//U_CFG_TEST_CELL_MODULE_TYPE,
		    //"1234", /* SIM pin */
		    NULL, /* SIM pin */
		    U_CFG_APP_PIN_CELL_ENABLE_POWER,
		    U_CFG_APP_PIN_CELL_PWR_ON,
		    U_CFG_APP_PIN_CELL_VINT,
		    U_CFG_APP_PIN_CELL_DTR
		  },
		},
		U_DEVICE_TRANSPORT_TYPE_UART,
    		{
		  {
		  0,
		  U_CFG_APP_CELL_UART,
		  U_CELL_UART_BAUD_RATE,
		  U_CFG_APP_PIN_CELL_TXD,
		  U_CFG_APP_PIN_CELL_RXD,
		  U_CFG_APP_PIN_CELL_CTS,
		  U_CFG_APP_PIN_CELL_RTS
		  },
	    },
    	};
// NETWORK configuration for cellular
static const uNetworkCfgCell_t gNetworkCfg = {
    	0,
	U_NETWORK_TYPE_CELL,
    	NULL, /* APN: NULL to accept default.  If using a Thingstream SIM enter "tsiot" here */
    	240 /* Connection timeout in seconds */
    // There is an additional field here "pKeepGoingCallback",
    // which we do NOT set, we allow the compiler to set it to 0
    // and all will be fine. You may set the field to a function
    // of the form "bool keepGoingCallback(uDeviceHandle_t devHandle)",
    // e.g.:
    // .pKeepGoingCallback = keepGoingCallback
    // ...and your function will be called periodically during an
    // abortable network operation such as connect/disconnect;
    // if it returns true the operation will continue else it
    // will be aborted, allowing you immediate control.  If this
    // field is set, timeoutSeconds will be ignored.
};

static const uNetworkType_t gNetType = U_NETWORK_TYPE_CELL;
#endif //NO_UBX_LIB_PRESENT

// Globals, used for compatibility with Arduino-style sketches.
// Globals, used for compatibility with Arduino-style sketches.
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
int inference_count = 0;

constexpr int kTensorArenaSize = 2000;
uint8_t tensor_arena[kTensorArenaSize];
}  // namespace

/* Private includes ----------------------------------------------------------*/
const int c_stack_size_TinyML = 500;
int const array_size = 16;


// This constant represents the range of x values our model was trained on,
// which is from 0 to (2 * Pi). We approximate Pi to avoid requiring
// additional libraries.
const int kInferencesPerCycle = 1000;

// This constant determines the number of inferences to perform across the range
// of x values defined above. Since each inference takes time, the higher this
// number, the more time it will take to run through the entire range. The value
// of this constant can be tuned so that one full cycle takes a desired amount
// of time. Since different devices take different amounts of time to perform
// inference, this value should be defined per-device.
// A larger number than the default to make the animation smoother
const float kXrange = 2.f * 3.14159265359f;

// Expose a C friendly interface for main functions.
// Initializes all data needed for the example. The name is important, and needs
// to be setup() for Arduino compatibility.
void setup();

// Runs one iteration of data gathering and inference. This should be called
// repeatedly from the application code. The name needs to be loop() for Arduino
// compatibility.
void loop();

void handle_output(tflite::ErrorReporter* error_reporter, float x_value, float y_value);
#endif //# HAL_DRIVERS_ONLY

/* Private variables ---------------------------------------------------------*/


osThreadId Task1Handle;
osThreadId Task2Handle;
osSemaphoreId myBinarySem01Handle;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

void StartDefaultTask(void const * argument);
void StartTask02(void const * argument);

#ifndef HAL_DRIVERS_ONLY
void HandleOutput(tflite::ErrorReporter* error_reporter, float x_value, float y_value)
{
	// Log the current X and Y values
	TF_LITE_REPORT_ERROR(error_reporter, "x_value: %f, y_value: %f\n", x_value, y_value);
}
#endif // HAL_DRIVERS_ONLY

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  const int length = 27;
  char current_char = 'A';
  char my_buffer[length];
  setup();
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_DEVICE_Init();

  my_buffer[length-1] = '\n';
  
  for(int index=0; index < length-1; index++)
  {
    my_buffer[index] = current_char++;

    CDC_Transmit_FS((uint8_t *)my_buffer, length);
  }
  /* USER CODE BEGIN 2 */

  /* Create the semaphores(s) */
  myBinarySem01Handle = xSemaphoreCreateBinary();

  /* Create the thread(s) */
  /* definition and creation of Task1 */
  osThreadDef(Task1, StartDefaultTask, osPriorityNormal, 0, c_stack_size_TinyML);
  Task1Handle = osThreadCreate(osThread(Task1), NULL);

  /* definition and creation of Task2 */
  osThreadDef(Task2, StartTask02, osPriorityNormal, 0, c_stack_size_TinyML);
  Task2Handle = osThreadCreate(osThread(Task2), NULL);


  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  while (1)
  {
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 6;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
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

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the Task1 thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  unsigned char ch='-';
  UBaseType_t uxHighWaterMark;
#ifndef NO_UBX_LIB_PRESENT
  uPortGpioConfig_t gpioConfig = U_PORT_GPIO_CONFIG_DEFAULT;
  uHttpClientContext_t *pContext = NULL;
  uHttpClientConnection_t connection = U_HTTP_CLIENT_CONNECTION_DEFAULT;
  uSecurityTlsSettings_t tlsSettings = U_SECURITY_TLS_SETTINGS_DEFAULT;
  char serialNumber[U_SECURITY_SERIAL_NUMBER_MAX_LENGTH_BYTES];
  char path[U_SECURITY_SERIAL_NUMBER_MAX_LENGTH_BYTES + 10];
  char buffer[32] = {0};
  size_t size = sizeof(buffer);
  int32_t statusCode = 0;

  uDeviceHandle_t devHandle = NULL;
  int32_t returnCode;

  uPortLog("StartDefaultTask called\n");

  uPortInit();

  uPortLog("uPortInit successfull\n");
#if !defined (STM32F405xx)

  // Enable power to 3V3 rail for the C030 board
  gpioConfig.pin = U_CFG_APP_PIN_C030_ENABLE_3V3;
  gpioConfig.driveMode = U_PORT_GPIO_DRIVE_MODE_OPEN_DRAIN;
  gpioConfig.direction = U_PORT_GPIO_DIRECTION_OUTPUT;
  uPortGpioConfig(&gpioConfig);
  uPortGpioSet(U_CFG_APP_PIN_C030_ENABLE_3V3, 1);


  // Make sure the PWR_ON pin is initially high
  // BEFORE taking the module out of reset: this
  // ensures that it powers on from reset which
  // permits FW update on SARA-R5
  uPortGpioSet(U_CFG_APP_PIN_CELL_PWR_ON, 1);
  gpioConfig.direction = U_PORT_GPIO_DIRECTION_OUTPUT;
  gpioConfig.driveMode = U_PORT_GPIO_DRIVE_MODE_NORMAL;
  gpioConfig.pin = U_CFG_APP_PIN_CELL_PWR_ON;
  uPortGpioConfig(&gpioConfig);
  uPortGpioSet(U_CFG_APP_PIN_CELL_PWR_ON, 1);

  // Set reset high (i.e. not reset) if it is connected
  gpioConfig.pin = U_CFG_APP_PIN_CELL_RESET;
  gpioConfig.driveMode = U_PORT_GPIO_DRIVE_MODE_NORMAL;
  gpioConfig.direction = U_PORT_GPIO_DIRECTION_OUTPUT;
  uPortGpioConfig(&gpioConfig);
  uPortGpioSet(U_CFG_APP_PIN_CELL_RESET, 1);

#endif //STM32F405xx

  osDelay(100);

  uDeviceInit();
  uPortLog("uDeviceInit successfull\n");

  returnCode = uDeviceOpen(&gDeviceCfg, &devHandle);
  uPortLog("Opened device with return code %d.\n", returnCode);

#endif //NO_UBX_LIB_PRESENT
  /* Inspect our own high water mark on entering the task. */
  uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
  printf("StartDefaultTask:: MAX_Stack_SZ:%d, uxHighWaterMark:%ld\n", c_stack_size_TinyML, uxHighWaterMark);
#ifndef NO_UBX_LIB_PRESENT
  uCellPwrIsAlive(devHandle);
#endif //NO_UBX_LIB_PRESENT

  // Bring up the network interface
  uPortLog("Bringing up the network...\n");
  if (uNetworkInterfaceUp(devHandle, gNetType,
						  &gNetworkCfg) == 0) {


      // Note: since devHandle is a cellular
      // handle, any of the `cell` API calls could
      // be made here using it.

      // In order to create a unique path on the public
      // server which won't collide with anyone else,
      // we make the path the serial number of the
      // module
      uSecurityGetSerialNumber(devHandle, serialNumber);
      snprintf(path, sizeof(path), "/%s.html", serialNumber);

      // Set the URL of the server; each instance
      // is associated with a single server -
      // you may create more than one insance,
      // for different servers, or close and
      // open instances to access more than one
      // server.  There are other settings in
      // uHttpClientConnection_t, but for the
      // purposes of this example they can be
      // left at defaults
      connection.pServerName = MY_SERVER_NAME;

      // Create an HTTPS instance for the server; to
      // create an HTTP instance instead you would
      // replace &tlsSettings with NULL (and of
      // course use port 8080 on the test HTTP server).
      pContext = pUHttpClientOpen(devHandle, &connection, &tlsSettings);
      if (pContext != NULL) {

          // POST some data to the server; doesn't have to be text,
          // can be anything, including binary data, though obviously
          // you must give the appropriate content-type
          statusCode = uHttpClientPostRequest(pContext, path, gpMyData, strlen(gpMyData),
                                              "text/plain", NULL, NULL, NULL);
          if (statusCode == 200) {

              uPortLog("POST some data to the file \"%s\" on %s.\n", path, MY_SERVER_NAME);

              // GET it back again
              statusCode = uHttpClientGetRequest(pContext, path, buffer, &size, NULL);
              if (statusCode == 200) {

                  uPortLog("GET the data: it was \"%s\" (%d byte(s)).\n", buffer, size);

              } else {
                  uPortLog("Unable to GET file \"%s\" from %s; status code was %d!\n",
                           path, MY_SERVER_NAME, statusCode);
              }
          } else {
              uPortLog("Unable to POST file \"%s\" to %s; status code was %d!\n",
                       path, MY_SERVER_NAME, statusCode);
          }

          // Close the HTTP instance again
          uHttpClientClose(pContext);

      } else {
          uPortLog("Unable to create HTTP instance!\n");
      }

      // When finished with the network layer
      uPortLog("Taking down network...\n");
      uNetworkInterfaceDown(devHandle, gNetType);
  } else {
      uPortLog("Unable to bring up the network!\n");
  }

  // Close the device
  // Note: we don't power the device down here in order
  // to speed up testing; you may prefer to power it off
  // by setting the second parameter to true.
  uDeviceClose(devHandle, false);

  // Tidy up
  uDeviceDeinit();
  uPortDeinit();

  uPortLog("Done.\n");

  xSemaphoreGive( myBinarySem01Handle);

  /* Infinite loop */
  while (true)
  {
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_15);
	HAL_Delay(100);
  }
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the Task2 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void const * argument)
{
  unsigned char ch='>';
  UBaseType_t uxHighWaterMark;

  xSemaphoreTake( myBinarySem01Handle, 0xFFFF );
  /* USER CODE BEGIN StartTask02 */
  /* Infinite loop */
  for(;;)
  {
	loop();
	/* Inspect our own high water mark on entering the task. */
	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
	printf("MAX_Stack_SZ:%ld, uxHighWaterMark:%ld\n",c_stack_size_TinyML, uxHighWaterMark);

	osDelay(1000);
  }
  /* USER CODE END StartTask02 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
}

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

}
#endif /* USE_FULL_ASSERT */


#ifndef HAL_DRIVERS_ONLY
// The name of this function is important for Arduino compatibility.
void setup() {
  // Set up logging. Google style is to avoid globals or statics because of
  // lifetime uncertainty, but since this has a trivial destructor it's okay.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  model = tflite::GetModel(model_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  // This pulls in all the operation implementations we need.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::AllOpsResolver resolver;

  // Build an interpreter to run the model with.
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }

  // Obtain pointers to the model's input and output tensors.
  input = interpreter->input(0);
  output = interpreter->output(0);

  // Keep track of how many inferences we have performed.
  inference_count = 0;
}

// The name of this function is important for Arduino compatibility.
void loop() {
  // Calculate an x value to feed into the model. We compare the current
  // inference_count to the number of inferences per cycle to determine
  // our position within the range of possible x values the model was
  // trained on, and use this to calculate a value.
  float position = static_cast<float>(inference_count) /
                   static_cast<float>(kInferencesPerCycle);
  float x = position * kXrange;

  // Quantize the input from floating-point to integer
  int8_t x_quantized = x / input->params.scale + input->params.zero_point;
  // Place the quantized input in the model's input tensor
  input->data.int8[0] = x_quantized;

  // Run inference, and report any error
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed on x: %f\n",
                         static_cast<double>(x));
    return;
  }

  // Obtain the quantized output from model's output tensor
  int8_t y_quantized = output->data.int8[0];
  // Dequantize the output from integer to floating-point
  float y = (y_quantized - output->params.zero_point) * output->params.scale;

  // Output the results. A custom HandleOutput function can be implemented
  // for each supported hardware target.
  HandleOutput(error_reporter, x, y);

  // Increment the inference_counter, and reset it if we have reached
  // the total number per cycle
  inference_count += 1;
  if (inference_count >= kInferencesPerCycle) inference_count = 0;
}
#endif // HAL_DRIVERS_ONLY
