/*
 * funcionesSAFA.h
 *
 *  Created on: 11 sep. 2018
 *      Author: utnso
 */

#ifndef FUNCIONESSAFA_H_
#define FUNCIONESSAFA_H_
#include <grantp/structCommons.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <commons/string.h>
#include <grantp/configuracion.h>
#include <grantp/mutex_log.h>
#include "stdbool.h"


//extern t_list *statusList;
//extern pthread_mutex_t mutexStatusList;
extern t_list* colaNew;			// Lista New.
extern t_list* colaReady;			// Lista Ready.
extern t_list* colaBloqueados;		// Lista Bloqueados.
extern t_list* colaEjecutando;     // Lista DTBs en ejec
extern t_list* colaExit;		// Lista Terminados.

extern pthread_mutex_t mutexMaster;
extern pthread_mutex_t mutexReadset;
extern pthread_mutex_t mutexMaxfd;
//extern pthread_mutex_t mutexTime;
extern pthread_mutex_t mutexStop;
extern pthread_mutex_t mutexConsole;
extern pthread_mutex_t mutexgdtCounter;
extern pthread_mutex_t mutexNewList;
extern pthread_mutex_t mutexReadyList;
extern pthread_mutex_t mutexBloqueadosList;
extern pthread_mutex_t mutexEjecutandoList;
extern pthread_mutex_t mutexExitList;
extern pthread_mutex_t mutexDummy;

//extern int tiempo;
extern int maxfd;
extern int console;
extern int scheduler;
extern int gdtCounter;
extern fd_set master;
extern fd_set readset;
extern configSAFA *conf;
extern t_log_mutex *logger;

extern int dummyBloqueado;




//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA EL MANEJO DEL SELECT
//------------------------------------------------------------------------------------------------------------------

void addNewSocketToMaster(int socket);
int getMaxfd();
void updateMaxfd(int socket);
void deleteSocketFromMaster(int socket);
void updateReadset();
int isSetted(int socket);

//======================================================================================================================================
//============================================FUNCIONES Consola=========================================================================
//======================================================================================================================================

void setPlay();
void setPause();
int getConsoleStatus();
void playExecute();
void stopExecute();
int getExecute();


//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES GDT y DTB
//------------------------------------------------------------------------------------------------------------------

t_dtb *crearNuevoDTB(char *dirScript);
int agregarDTBaNEW(t_dtb *dtb);
void sumarGDTCounter();
int obtenerGDTCounter();
t_dtb *crearDummyDTB();
void desbloquearDummy();
void bloquearDummy();
int obtenerEstadoDummy();



#endif /* FUNCIONESSAFA_H_ */
