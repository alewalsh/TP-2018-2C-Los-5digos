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
#include <commons/bitarray.h>
#include "funcionesFM9.h"
#include <errno.h>

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

typedef struct{
	int nroSegmento;
	int base;
	int limite;
	char * archivo;
} t_segmento;

typedef struct{
	int nroPagina;
	int pid;
	char * path;
} t_pagina;

typedef struct{
	t_dictionary * tablaSegmentos;
	t_dictionary * tablaPaginas;
} t_gdt;

typedef struct{
	int base;
	int offset;
} t_bloqueMemoria;

pthread_mutex_t mutexMaster;
pthread_mutex_t mutexReadset;
pthread_mutex_t mutexMaxfd;
pthread_mutex_t mutexExit;

void inicializarContadores();
void manejarConexiones();
void manejarSolicitud(t_package pkg, int socketFD);
void liberarRecursos();
int guardarLineaSegunEsquemaMemoria(t_package pkg, int socketSolicitud);
int cargarEscriptorioSegunEsquemaMemoria(t_package pkg, int socketSolicitud);
void ejecutarEsquemaTPI(t_package pkg, int socketSolicitud, int accion);
void ejecutarEsquemaSegPag(t_package pkg, int socketSolicitud, int accion);
int ejecutarCargarEsquemaSegmentacion(t_package pkg, int socketSolicitud);
int ejecutarCargarEsquemaTPI(t_package pkg,int socketSolicitud);
void ejecutarCargarEsquemaSegPag(t_package pkg, int socketSolicitud);
int ejecutarGuardarEsquemaSegmentacion(t_package pkg, int socket);
int ejecutarGuardarEsquemaTPI(t_package pkg, int socket);
int ejecutarGuardarEsquemaSegPag(t_package pkg);
int retornarLineaSolicitada(t_package pkg, int socketSolicitud);

static void liberar_segmento(t_segmento *self);
static void liberar_pagina(t_pagina * self);

void inicializarBitmap(t_bitarray * bitArray);
int cerrarArchivoSegunEsquemaMemoria(t_package pkg, int socketSolicitud);
void logicaCerrarArchivoSegmentacion(t_package pkg, int socketSolicitud);
int cerrarArchivoSegmentacion(t_package pkg, int socketSolicitud);
void guardarLinea(int posicionMemoria, char * linea);
char * intToString(int numero);
void logicaGuardarSegmentacion(t_package pkg, int socketSolicitud);
void liberarLineas(int base, int limite);
void logicaCargarEscriptorioSegmentacion(t_package pkg, int socketSolicitud);
int direccion(int base, int desplazamiento);
int posicionesLibres(t_bitarray * bitArray);

void actualizarPosicionesLibres(int finalBitArray, int lineasEsperadas, t_bitarray * bitArray);
bool filtrarPorPid(t_pagina * pagina);
void reservarPaginasNecesarias(int paginasAReservar, int pid, char * path);
void actualizarTPI(t_pagina * pagina);

void logicaCargarEscriptorioTPI(t_package pkg, int socketSolicitud);
void liberarMarco(t_pagina * pagina);
void logicaCerrarArchivoTPI(t_package pkg, int socketSolicitud);
int cerrarArchivoTPI(t_package pkg, int socketSolicitud);
#endif /* FUNESMEMORY9_H_ */
