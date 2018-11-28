/*
 * planificadorCorto.h
 *
 *  Created on: 20 nov. 2018
 *      Author: utnso
 */

#ifndef PLANIFICADORCORTO_H_
#define PLANIFICADORCORTO_H_

#include "funcionesSAFA.h"
#include <grantp/configuracion.h>
#include <grantp/compression.h>
#include "handlerConexiones.h"

extern configSAFA *conf;
extern t_list* colaNew;			// Lista New.
extern t_list* colaReady;			// Lista Ready.
extern t_list* colaBloqueados;		// Lista Bloqueados.
extern t_list* colaEjecutando;     // Lista DTBs en ejec
extern t_list* colaExit;		// Lista Terminados.

t_dtb *dummyDTB;
int dummyBloqueado;

pthread_mutex_t mutexPlanificando;


void planificadorCP();
void planificadorCPdesbloquearDummy(int idGDT, char *dirScript);
void ejecutarRR();
t_dtb *pasarDTBdeREADYaEXEC();
void enviarDTBaCPU(t_dtb *dtbAEnviar);
void pasarDTBdeEXECaBLOQUED(t_dtb * dtbABloq);
int buscarCPULibre();
void planificadorCPdesbloquearDummy(int idGDT, char *dirScript);
//variable global

#endif /* PLANIFICADORCORTO_H_ */
