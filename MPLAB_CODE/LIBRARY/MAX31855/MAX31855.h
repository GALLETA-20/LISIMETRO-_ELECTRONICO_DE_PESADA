/* 
 * File:   MAX31855.h
 * Author: paolo
 *
 * Created on November 20, 2026, 7:39 PM
 */

#ifndef MAX31855_H
#define	MAX31855_H

#include <stdint.h>
#include <stdbool.h>

#define MAX31855_OK                 0x00U
#define MAX31855_FAULT_OPEN         0x01U
#define MAX31855_FAULT_SHORT_GND    0x02U
#define MAX31855_FAULT_SHORT_VCC    0x04U

typedef struct{
    int32_t tc_x100;
    int32_t ij_x100;
    uint8_t fault;
} max31855_data_t;

void        MAX31855_Init(void);
uint32_t    MAX31855_ReadRaw(void);
bool        MAX31855_Read(max31855_data_t *out);


#endif	/* MAX31855_H */

