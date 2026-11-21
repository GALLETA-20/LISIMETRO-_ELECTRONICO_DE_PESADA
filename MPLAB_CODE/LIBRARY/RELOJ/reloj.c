#include "reloj.h"
#include "mcc_generated_files/system/system.h"

static volatile uint16_t tiks_tmr1 = 0; //VALOR ACTUAL DEL CONTADOR GENERAL.

void reloj(void){
    tiks_tmr1++;
}



/*CUANDO LA FUNCIÓN ES LLAMADA, ESTA APAGA LAS INTERRUPCIONES GLOBALES, LUEGO
 ASIGNA EL VALOR DE tiks_tmr1 A cp_tiks_tmr1, DEJA LAS INTERRUPCIONES EN SU VALOR ANTERIOR A LA FUNCION
 Y DEVUELVE EL VALORES DE cp_tiks_tmr1*/
static uint16_t reg_val_tmr1(void){
    uint16_t cp_tiks_tmr1;
    uint8_t gie_previo = INTCON0bits.GIE;
    
    INTERRUPT_GlobalInterruptDisable(); 
    cp_tiks_tmr1 = tiks_tmr1;
    if(gie_previo) INTERRUPT_GlobalInterruptEnable();
    
    return cp_tiks_tmr1;
}


void arranque_tmp(temporizador_t *tmp){
    
    tmp -> cont_val_ini = reg_val_tmr1();
    tmp -> flag_start_cont = 1;  
    
}


uint8_t time_delay_ms(temporizador_t *tmp){
    if((tmp -> flag_start_cont) == 1){

        /*EL CONTADOR GENERAL SE COMPARA CON UN CONTADOR DE 16BITS - SI SE DESEA AMPLIAR
         *EL TIEMPO DE ESPERA DEBE VERIFICAR QUE NO EXCEDA LOS 65 535, SI ES ASÍ DEBE HACER CONFIGURACIONES DEL TIMER*/
        uint16_t cp_tiks_tmr1 = reg_val_tmr1();
        
        /*HACE UNA RESTA ENTRE EL VALOR ACTUAL DE CONTADOR CON EL VALOR INICIAL Y SI LA RESTA
         GENERA UN VALOR NEGATIVO LO CONVIERTE A POSITIVO CON SU RESPECTIVO VALOR REAL*/
        uint16_t ttmr1_subs_count = (uint16_t)(cp_tiks_tmr1 - (tmp -> cont_val_ini));

        if(ttmr1_subs_count >= (tmp->periodo_ms)){
             tmp -> flag_start_cont = 0;
            return 1;
        } else {
            return 0;
        }
    }
    
    return 0;

}
