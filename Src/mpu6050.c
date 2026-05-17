/*
 * mpu6050.c  –  MPU-6050 6-axis IMU driver over I2C1 (shared with SSD1306)
 *
 * Wiring:
 *   SDA → PB7   SCL → PB6   (same bus as OLED, different I2C address)
 *   VCC → 3.3 V             GND → GND
 *   AD0 → GND  (address 0x68)
 */

#include "mpu6050.h"

/* ─────────────────────────────────────────────────────────────────────────────
 *  Low-level I2C helpers  (I2C1 assumed already initialised by ssd1306.c)
 * ───────────────────────────────────────────────────────────────────────────*/
static void MPU_I2C_Start(void)
{
    MPU6050_I2C->CR1 |= I2C_CR1_START;
    while (!(MPU6050_I2C->SR1 & I2C_SR1_SB));
}
static void MPU_I2C_Stop(void)
{
    MPU6050_I2C->CR1 |= I2C_CR1_STOP;
}
static void MPU_I2C_SendAddr(uint8_t addr, uint8_t rw)
{
    MPU6050_I2C->DR = addr | rw;
    while (!(MPU6050_I2C->SR1 & I2C_SR1_ADDR));
    (void)MPU6050_I2C->SR2;
}
static void MPU_I2C_SendByte(uint8_t d)
{
    while (!(MPU6050_I2C->SR1 & I2C_SR1_TXE));
    MPU6050_I2C->DR = d;
}
static void MPU_I2C_WaitBTF(void)
{
    while (!(MPU6050_I2C->SR1 & I2C_SR1_BTF));
}
static uint8_t MPU_I2C_ReadByte(uint8_t ack)
{
    if (ack)
        MPU6050_I2C->CR1 |=  I2C_CR1_ACK;
    else
        MPU6050_I2C->CR1 &= ~I2C_CR1_ACK;
    while (!(MPU6050_I2C->SR1 & I2C_SR1_RXNE));
    return (uint8_t)MPU6050_I2C->DR;
}

/* Write one register */
static void MPU_WriteReg(uint8_t reg, uint8_t val)
{
    MPU_I2C_Start();
    MPU_I2C_SendAddr(MPU6050_I2C_ADDR, 0);
    MPU_I2C_SendByte(reg);
    MPU_I2C_SendByte(val);
    MPU_I2C_WaitBTF();
    MPU_I2C_Stop();
}

/* Read one register */
static uint8_t MPU_ReadReg(uint8_t reg)
{
    MPU_I2C_Start();
    MPU_I2C_SendAddr(MPU6050_I2C_ADDR, 0);
    MPU_I2C_SendByte(reg);
    MPU_I2C_WaitBTF();

    MPU_I2C_Start();
    MPU_I2C_SendAddr(MPU6050_I2C_ADDR, 1);
    uint8_t val = MPU_I2C_ReadByte(0); /* NACK on last byte */
    MPU_I2C_Stop();
    return val;
}

/* Burst-read `len` bytes starting at `reg` into `buf` */
static void MPU_ReadBurst(uint8_t reg, uint8_t *buf, uint8_t len)
{
    MPU_I2C_Start();
    MPU_I2C_SendAddr(MPU6050_I2C_ADDR, 0);
    MPU_I2C_SendByte(reg);
    MPU_I2C_WaitBTF();

    MPU_I2C_Start();
    MPU_I2C_SendAddr(MPU6050_I2C_ADDR, 1);
    MPU6050_I2C->CR1 |= I2C_CR1_ACK;

    for (uint8_t i = 0; i < len; i++) {
        if (i == len - 1)
            MPU6050_I2C->CR1 &= ~I2C_CR1_ACK;  /* NACK before last byte */
        while (!(MPU6050_I2C->SR1 & I2C_SR1_RXNE));
        buf[i] = (uint8_t)MPU6050_I2C->DR;
    }
    MPU_I2C_Stop();
}

/* ─────────────────────────────────────────────────────────────────────────────
 *  Public API
 * ───────────────────────────────────────────────────────────────────────────*/
uint8_t MPU6050_Init(void)
{
    /* Wake up: clear sleep bit */
    MPU_WriteReg(MPU6050_REG_PWR_MGMT_1, 0x00);

    /* 1 kHz sample rate */
    MPU_WriteReg(MPU6050_REG_SMPLRT_DIV, 0x07);

    /* DLPF bandwidth ~44 Hz */
    MPU_WriteReg(MPU6050_REG_CONFIG, 0x03);

    /* Gyro  ±250 °/s */
    MPU_WriteReg(MPU6050_REG_GYRO_CONFIG, 0x00);

    /* Accel ±2 g */
    MPU_WriteReg(MPU6050_REG_ACCEL_CONFIG, 0x00);

    /* Verify WHO_AM_I */
    return (MPU_ReadReg(MPU6050_REG_WHO_AM_I) == 0x68) ? 0 : 1;
}

void MPU6050_Read(MPU6050_Data_t *d)
{
    uint8_t raw[14];
    MPU_ReadBurst(MPU6050_REG_ACCEL_XOUT_H, raw, 14);

    d->ax = (int16_t)((raw[0]  << 8) | raw[1]);
    d->ay = (int16_t)((raw[2]  << 8) | raw[3]);
    d->az = (int16_t)((raw[4]  << 8) | raw[5]);
    /* raw[6..7] = temperature, skip */
    d->gx = (int16_t)((raw[8]  << 8) | raw[9]);
    d->gy = (int16_t)((raw[10] << 8) | raw[11]);
    d->gz = (int16_t)((raw[12] << 8) | raw[13]);
}
