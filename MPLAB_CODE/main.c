#include "mcc_generated_files/system/system.h"
#include <stdio.h>
#include "lora_uart.h"
#include "reloj.h"
#include "BME280_I2C.h"
#include "max31855.h"
#include "nau7802.h"

#define LARGO_MSJ_SEND 40 //TAMAÑO DEL MENSAJE QUE SE ENVIA POR UART2 (AUMENTADO POR NAU7802 DE 24 BITS)
#define TAM_MSJ_SENSORS (LARGO_MSJ_SEND + 1) //TAMAÑO DEL BUFFER DONDE SE GUARDAN LOS VALORES DE LOS SENSORES

//LOS TIEMPOS DE PERIODO ESTÁN EN MS DEBIDO A QUE EL TIMER ESTÁ CONFIGURADO A 1MS POR TICK
#define PERIODO_SEND_MS 5000 //TIEMPO DE ENVIO DEL LORA ES 5 SEGUNDOS
#define PERIODO_ANTIREBOTE_MS 100

typedef enum MODO_ESTADO{
    CONFIG,
    RD_WR
} modo_t;

//VARIABLES GLOBALES
static modo_t MODO_ACTUAL;    //DEFINE EL MODO DEL CONTROLADOR.
static volatile uint8_t flag_sw_val = 0; //VALOR DE LA BANDERA - PONE 1 SI SE DETECTA UN FLANCO EN EL PIN DE INTERRUPCIÓN.
static temporizador_t tmp_antirrebote = {0, PERIODO_ANTIREBOTE_MS, 0};
static temporizador_t tmp_delay_5_seg = {0, PERIODO_SEND_MS, 0};

static uint8_t status_bme280 = 0;
static uint8_t status_nau7802 = 0;
static bme280_raw_t data_read_bme280 = {0};

//FUNCIONES
static void config_modo(void);
static int read_sensors(char *msj_sensors, size_t size_sen);


void flag_sw_mode(void){
    flag_sw_val = 1; 
}


int main(void)
{
    SYSTEM_Initialize();
    MAX31855_Init();
    MODE_SetInterruptHandler(flag_sw_mode);
    TMR1_OverflowCallbackRegister(reloj);
    INTERRUPT_GlobalInterruptEnable(); 

    //LEEMOS EL PIN Y SETEAMOS EL MODO
    config_modo();
    char MSJ_SENSORS[TAM_MSJ_SENSORS] = {0};
    int largo_msj;
    
    if(BME280_INIT() == 0){
        status_bme280 = 1;
    }

    if(NAU7802_Init()){
        status_nau7802 = 1;
    }

    while(1)
    {
        if(flag_sw_val == 1){
            
            flag_sw_val = 0;
            arranque_tmp(&tmp_antirrebote);
            
        }
        
        if(time_delay_ms(&tmp_antirrebote) == 1) config_modo();
        
        switch (MODO_ACTUAL){
            // SI ES CONFIG SE LEE EL UART 1 Y 2, PARA LA CONFIGURACIÓN AT.
            case CONFIG:
                com_uart1_uart2();
            break;

            //RD_WR ENVIA LOS VALORES DE LOS SENSORES POR EL UART2 PARA EL LORA.
            case RD_WR:
                
                if(time_delay_ms(&tmp_delay_5_seg) == 1){
                    largo_msj = read_sensors(MSJ_SENSORS, TAM_MSJ_SENSORS);
                    if(largo_msj == LARGO_MSJ_SEND){
                        INDICADOR_Toggle();
                        send_uart2(MSJ_SENSORS, largo_msj);
                        
                    } else {
                        INDICADOR_SetLow();
                    }
                    
                    arranque_tmp(&tmp_delay_5_seg);
                      
                }
                
            break;
        }
    }    
}


/*FUNCIÓN QUE DEFINE EL MODO OPERATIVO DEL PIC.
- MODO CONFIGURACIÓN: LEE EL UART1 Y UART2 PARA CONFIGURACIÓN DEL LORA CON AT
- MODO RD_WR: LEE LOS SENSORES Y ENVIA LOS DATOS POR UART2 QUE ES EL LORA.
 */
void config_modo(void){
    vaciar_buff_uart();
    INDICADOR_SetHigh();

    if(MODE_GetValue() == HIGH){
        MODO_ACTUAL = CONFIG;
        printf("MODO CONFIG");
    } else {
        MODO_ACTUAL = RD_WR;
        printf("MODO ENVIO");
        arranque_tmp(&tmp_delay_5_seg);
    }
}


int read_sensors(char *msj_sensors, size_t size_sen){
    /*EL LARGO DEL MENSAJE ES CALCULADO CON LA CANTIDAD DE NUMEROS Y COMAS QUE SE UNEN EN snprintf.*/
    
    float PRES_BME280;
    float HUME_BME280;
    uint16_t TEMP_PT100;
    int32_t CELL_LOAD = 0;
    uint8_t PLUVI;
    max31855_data_t data_max31855 = {0};
    
    // LECTURA DEL BME280
    if(status_bme280){
        if(BME280_ASK_DATA(&data_read_bme280) == 0){
            PRES_BME280 = (float)data_read_bme280.presion / 256.0f;
            HUME_BME280 = (float)data_read_bme280.humedad / 1024.0f;
        } else {
            printf("ERROR AL PEDIR DATOS BME280.\n");
            return -1;
        }
    } else {
        printf("M1: ERROR EN LA COMUNICACION DEL BME280.\n");
        return -1;
    }

    // LECTURA DEL MAX31855 (TEMPERATURA EN CENTESIMAS DE °C)
    if(MAX31855_Read(&data_max31855)){
        if(data_max31855.tc_x100 < 0){
            TEMP_PT100 = 0;
        } else {
            TEMP_PT100 = (uint16_t)data_max31855.tc_x100;
        }
    } else {
        if(data_max31855.fault & MAX31855_FAULT_OPEN)      printf("M2: ERROR MAX31855 - TERMOPAR ABIERTO.\n");
        if(data_max31855.fault & MAX31855_FAULT_SHORT_GND) printf("M2: ERROR MAX31855 - CORTO A GND.\n");
        if(data_max31855.fault & MAX31855_FAULT_SHORT_VCC) printf("M2: ERROR MAX31855 - CORTO A VCC.\n");
        return -1;
    }

    // LECTURA DEL NAU7802 (PROMEDIO DE 5 MUESTRAS CRUDAS DE 24 BITS CON SIGNO)
    if(status_nau7802){
        if(!NAU7802_ReadAverage(5, &CELL_LOAD)){
            printf("M3: ERROR AL LEER EL NAU7802.\n");
            return -1;
        }
    } else {
        printf("M3: NAU7802 NO INICIALIZADO.\n");
        return -1;
    }

    // VALOR EMULADO RESTANTE
    PLUVI = 14;

    int tam = snprintf(msj_sensors, size_sen, "%05u,%+09ld,%04ld,%06.2f,%08.1f,%02u\n",
                       TEMP_PT100, (long)CELL_LOAD, (int32_t)data_read_bme280.temperatura,
                       HUME_BME280, PRES_BME280, PLUVI);
    return tam;  
}