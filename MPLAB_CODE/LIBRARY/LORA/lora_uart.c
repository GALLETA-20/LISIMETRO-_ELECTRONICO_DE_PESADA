#include "mcc_generated_files/system/system.h"
#include "lora_uart.h"

/*
 * DEJAMOS EL BUFFER VACIO EN CADA CAMBIO DE MODO PARA QUE AL ENTRAR EN ALGUNO DE ELLOS, EL BUFER ESTÉ VACÍO PARA LA TRANSMISIÓN
 * Y RECEPCIÓN DE DATOS
 */
void vaciar_buff_uart(void){
    while(UART1_IsRxReady()){
        UART1_Read();
    }

    while(UART2_IsRxReady()){
        UART2_Read();
    }
}


void com_uart1_uart2(void){
    if(UART1_IsRxReady() && UART2_IsTxReady()){
        UART2_Write(UART1_Read());
    }

    if(UART2_IsRxReady() && UART1_IsTxReady()){
        UART1_Write(UART2_Read());
    } 

}


void send_uart2(char *MSJ_SENSORS, int largo_msj){
    
    //ENVIO DE DATOS DE LOS SENSORES POR UART2
    for(int i = 0; i < largo_msj; i++){
        UART2_Write(MSJ_SENSORS[i]);
    }
}