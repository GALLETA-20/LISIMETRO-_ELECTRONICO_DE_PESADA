#include "MAX31855.h"
#include "mcc_generated_files/system/system.h"

void MAX31855_Init(void){
    CS_SetHigh();
}

uint32_t MAX31855_ReadRaw(void){
    uint32_t data = 0;
    SPI1_Open(HOST_CONFIG);
    CS_SetLow();
    
    for(uint8_t i=0; i<4; i++){
        data = (data<<8) | SPI1_ByteExchange(0x00);
    }
    
    CS_SetHigh();
    SPI1_Close();
    
    return data;
}

bool MAX31855_Read(max31855_data_t *out){
    uint32_t data = MAX31855_ReadRaw();
    
    if(data & 0x00010000UL){
        out->fault      = (uint8_t)(data & 0x07UL);
        out->tc_x100    = 0;
        out->ij_x100    = 0;
        return false;
    }
    
    int16_t tc = (int16_t)(data>>16) >> 2;
    int16_t ij = (int16_t)(data & 0xFFF0UL) >> 4;
    
    out->tc_x100 = (int32_t)tc * 25;
    out->ij_x100 = ((int32_t)ij * 625) / 100;
    out->fault   = MAX31855_OK;
    
    return true;
    
}