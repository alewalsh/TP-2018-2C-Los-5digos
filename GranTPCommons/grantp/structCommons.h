/*
 * structCommons.h
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#ifndef STRUCTCOMMONS_H_
#define STRUCTCOMMONS_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "socket.h"
#include "compression.h"
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include "parser.h"

/*
 *  Ejemplo DTB: habria que ponerlo en GranTPCommons si utilizamos este.
 */
typedef struct {
	int idGDT;
    char *dirEscriptorio;
    int programCounter;
    int flagInicializado;
    char *tablaDirecciones;
    int cantidadLineas;
} t_dtb;


typedef struct{
	int pid;
	char * path;
	int transferSize;
} t_datosFlush;

typedef struct{
	int socket;
	int libre; //0 = libre, 1= en uso
} t_cpus;


typedef struct{
	int nroSegmento;
	int base;
	int limite;
	char * archivo;
} t_segmento;

typedef struct{
	int nroMarco;
	int nroPagina;
	int pid;
	char * path;
	int lineasUtilizadas;
	int nroSegmento;
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

typedef enum
{
	AccionDUMP = 1,
	AccionFLUSH
} accionFM9;

//typedef struct {
//    char *operation;
//    char *key;
//    char *value;
//} t_esi_instruction;
//
//typedef struct {
//    char *inputs;
//    char *size;
//} t_init_instance;
//
//typedef struct {
//    char *id;
//    int socket;
//    double estimate;
//    int real;
//    int time;
//    int shouldEstimate;
//    uint16_t status;
//} t_node_esi;
//
//typedef struct {
//    char *esiId;
//    char *key;
//    uint16_t status;
//} t_blocked_keys;

enum estadoSAFA {
	Operativo = 0, Corrupto = 1
};

enum typeLIST {
    READY = 2001, BLOCKED = 2002, EXECUTING = 2003, FINISHED = 2004, BLOCKEDKEYS
};

enum tipoPlanificacion {
    RR = 1, VRR, PROPIO
};

enum tipoEjecucion {
	SEG = 1, TPI, SPA
};

typedef struct {
    uint16_t type;
    int socket;
} t_plan_socket;

enum typeSocket {
    ESI_SOCKET, COORDINATOR_SOCKET
};

enum typeStatus {
    TAKEN = 3001, WAITING = 3002
};

enum command {
    EJECUTAR = 1, STATUS, FINALIZAR, METRICAS, EXIT, HELP, CLEAR
};
//LIST, STATUS, KILL, DEADLOCK, CLEAR, MAN, ESIS, KEYS

void freeEsiInstruction(void *esi);

void freeEsi(void *esi);

void freeBlockedKey(void *bk);

void liberarSegmento(t_segmento *self);

void liberarPagina(t_pagina * self);

void liberarOperacion(t_cpu_operacion * operacion);

t_infoGuardadoLinea * guardarDatosPaqueteGuardadoLinea(t_package pkg);

t_infoCargaEscriptorio * guardarDatosPaqueteCargaEscriptorio(t_package pkg);

t_infoCerrarArchivo * guardarDatosPaqueteCierreArchivo(t_package pkg);

t_datosFlush * guardarDatosPaqueteFlush(t_package pkg);

#endif /* COMANDOS_H_ */
