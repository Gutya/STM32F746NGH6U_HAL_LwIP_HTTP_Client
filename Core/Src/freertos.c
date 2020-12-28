/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "stdio.h"
#include "lwip.h"
#include "api.h"
#include "string.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define TIMEOUT		3000	//	in ms
#define BUF_LENGTH	2048

//#define	DOMAIN_NAME	"fly.sytes.net"
//#define PORT		8080

#define DOMAIN_NAME	"google.com"
#define PORT		80

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

char recv_buf[BUF_LENGTH] = {0};
extern TIM_HandleTypeDef htim12;

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId pwmTaskHandle;
osSemaphoreId BinarySemHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartPwmTask(void const * argument);

extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of BinarySem */
  osSemaphoreDef(BinarySem);
  BinarySemHandle = osSemaphoreCreate(osSemaphore(BinarySem), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 256);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of pwmTask */
  osThreadDef(pwmTask, StartPwmTask, osPriorityAboveNormal, 0, 256);
  pwmTaskHandle = osThreadCreate(osThread(pwmTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */

extern struct netif gnetif;
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */

  char * write_buf = pvPortMalloc(64);

  struct netconn * nc;
  struct netbuf * nb;
  volatile err_t res;
  ip_addr_t local_ip;
  ip_addr_t remote_ip;

  while(gnetif.ip_addr.addr == 0) osDelay(1);
  local_ip = gnetif.ip_addr;
  printf("Client IP: %s\r\n", ip4addr_ntoa(&gnetif.ip_addr));

  netconn_gethostbyname(DOMAIN_NAME, &remote_ip);
  printf("Server IP: %s\r\n\r\n", ip4addr_ntoa(&remote_ip));

  sprintf(write_buf, "GET / HTTP/1.1\r\nHost: %s\r\n\r\n", DOMAIN_NAME);

  /* Infinite loop */
  for(;;){

	  nc = netconn_new(NETCONN_TCP);
	  if(nc != NULL){

		  netconn_set_recvtimeout(nc, TIMEOUT);
		  res = netconn_bind(nc, &local_ip, 0);
		  if(res == ERR_OK){

			  res = netconn_connect(nc, &remote_ip, PORT);
			  if(res == ERR_OK){

				  for(;;){

					  res = netconn_write(nc, write_buf, strlen(write_buf), NETCONN_COPY);
					  printf("send:\r\n%s", write_buf);
					  if(res != ERR_OK){

						  printf("\r\n\r\nnetconn_write error: %d\r\n\r\n", res);
						  break;
					  }
					  res = netconn_recv(nc, &nb);
					  if(res == ERR_OK){

						  uint16_t len = netbuf_len(nb);
						  netbuf_copy(nb, recv_buf, len);
						  netbuf_delete(nb);
						  recv_buf[len] = 0;
						  printf("recv:\r\n%s", recv_buf);

					  } else {

						  printf("\r\n\r\nnetconn_recv error: %d\r\n\r\n", res);
						  break;
					  }
					  xSemaphoreGive(BinarySemHandle);
					  osDelay(TIMEOUT);
				  }
			  } else printf("netconn_connect error: %d\r\n", res);
		  }	else printf("netconn_bind error: %d\r\n", res);
		  netconn_close(nc);
		  netconn_delete(nc);
		  printf("connection closed and deleted\r\n");
	  } else printf("netconn_new error\r\n");
	  osDelay(TIMEOUT);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartPwmTask */
/**
* @brief Function implementing the pwmTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartPwmTask */
void StartPwmTask(void const * argument)
{
  /* USER CODE BEGIN StartPwmTask */
	xSemaphoreTake(BinarySemHandle, (TickType_t) 10);
  /* Infinite loop */
  for(;;){

	  if(xSemaphoreTake(BinarySemHandle, (TickType_t) 10) == pdTRUE){

		  char * ptr = strchr(recv_buf, '$');
		  if(ptr != NULL){

			  uint16_t brightness = 0;
			  sscanf(ptr + 1, "%hu", &brightness);
			  TIM12->CCR1 = brightness;	//	0 - min 100 - max
			  printf("brightness = %d\r\n", brightness);
		  	  memset(recv_buf, 0, BUF_LENGTH);
		  } else printf("\r\n$ not found\r\n");
	  }
  }
  /* USER CODE END StartPwmTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
