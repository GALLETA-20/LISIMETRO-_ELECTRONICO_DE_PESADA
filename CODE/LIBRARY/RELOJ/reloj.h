#ifndef RELOJ_H
#define RELOJ_H

#include <stdint.h>

typedef struct{
    uint16_t    cont_val_ini; //VARIABLE QUE CONTIENE EL TIEMPO (TICKS) INICIAL.
    uint16_t    periodo_ms; //VARIABLE QUE CONTIENE EL LA CANTIDAD DE TIEMPO (TICKS) QUE DEBEN TRANSCURRIR.
    uint8_t     flag_start_cont; //BANDERA QUE INDICA SI EL CONTADOR ESTÁ ACTIVO
} temporizador_t;


void reloj(void);

uint8_t time_delay_ms(temporizador_t *tmp);
void arranque_tmp(temporizador_t *tmp);

#endif