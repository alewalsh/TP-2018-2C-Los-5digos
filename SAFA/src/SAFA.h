/*
 * SAFA.h
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#ifndef SAFA_H_
#define SAFA_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <commons/log.h>
#include <pthread.h>
#include <grantp/socket.h>
#include "handlerConexiones.h"
#include <grantp/configuracion.h>
#include "consola.h"
#include <sys/inotify.h>
#include <unistd.h>


//Para el SELECT
fd_set master;
fd_set readset;

// ------------------------------------------------------------------------------
//	MUTEX GLOBALES a ser utilizados
// ------------------------------------------------------------------------------
pthread_mutex_t mutexMaster;
pthread_mutex_t mutexNewList;
pthread_mutex_t mutexReadyList;
pthread_mutex_t mutexBloqueadosList;
pthread_mutex_t mutexEjecutandoList;
pthread_mutex_t mutexExitList;
pthread_mutex_t mutexReadset;
pthread_mutex_t mutexMaxfd;
//pthread_mutex_t mutexTime;
pthread_mutex_t mutexgdtCounter;
pthread_mutex_t mutexDummy;

pthread_mutex_t mutexExit;

pthread_mutex_t mutexStop;
//pthread_mutex_t mutexReadyExecute;
pthread_mutex_t mutexConsole;


// ------------------------------------------------------------------------------
//	VARIABLES GLOBALES
// ------------------------------------------------------------------------------
configSAFA *conf;
t_log_mutex *logger;

int shouldExit;
int maxfd;
//int coordinator = 0;
//int tiempo = 0;
int console = 1;         //VER SI LAS TENGO QUE USAR
int scheduler = 0;       //VER SI LAS TENGO QUE USAR
int gdtCounter = 0;

t_list* colaNew;			// Lista New.
t_list* colaReady;			// Lista Ready.
t_list* colaBloqueados;		// Lista Bloqueados.
t_list* colaEjecutando;     // Lista DTBs en ejec
t_list* colaExit;		// Lista Terminados.

t_cpus* listaCpus;

////informa que se corrio un EJECUTAR y hay dtbs en NEW
//sem_t sem_DTBenNEW;

////informa que debe ejecutar
//sem_t sem_shouldExecute;
//
////informa si se esta ejecutando(tener en cuenta ya que la corridas son atomicas)
//sem_t sem_shouldScheduler;
//
//sem_t sem_preemptive;

// ------------------------------------------------------------------------------
//	VARIABLES SAFA
// ------------------------------------------------------------------------------

int inotifyFd;
int inotifyWd;
char inotifyBuf[200];

// ------------------------------------------------------------------------------
//	METODOS
// ------------------------------------------------------------------------------
void inicializarRecursos(char * pathConfig);
void liberarRecursos();
void initMutexs();
void initList();
void cambiosConfig();
void manejoCortoPlazo();
void manejoLargoPlazo();

void sig_handler(int signo);
void exit_gracefully(int error);


#endif /* SAFA_H_ */
