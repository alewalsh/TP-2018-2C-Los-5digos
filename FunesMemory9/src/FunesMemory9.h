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
#include <grantp/compression.h>
#include <grantp/split.c>
#include <commons/bitarray.h>
#include "funcionesFM9.h"
#include <errno.h>
#include "segmentacionSimple.h"
#include "TPI.h"
#include "segPaginada.h"

int contLineasUsadas;
int cantLineas;
int cantPaginas;
int lineasXPagina;
int pidBuscado;

t_log_mutex * logger;
configFM9 * config;
t_dictionary * tablaProcesos;
t_list * tablaPaginasInvertida;
t_bitarray * estadoLineas;
t_bitarray * estadoMarcos;
char * storage;

fd_set master;
fd_set readset;
int maxfd;

pthread_mutex_t mutexMaster;
pthread_mutex_t mutexReadset;
pthread_mutex_t mutexMaxfd;
pthread_mutex_t mutexExit;
pthread_mutex_t mutexPaginaBuscada;
pthread_mutex_t mutexPIDBuscado;

void inicializarSemaforos();
void inicializarContadores();
void manejarConexiones();
void manejarSolicitud(t_package pkg, int socketFD);
void liberarRecursos();
bool existeProceso(int pid);

int cerrarArchivoSegunEsquemaMemoria(t_package pkg, int socketSolicitud);
void logicaCerrarArchivo(t_package pkg, int socketSolicitud, int code);

int guardarLineaSegunEsquemaMemoria(t_package pkg, int socketSolicitud);
void logicaGuardarLinea(t_package pkg, int socketSolicitud, int code);

int cargarEscriptorioSegunEsquemaMemoria(t_package pkg, int socketSolicitud);
void logicaCargarEscriptorio(t_package pkg, int socketSolicitud, int code);

int realizarFlushSegunEsquemaMemoria(t_package pkg, int socketSolicitud);
void logicaFlush(t_package pkg, int socketSolicitud, int code);

#endif /* FUNESMEMORY9_H_ */
