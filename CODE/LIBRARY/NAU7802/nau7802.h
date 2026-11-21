#ifndef NAU7802_H
#define NAU7802_H

#include <stdint.h>
#include <stdbool.h>

#define NAU7802_I2C_ADDR    0x2A

// Registros principales
#define NAU7802_PU_CTRL     0x00
#define NAU7802_CTRL1       0x01
#define NAU7802_CTRL2       0x02
#define NAU7802_ADCO_B2     0x12
#define NAU7802_ADC         0x15
#define NAU7802_PWR_CTRL    0x1C

// Ganancias disponibles
typedef enum {
    NAU7802_GAIN_1   = 0,
    NAU7802_GAIN_2   = 1,
    NAU7802_GAIN_4   = 2,
    NAU7802_GAIN_8   = 3,
    NAU7802_GAIN_16  = 4,
    NAU7802_GAIN_32  = 5,
    NAU7802_GAIN_64  = 6,
    NAU7802_GAIN_128 = 7
} nau7802_gain_t;

// Velocidades de muestreo (SPS)
typedef enum {
    NAU7802_SPS_10  = 0,
    NAU7802_SPS_20  = 1,
    NAU7802_SPS_40  = 2,
    NAU7802_SPS_80  = 3,
    NAU7802_SPS_320 = 7
} nau7802_sps_t;

bool NAU7802_Init(void);
bool NAU7802_Calibrate(void);
bool NAU7802_DataReady(void);
bool NAU7802_ReadRaw(int32_t *out);
bool NAU7802_ReadAverage(uint8_t samples, int32_t *out);

#endif // NAU7802_H