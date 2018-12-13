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
#include "planificadorCorto.h"
#include "planificadorLargo.h"

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
pthread_mutex_t mutexReadyEspList;
pthread_mutex_t mutexReadset;
pthread_mutex_t mutexMaxfd;
pthread_mutex_t mutexgdtCounter;
pthread_mutex_t mutexDummy;
pthread_mutex_t mutexExit;
pthread_mutex_t mutexStop;
pthread_mutex_t mutexConsole;

sem_t mandadosPorConsola;

// ------------------------------------------------------------------------------
//	VARIABLES GLOBALES
// ------------------------------------------------------------------------------
configSAFA *conf;
t_log_mutex *logger;

int maxfd;
int console = 1;         //VER SI LAS TENGO QUE USAR
int scheduler = 0;       //VER SI LAS TENGO QUE USAR
int gdtCounter = 0;

t_list* colaNew;			// Lista New.
t_list* colaReady;			// Lista Ready.
t_list* colaBloqueados;		// Lista Bloqueados.
t_list* colaEjecutando;     // Lista DTBs en ejec
t_list* colaExit;		// Lista Terminados.
t_list* colaReadyEspecial;
t_list* listaMetricasLP;

// ------------------------------------------------------------------------------
//	VARIABLES SAFA
// ------------------------------------------------------------------------------

#define EVENT_SIZE  ( sizeof (struct inotify_event) + 24 )
#define BUF_LEN     ( 1024 * EVENT_SIZE )
char * rutaConfig;
char * rutaConfigSinCofig;

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
void liberarRecursos();

#endif /* SAFA_H_ */
