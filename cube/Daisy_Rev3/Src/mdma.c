/**
  ******************************************************************************
  * File Name          : mdma.c
  * Description        : This file provides code for the configuration
  *                      of all the requested global MDMA transfers.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "mdma.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure MDMA                                                              */
/*----------------------------------------------------------------------------*/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
MDMA_HandleTypeDef hmdma_mdma_channel40_sdmmc1_end_data_0;

/** 
  * Enable MDMA controller clock
  * Configure MDMA for global transfers
  *   hmdma_mdma_channel40_sdmmc1_end_data_0
  */
void MX_MDMA_Init(void) 
{
  /* MDMA controller clock enable */
  __HAL_RCC_MDMA_CLK_ENABLE();
  /* Local variables */

  /* Configure MDMA channel MDMA_Channel0 */
  /* Configure MDMA request hmdma_mdma_channel40_sdmmc1_end_data_0 on MDMA_Channel0 */
  hmdma_mdma_channel40_sdmmc1_end_data_0.Instance = MDMA_Channel0;
  hmdma_mdma_channel40_sdmmc1_end_data_0.Init.Request = MDMA_REQUEST_SDMMC1_END_DATA;
  hmdma_mdma_channel40_sdmmc1_end_data_0.Init.TransferTriggerMode = MDMA_BUFFER_TRANSFER;
  hmdma_mdma_channel40_sdmmc1_end_data_0.Init.Priority = MDMA_PRIORITY_LOW;
  hmdma_mdma_channel40_sdmmc1_end_data_0.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
  hmdma_mdma_channel40_sdmmc1_end_data_0.Init.SourceInc = MDMA_SRC_INC_BYTE;
  hmdma_mdma_channel40_sdmmc1_end_data_0.Init.DestinationInc = MDMA_DEST_INC_BYTE;
  hmdma_mdma_channel40_sdmmc1_end_data_0.Init.SourceDataSize = MDMA_SRC_DATASIZE_BYTE;
  hmdma_mdma_channel40_sdmmc1_end_data_0.Init.DestDataSize = MDMA_DEST_DATASIZE_BYTE;
  hmdma_mdma_channel40_sdmmc1_end_data_0.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
  hmdma_mdma_channel40_sdmmc1_end_data_0.Init.BufferTransferLength = 1;
  hmdma_mdma_channel40_sdmmc1_end_data_0.Init.SourceBurst = MDMA_SOURCE_BURST_SINGLE;
  hmdma_mdma_channel40_sdmmc1_end_data_0.Init.DestBurst = MDMA_DEST_BURST_SINGLE;
  hmdma_mdma_channel40_sdmmc1_end_data_0.Init.SourceBlockAddressOffset = 0;
  hmdma_mdma_channel40_sdmmc1_end_data_0.Init.DestBlockAddressOffset = 0;
  if (HAL_MDMA_Init(&hmdma_mdma_channel40_sdmmc1_end_data_0) != HAL_OK)
  {
    Error_Handler();
  }

}
/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
