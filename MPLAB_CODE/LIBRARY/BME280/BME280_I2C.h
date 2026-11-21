#ifndef BME280_I2C_H
#define BME280_I2C_H

#include <stddef.h>
#include <stdint.h>

typedef struct{
    int32_t presion;
    int32_t temperatura;
    int32_t humedad;
} bme280_raw_t;

typedef struct{
    uint16_t    dig_T1;
    int16_t     dig_T2;
    int16_t     dig_T3;
    
    uint16_t    dig_P1;
    int16_t     dig_P2;
    int16_t     dig_P3;
    int16_t     dig_P4;
    int16_t     dig_P5;
    int16_t     dig_P6;
    int16_t     dig_P7;
    int16_t     dig_P8;
    int16_t     dig_P9;
    
    uint8_t     dig_H1;
    int16_t     dig_H2;
    uint8_t     dig_H3;
    int16_t     dig_H4;
    int16_t     dig_H5;
    int8_t      dig_H6;
    
} data_compensacion_t;

int BME280_COM(uint8_t reg_posicion_write, uint8_t *reg_posicion_res, size_t len);
int BME280_ESCRIBIR(uint8_t reg_ctrl, uint8_t valor);
int BME280_INIT(void);
int32_t BME280_compensate_T_int32(int32_t adc_T);
int32_t BME280_compensate_P_int64(int32_t adc_P);
int32_t BME280_compensate_H_int32(int32_t adc_H);
int BME280_ASK_DATA(bme280_raw_t *raw);


#endif