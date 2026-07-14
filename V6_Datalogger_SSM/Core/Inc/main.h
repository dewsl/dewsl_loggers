/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx_hal.h"

void Error_Handler(void);

/* USER CODE BEGIN EFP */
extern CAN_HandleTypeDef hcan1;
extern SPI_HandleTypeDef hspi2;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

void SSM_Printf(const char *fmt, ...);
/* USER CODE END EFP */

#define SPI2_SDCS_Pin GPIO_PIN_12
#define SPI2_SDCS_GPIO_Port GPIOB

#define CAN1_STB_Pin GPIO_PIN_9
#define CAN1_STB_GPIO_Port GPIOA

#define CAN1_SHDN_Pin GPIO_PIN_10
#define CAN1_SHDN_GPIO_Port GPIOA

#ifdef __cplusplus
}
#endif

#endif
