#ifndef CONTROLE_H
#define CONTROLE_H

#include <stdbool.h>
#include "pico/util/datetime.h"

void controle_ventilador(float temperatura);

void atualizar_horario_racao(const char *momento, int hora, int minuto);
void atualizar_horario_limpeza(int hora, int minuto);

void acionar_racao_manual();
void acionar_limpeza_manual();

void verificar_acionamento_racao(const datetime_t *t);
void verificar_acionamento_limpeza(const datetime_t *t);

#endif
