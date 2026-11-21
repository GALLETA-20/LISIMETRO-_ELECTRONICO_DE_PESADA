#include "BME280_I2C.h"
#include "mcc_generated_files/i2c_host/i2c1.h"
#include <stdint.h>
#include <stdio.h>

#define ADDRS       0x76
#define ID_VAL      0x60

//DIRECCIONES DE REGISTRO
#define REG_ID      0xD0
#define REG_CONF    0xF5
#define PRESS_MSB   0xF7 //ES EL PRIMER BYTE QUE HACE QUE EL SENSOR RESPONDA CON UNA RÁFAGA DE DATOS.

#define REG_CTRL_HUM    0xF2
#define VAL_CTRL_HUM    0x01

#define REG_CTRL_MEAS   0xF4
#define VAL_CTRL_MEAS   0x27

#define REG_CALIB_TEMP_PRES     0x88
#define REG_CALIB_HUME_1        0xA1
#define REG_CALIB_HUME_2        0xE1

static uint8_t id = 0;
static uint8_t RESPUESTA_BME280[8] = {0};
static data_compensacion_t calib = {0};
static int32_t t_fine = 0;


static int STATUS_I2C(void){
    while(I2C1_Host.IsBusy()){
        I2C1_Host.Tasks();
    }

    if(I2C1_Host.ErrorGet() == I2C_ERROR_NONE){
        return 0;
    } else {
        return -1;
    }
}

int BME280_COM(uint8_t reg_posicion_write, uint8_t *reg_posicion_res, size_t len){
    
    if(I2C1_Host.WriteRead(ADDRS, &reg_posicion_write, 1, reg_posicion_res, len)){    
        
        return STATUS_I2C();
        
    } else {
        printf("E_COM: ERROR AL ENVIAR DATOS.\n");
        return -1;
    }
}

int BME280_ESCRIBIR(uint8_t reg_ctrl, uint8_t valor){
    uint8_t paquete[2] = {reg_ctrl, valor};
    
    if(I2C1_Host.Write(ADDRS, paquete, 2)){

        return STATUS_I2C();

    } else {
        printf("E_WRITE: ERROR AL ESCRIBIR DATOS.\n");
        return -1;
    }
}

static int leer_calibracion(void){
    uint8_t buf_datos_calib_tmp_pres[24] = {0};
    uint8_t buf_datos_calib_hume1 = 0;
    uint8_t buf_datos_calib_hume2[7] = {0};
    
    
    if(BME280_COM(REG_CALIB_TEMP_PRES, buf_datos_calib_tmp_pres, 24) == 0){
       
        calib.dig_T1 =  ((uint16_t)buf_datos_calib_tmp_pres[1] << 8) |
                        ((uint16_t)buf_datos_calib_tmp_pres[0]);
        
        calib.dig_T2 = (int16_t) ( ((uint16_t)buf_datos_calib_tmp_pres[3] << 8) |
                                 (  (uint16_t)buf_datos_calib_tmp_pres[2]) );
        
        calib.dig_T3 = (int16_t) ( ((uint16_t)buf_datos_calib_tmp_pres[5] << 8) |
                                 (  (uint16_t)buf_datos_calib_tmp_pres[4]) );
        
        calib.dig_P1 = (uint16_t)(buf_datos_calib_tmp_pres[7]) << 8 |
                       (uint16_t)(buf_datos_calib_tmp_pres[6]);
        
        calib.dig_P2 = (int16_t) (( (uint16_t)(buf_datos_calib_tmp_pres[9]) << 8) |
                                    (uint16_t)(buf_datos_calib_tmp_pres[8]) );
        
        calib.dig_P3 = (int16_t) (( (uint16_t)(buf_datos_calib_tmp_pres[11]) << 8) |
                                    (uint16_t)(buf_datos_calib_tmp_pres[10]) );
        
        calib.dig_P4 = (int16_t) (( (uint16_t)(buf_datos_calib_tmp_pres[13]) << 8) |
                                    (uint16_t)(buf_datos_calib_tmp_pres[12]) );
        
        calib.dig_P5 = (int16_t) (( (uint16_t)(buf_datos_calib_tmp_pres[15]) << 8) |
                                    (uint16_t)(buf_datos_calib_tmp_pres[14]) );
        
        calib.dig_P6 = (int16_t) (( (uint16_t)(buf_datos_calib_tmp_pres[17]) << 8) |
                                    (uint16_t)(buf_datos_calib_tmp_pres[16]) );
        
        calib.dig_P7 = (int16_t) (( (uint16_t)(buf_datos_calib_tmp_pres[19]) << 8) |
                                    (uint16_t)(buf_datos_calib_tmp_pres[18]) );
        
        calib.dig_P8 = (int16_t) (( (uint16_t)(buf_datos_calib_tmp_pres[21]) << 8) |
                                    (uint16_t)(buf_datos_calib_tmp_pres[20]) );
        
        calib.dig_P9 = (int16_t) (( (uint16_t)(buf_datos_calib_tmp_pres[23]) << 8) |
                                    (uint16_t)(buf_datos_calib_tmp_pres[22]) );
        
    } else {
        printf("E_COMPENSACION: ERROR AL RECIBIR O ENVIAR DATOS DE CALIBRACION DE TEMPERATURA Y PRESION.\n");
        return -1;
    }
    
    if(BME280_COM(REG_CALIB_HUME_1, &buf_datos_calib_hume1, 1) == 0){
        calib.dig_H1 = buf_datos_calib_hume1;
    } else {
        printf("E_COMPENSACION: ERROR AL RECIBIR O ENVIAR DATOS DE CALIBRACION DE HUMEDAD 1.\n");
        return -1;
    }
    
    if(BME280_COM(REG_CALIB_HUME_2, buf_datos_calib_hume2, 7) == 0){
        calib.dig_H2 = (int16_t) ( ((uint16_t)buf_datos_calib_hume2[1] << 8) |
                                   (uint16_t)buf_datos_calib_hume2[0]);
        
        calib.dig_H3 = buf_datos_calib_hume2[2];
        
        calib.dig_H4 = (int16_t) (( (int8_t)buf_datos_calib_hume2[3] * 16) + (buf_datos_calib_hume2[4] & 0x0F) );
        
        calib.dig_H5 = (int16_t) (( (int8_t)buf_datos_calib_hume2[5] * 16) + ((buf_datos_calib_hume2[4]) >> 4));
        
        calib.dig_H6 = (int8_t)buf_datos_calib_hume2[6];
        
    } else {
        printf("E_COMPENSACION: ERROR AL RECIBIR O ENVIAR DATOS DE CALIBRACION DE HUMEDAD 2.\n");
        return -1;
    }

    return 0;
}

int BME280_INIT(void){
    
    if(BME280_COM(REG_ID, &id, 1) == 0){
        if(id == ID_VAL){
            printf("BME280 RECONOCIDO.\n");
        } else {
            printf("E1: EL SENSOR NO CORRESPONDE AL BM280.\n");
            return -1;
        }
    } else {
        printf("E2: ERROR AL COMUNICAR BME280.\n");
        return -1;
    }
    
    if(BME280_ESCRIBIR(REG_CTRL_HUM, VAL_CTRL_HUM) != 0){
        printf("ALGO FALLO AL CONFIGURAR REGISTRO DE HUMEDAD.\n");
        return -1;
    }
    
    if(BME280_ESCRIBIR(REG_CTRL_MEAS, VAL_CTRL_MEAS) != 0){
        printf("ALGO FALLO AL CONFIGURAR REGITRO DE PRESION, TEMPERATURA Y MODO.\n");
        return -1;
    }
    
    if(leer_calibracion() < 0){
        printf("ALGO FALLO EN LEER CALIBRACION.\n");
        return -1;
    }
    return 0;
    
}

int32_t BME280_compensate_T_int32(int32_t adc_T){
    
    int32_t var1 = 0;
    int32_t var2 = 0;
    int32_t T = 0;
    
    //COMPENSACION DE TERMPERATURA
    var1 = (( (adc_T >> 3) - ((int32_t)calib.dig_T1 << 1) ) * (int32_t)calib.dig_T2) >> 11;
    var2 = ((( (adc_T >> 4) - (int32_t)calib.dig_T1) * ( (adc_T >> 4) - (int32_t)calib.dig_T1)) >> 12) * ((int32_t)calib.dig_T3) >> 14;
    
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    
   return T;
}

int32_t BME280_compensate_P_int64(int32_t adc_P){
    int64_t var1 = 0;
    int64_t var2 = 0;
    int64_t p = 0;
    
        
    //COMPENSACION DE PRESION
    var1 = (int64_t)t_fine - (int64_t)128000;
    var2 = (int64_t)var1 * (int64_t)var1 * (int64_t)calib.dig_P6;
    var2 = (int64_t)var2 + (((int64_t)var1 * (int64_t)calib.dig_P5) << 17);
    var2 = (int64_t)var2 + ((int64_t)calib.dig_P4 << 35);
    var1 = (((int64_t)var1 * (int64_t)var1 * (int64_t)calib.dig_P3) >> 8) + (((int64_t)var1 * (int64_t)calib.dig_P2) << 12);
    var1 = ((((int64_t)1) << 47) + var1) * ((int64_t)calib.dig_P1) >> 33;
    
    if(var1 == 0){
        printf("ERROR CON EL VALOR VAR1 EN BME280_compensate_P_int64\n");
        return -1;
    }
    
    p =  1048576 - adc_P;
    p = (((p << 31) - (int64_t)var2) * 3125) / var1;
    var1 = ((int64_t)calib.dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((int64_t)calib.dig_P8 * p) >> 19;
    
    p = ((p + (int64_t)var1 + (int64_t)var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
    
    return (int32_t)p;
    
}

int32_t BME280_compensate_H_int32(int32_t adc_H){
    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine - (int32_t)76800);
    v_x1_u32r =  (((((adc_H << 14) - (((int32_t)calib.dig_H4) << 20) - (((int32_t)calib.dig_H5) * 
                v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * 
                ((int32_t)calib.dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)calib.dig_H3)) >> 11) + 
                ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)calib.dig_H2) + 
                8192) >> 14));
    
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * 
                ((int32_t)calib.dig_H1)) >> 4));
    
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);  
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);  
    
    return (int32_t)(v_x1_u32r>>12);
}

int BME280_ASK_DATA(bme280_raw_t *raw){

    int32_t temp = 0;
    int32_t pres = 0;
    int32_t hume = 0;
    
    if(BME280_COM(PRESS_MSB, RESPUESTA_BME280, 8) == 0){
        
        pres = ((int32_t)RESPUESTA_BME280[0] << 12) | ((int32_t)RESPUESTA_BME280[1] << 4) | ((int32_t)RESPUESTA_BME280[2] >> 4);
        temp = ((int32_t)RESPUESTA_BME280[3] << 12) | ((int32_t)RESPUESTA_BME280[4] << 4) | ((int32_t)RESPUESTA_BME280[5] >> 4);
        hume = ((uint16_t)RESPUESTA_BME280[6] << 8) | ((uint16_t)RESPUESTA_BME280[7]);
        
    } else {
        printf("E_DATOS: ERROR AL ENVIAR RAFAGAS DE DATOS\n");
        return -1;
    }
    
    raw -> temperatura  = BME280_compensate_T_int32(temp);
    raw -> presion      = BME280_compensate_P_int64(pres);
    raw -> humedad      = BME280_compensate_H_int32(hume);
    
    return 0;
}
