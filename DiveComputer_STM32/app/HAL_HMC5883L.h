#ifndef __HAL_HMC5883L_H
#define __HAL_HMC5883L_H

#ifdef __cplusplus
extern "C" {
#endif 

/* Includes */
#include "stm32f10x.h"

/**
 * @addtogroup  HMC5883L_I2C_Define
 * @{
 */

#define HMC5883L_I2C                  I2C1
#define HMC5883L_I2C_RCC_Periph       RCC_APB1Periph_I2C1
#define HMC5883L_I2C_Port             GPIOB
#define HMC5883L_I2C_SCL_Pin          GPIO_Pin_6
#define HMC5883L_I2C_SDA_Pin          GPIO_Pin_7
#define HMC5883L_I2C_RCC_Port         RCC_APB2Periph_GPIOB
#define HMC5883L_I2C_Speed            100000

/**
 *@}
 *//* end of group HMC5883L_I2C_Define */

#ifdef __cplusplus
}
#endif

#endif /* __HAL___HMC5883L_H */

/******************* (C) COPYRIGHT 2012 Harinadha Reddy Chintalapalli *****END OF FILE****/
