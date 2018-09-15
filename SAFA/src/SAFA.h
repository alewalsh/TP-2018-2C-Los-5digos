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
#include <socket.h>
#include "handlerConexiones.h"
#include <configuracion.h>
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
//pthread_mutex_t mutexStatusList;
//pthread_mutex_t mutexBlockedKeys;
pthread_mutex_t mutexReadset;
pthread_mutex_t mutexMaxfd;
//pthread_mutex_t mutexTime;
pthread_mutex_t mutexExit;
//pthread_mutex_t mutexStop;
//pthread_mutex_t mutexReadyExecute;
//pthread_mutex_t mutexConsole;


// ------------------------------------------------------------------------------
//	VARIABLES GLOBALES
// ------------------------------------------------------------------------------
configSAFA *conf;
t_log_mutex *logger;
int shouldExit;
int maxfd;
//int coordinator = 0;
//int tiempo = 0;
//int console = 1;
//int scheduler = 0;


// ------------------------------------------------------------------------------
//	VARIABLES SAFA
// ------------------------------------------------------------------------------

int inotifyFd;
int inotifyWd;
char inotifyBuf[200];
char* configFilePath;

// ------------------------------------------------------------------------------
//	METODOS
// ------------------------------------------------------------------------------
void inicializarRecursos();
void liberarRecursos();
void initMutexs();

void sig_handler(int signo);
void exit_gracefully(int error);


#endif /* SAFA_H_ */
