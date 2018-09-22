/*
 * FunesMemory9.h
 *
 *  Created on: 16 sep. 2018
 *      Author: utnso
 */

#ifndef FUNESMEMORY9_H_
#define FUNESMEMORY9_H_

#include <stdio.h>
#include <stdlib.h>
#include <grantp/configuracion.h>
#include <grantp/socket.h>
#include <grantp/mutex_log.h>
#include <grantp/structCommons.h>
#include "funcionesFM9.h"

t_log_mutex * logger;
configFM9 * config;

fd_set master;
fd_set readset;
int maxfd;

pthread_mutex_t mutexMaster;
pthread_mutex_t mutexReadset;
pthread_mutex_t mutexMaxfd;
pthread_mutex_t mutexExit;

void manejarConexiones();
void manejarSolicitud(t_package pkg, int socketFD);
void liberarRecursos();

#endif /* FUNESMEMORY9_H_ */
