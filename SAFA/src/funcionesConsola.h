/*
 * funcionesConsola.h
 *
 *  Created on: 11 sep. 2018
 *      Author: utnso
 */

#ifndef FUNCIONESCONSOLA_H_
#define FUNCIONESCONSOLA_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "funcionesSAFA.h"
#include <semaphore.h>

extern pthread_mutex_t mutexExit;

extern t_list* colaNew;			// Lista New.
extern t_list* colaReady;			// Lista Ready.
extern t_list* colaBloqueados;		// Lista Bloqueados.
extern t_list* colaEjecutando;     // Lista DTBs en ejec
extern t_list* colaExit;		// Lista Terminados.
extern t_list* colaReadyEspecial;
//TODO: Vamos a tener que agregar la cola especial que creo Fran
extern t_list* listaMetricasTRDefinitiva;
extern sem_t mandadosPorConsola;

void consolaEjecutar(char *args);
void consolaLiberar();
void consolaStatus();
void imprimirNEW();
void imprimirREADY();
void imprimirEJECUTANDO();
void imprimirBLOQUEADOS();
void imprimirEXIT();
void imprimirREADYESPECIAL();
void consolaStatusDTB(char *args);
void imprimirDTB(t_dtb * dtb);
void consolaMetricasDTB(char *args);
void consolaMetricas();
void imprimirDtbsEnNew();
void imprimirSentenciasDAM();
void imprimirPromedioSentenciasDam();
void imprimirSentenciasPromEnExit();
void imprimirMetricaTiempoDeRespuestaPromedio();
void consoleExit();
void consoleHelp();
void consoleClear();
int getIdFunction(char *function);
void parseCommand(char *line, char **command, char **args);
void freeCommand(char *command, char *args);

#endif /* FUNCIONESCONSOLA_H_ */
