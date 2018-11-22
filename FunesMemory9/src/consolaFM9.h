/*
 * consolaFM9.h
 *
 *  Created on: 22 nov. 2018
 *      Author: utnso
 */

#ifndef SRC_CONSOLAFM9_H_
#define SRC_CONSOLAFM9_H_

#include <string.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <grantp/configuracion.h>
#include <grantp/socket.h>
#include <grantp/mutex_log.h>
#include <grantp/structCommons.h>

pthread_t threadConsolaFM9;
pthread_attr_t tattr;

extern char * storage;
extern t_log_mutex * logger;
extern configFM9 * config;

void manejarConsolaFM9();
void ejecutarDumpSegunEsquemaMemoria(char * storage);
void dumpEnSegmentacionPura(char * storage);
void dumpEnTPI(char * storage);
void dumpEnSegPag(char * storage);
int esUnNumeroIDProceso(char * numero);

#endif /* SRC_CONSOLAFM9_H_ */
