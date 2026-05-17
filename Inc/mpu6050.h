#ifndef MPU6050_H
#define MPU6050_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ── I2C address: AD0 pin low → 0x68, high → 0x69 ── */
#define MPU6050_I2C_ADDR   (0x68 << 1)

/* ── Shares I2C1 with the OLED ── */
#define MPU6050_I2C        I2C1

/* ── Register map (minimal subset) ── */
#define MPU6050_REG_SMPLRT_DIV   0x19
#define MPU6050_REG_CONFIG       0x1A
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_WHO_AM_I     0x75

typedef struct {
    int16_t ax, ay, az;   /* raw accel (±2 g range → 16384 LSB/g) */
    int16_t gx, gy, gz;   /* raw gyro  (±250 °/s  →    131 LSB/°/s) */
} MPU6050_Data_t;

/* ── Public API ── */
uint8_t MPU6050_Init(void);              /* returns 0 on success */
void    MPU6050_Read(MPU6050_Data_t *d);

#endif /* MPU6050_H */
