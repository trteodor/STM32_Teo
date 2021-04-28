#include "stm32f1xx_hal.h"


#define MPU6050_ADDR 0xD0

#define SMPLRT_DIV_REG 0x68
#define GYRO_CONFIG_REG 0x1B
#define ACCEL_CONFIG_REG 0x1C
#define ACCEL_XOUT_H_REG 0x3B
#define TEMP_OUT_H_REG 0x41
#define GYRO_XOUT_H_REG 0x43
#define PWR_MGMT_1_REG 0x6B
#define WHO_AM_I_REG 0x75
extern I2C_HandleTypeDef hi2c1;

extern UART_HandleTypeDef huart2;

extern int16_t Accel_X_RAW;
extern int16_t Accel_Y_RAW;
extern int16_t Accel_Z_RAW;

extern int16_t Gyro_X_RAW;
extern int16_t Gyro_Y_RAW;
extern int16_t Gyro_Z_RAW;

extern float Ax, Ay, Az, Gx, Gy, Gz;

extern void MPU6050_Init (void);
extern void MPU6050_Read_Accel (void);
extern	void MPU6050_Read_Gyro (void);