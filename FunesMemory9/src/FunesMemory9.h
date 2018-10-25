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
#include "funcionesFM9.h"

t_log_mutex * logger;
configFM9 * config;

fd_set master;
fd_set readset;
int maxfd;
void * storage;

typedef struct{
	int nroSegmento;
	int offset;
}t_segmento;

typedef struct{
	t_dtb * dtb;
	t_segmento segmentoGDT;
}t_gdt;

typedef struct{
	int *idProceso;
	int base;
	int limite;
}t_tablaSegmentos;

typedef struct{
	int base;
	int offset;
}t_bloqueMemoria;

pthread_mutex_t mutexMaster;
pthread_mutex_t mutexReadset;
pthread_mutex_t mutexMaxfd;
pthread_mutex_t mutexExit;

void manejarConexiones();
void manejarSolicitud(t_package pkg, int socketFD);
void liberarRecursos();
int guardarLineaSegunEsquemaMemoria(t_package pkg, int socketSolicitud);
int cargarEscriptorioSegunEsquemaMemoria(t_package pkg, int socketSolicitud);
void ejecutarEsquemaTPI(t_package pkg, int socketSolicitud, int accion);
void ejecutarEsquemaSegPag(t_package pkg, int socketSolicitud, int accion);
int ejecutarCargarEsquemaSegmentacion(t_package pkg, int socketSolicitud);
int ejecutarCargarEsquemaTPI(t_package pkg, int socketSolicitud);
int ejecutarCargarEsquemaSegPag(t_package pkg, int socketSolicitud);
int ejecutarGuardarEsquemaSegmentacion(t_package pkg);
int ejecutarGuardarEsquemaTPI(t_package pkg);
int ejecutarGuardarEsquemaSegPag(t_package pkg);
int retornarLineaSolicitada(t_package pkg, int socketSolicitud);
#endif /* FUNESMEMORY9_H_ */
