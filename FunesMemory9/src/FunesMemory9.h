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
	int pid;
	char * path;
	int transferSize;
} t_datosFlush;

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
	int lineasUtilizadas;
} t_pagina;

typedef struct{
	t_dictionary * tablaSegmentos;
	t_list * tablaPaginas;
} t_gdt;

typedef struct{
	int pid;
	char * path;
	int linea;
	char * datos;
} t_infoGuardadoLinea;

typedef struct{
	int pid;
	int cantPaquetes;
	char * path;
} t_infoCargaEscriptorio;

typedef struct{
	int pid;
	char * path;
} t_infoCerrarArchivo;

pthread_mutex_t mutexMaster;
pthread_mutex_t mutexReadset;
pthread_mutex_t mutexMaxfd;
pthread_mutex_t mutexExit;

void inicializarContadores();
void manejarConexiones();
void manejarSolicitud(t_package pkg, int socketFD);
void liberarRecursos();
bool existeProceso(int pid);
int guardarLineaSegunEsquemaMemoria(t_package pkg, int socketSolicitud);
int cargarEscriptorioSegunEsquemaMemoria(t_package pkg, int socketSolicitud);
void ejecutarEsquemaTPI(t_package pkg, int socketSolicitud, int accion);
void ejecutarEsquemaSegPag(t_package pkg, int socketSolicitud, int accion);
int ejecutarCargarEsquemaSegmentacion(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud);
int ejecutarCargarEsquemaTPI(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud);
int ejecutarCargarEsquemaSegPag(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud);
int ejecutarGuardarEsquemaSegmentacion(t_package pkg, t_infoGuardadoLinea* datosPaquete, int socket);
int ejecutarGuardarEsquemaTPI(t_package pkg, t_infoGuardadoLinea* datosPaquete, int socket);
int ejecutarGuardarEsquemaSegPag(t_package pkg, t_infoGuardadoLinea* datosPaquete, int socket);

static void liberar_segmento(t_segmento *self);
static void liberar_pagina(t_pagina * self);

void inicializarBitmap(t_bitarray * bitArray);
int cerrarArchivoSegunEsquemaMemoria(t_package pkg, int socketSolicitud);
void logicaCerrarArchivoSegmentacion(t_package pkg, t_infoCerrarArchivo* datosPaquete, int socketSolicitud);
int cerrarArchivoSegmentacion(t_package pkg, t_infoCerrarArchivo* datosPaquete, int socketSolicitud);
void guardarLinea(int posicionMemoria, char * linea);
char * intToString(int numero);
void logicaGuardarSegmentacion(t_package pkg, t_infoGuardadoLinea* datosPaquete, int socketSolicitud);
void liberarLineas(int base, int limite);
void logicaCargarEscriptorioSegmentacion(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud);
int direccion(int base, int desplazamiento);
int posicionesLibres(t_bitarray * bitArray);

void actualizarPosicionesLibres(int finalBitArray, int lineasEsperadas, t_bitarray * bitArray);
bool filtrarPorPid(t_pagina * pagina);
void reservarPaginasNecesarias(int paginasAReservar, int pid, char * path, int lineasAOcupar);
void actualizarTPI(t_pagina * pagina);

void logicaCargarEscriptorioTPI(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud);
void liberarMarco(t_pagina * pagina);
void logicaCerrarArchivoTPI(t_package pkg, t_infoCerrarArchivo* datosPaquete, int socketSolicitud);
int cerrarArchivoTPI(t_package pkg, t_infoCerrarArchivo* datosPaquete, int socketSolicitud);
void logicaCerrarArchivoSegPag(t_package pkg, t_infoCerrarArchivo* datosPaquete, int socketSolicitud);
int cerrarArchivoSegPag(t_package pkg, t_infoCerrarArchivo* datosPaquete, int socketSolicitud);

t_infoGuardadoLinea * guardarDatosPaqueteGuardadoLinea(t_package pkg);
t_infoCargaEscriptorio * guardarDatosPaqueteCargaEscriptorio(t_package pkg);
t_infoCerrarArchivo * guardarDatosPaqueteCierreArchivo(t_package pkg);
t_datosFlush * guardarDatosPaqueteFlush(t_package pkg);

int realizarFlushSegunEsquemaMemoria(t_package pkg, int socketSolicitud);
void logicaFlush(t_package pkg, int socketSolicitud, int code);
int flushSegmentacionPaginada(t_package pkg, int socketSolicitud, t_datosFlush * data);
int flushTPI(t_package pkg, int socketSolicitud, t_datosFlush * data);
int flushSegmentacion(t_package pkg, int socketSolicitud, t_datosFlush * data);
void realizarFlush(char * linea, int nroLinea, int tamanioPaquete, int socket);
char * obtenerLinea(int posicionMemoria);
void enviarLineaComoPaquetes(char * lineaAEnviar, int tamanioLinea, int tamanioPaquete, int cantidadPaquetes, int nroLinea, int socket);

int reservarSegmentoSegmentacionPaginada(t_gdt * gdt, int pid);
#endif /* FUNESMEMORY9_H_ */
