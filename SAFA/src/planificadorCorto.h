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
extern t_list* colaReadyEspecial; // Lista Ready Especial

int dummyBloqueado;

extern pthread_mutex_t mutexPlanificando;
extern sem_t desbloquearDTBDummy;
extern sem_t enviarDtbACPU;
void manejarDummy();
void manejarDispatcher();
void planificadorCP();
void ejecutarRR(int socketCpu);
void ejecutarVRR(int socketCpu);
void ejecutarIOBF(int socketCpu);
bool procesoConMayorCantIO(t_dtb * p1, t_dtb * p2);
void enviarDTBaCPU(t_dtb *dtbAEnviar, int socketCpu);
int buscarCPULibre();
int buscarDtbParaInicializar();

t_dtb *pasarDTBdeREADYaEXEC();
t_dtb *pasarDTBdeREADYESPaEXEC();

int pasarDTBdeEXECaBLOQUED(t_dtb * dtbABloq);
int pasarDTBdeEXECaREADY(t_dtb * dtbABloq);
int pasarDTBdeEXECaFINALIZADO(t_dtb * dtbABloq);

int pasarDTBdeBLOQUEADOaFINALIZADO(t_dtb * dtbABloq);
void pasarDTBdeBLOQaREADY(t_dtb * dtbAReady);
void pasarDTBdeBLOQaREADYESP(t_dtb * dtbAReadyEsp);


void pasarDTBdeNEWaREADY(t_dtb * dtbAReady);
int pasarDTBdeNEWaEXIT(t_dtb * dtbAExit);

void pasarDTBSegunQuantumRestante(t_dtb * dtb);
//variable global

bool obtenerDummy(t_dtb * dtb);

#endif /* PLANIFICADORCORTO_H_ */
