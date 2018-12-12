/*
 * CPU.h
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <grantp/configuracion.h>
#include <grantp/structCommons.h>
#include <grantp/compression.h>
#include <grantp/parser.h>
#include <grantp/socket.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>

/*
 * Config
 */
configCPU * config;

/*
 * Variables globales
 */
t_log_mutex * loggerCPU;
int quantum;
char * pathBuscado;
/*
 * Sockets
 */
t_socket * t_socketDAM;
t_socket * t_socketSAFA;
t_socket * t_socketFM9;
int * socketDAM;
int * socketSAFA;
int * socketFM9;

/*
 * Estructuras particulares CPU
 */

enum codigosError
{
	ERROR_SOCKET = INT_MIN,
	ERROR_PATH_CONFIG,
	ERROR_CONFIG,
	ERROR_DTB
};

enum accionEjecutada
{
	CONCENTRAR_EJECUTADO = 2,
	DTB_DESALOJADO
};

/*
 * Semaforos
 */
pthread_mutex_t mutexQuantum;
pthread_mutex_t mutexPath;

/*
 * Funciones
 */

/**
 * @NAME: inicializarCPU
 * @DESC: inicializa las configuraciones basicas del modulo CPU.
 * @PARAMS: {char *} pathConfig Indica el path donde se encuentra el archivo de configuración
 */
void inicializarCPU(char * pathConfig);

/**
 * @NAME: inicializarConexiones
 * @DESC: inicializa los sockets y realiza los handshakes correspondientes.
 * @PARAMS: -
 */
void inicializarConexiones();

/**
 * @NAME: exit_gracefully
 * @DESC: finaliza la ejecucion del program, liberando los recursos que correspondan.
 * @PARAMS: {int} error Indica un codigo para verificar si finaliza la ejecucion por error o por fin de ejecucion.
 */
void exit_gracefully(int error);

/**
 * @NAME: enumToProcess
 * @DESC: devuelve el nombre del proceso en base al Handshake recibido.
 * @PARAMS: {int} proceso Indica el proceso del cual se recibio el handshake.
 */
char * enumToProcess(int proceso);

t_socket * conectarseAProceso(int puerto, char *ip, int * socket, int handshakeProceso);
void recibirDTB();

void manejarSolicitud(t_package pkg, int socketFD);
int nuevoDummy(t_dtb * dtb, t_package paquete);

int comenzarEjecucion(t_package paquete);
int ejecutarOperacion(t_cpu_operacion * operacion, t_dtb ** dtb);
int enviarAModulo(t_cpu_operacion * operacion, t_dtb ** dtb, int accion, int modulo);
int manejarRecursosSAFA(char * recurso, int idGDT, int accion);
int setQuantum(t_package paquete);

t_dtb * transformarPaqueteADTB(t_package paquete);
t_package transformarDTBAPaquete(t_dtb * dtb);

void liberarMemoriaTSocket(t_socket * TSocket);

int realizarEjecucion(t_dtb * dtb);

int finalizoEjecucionDTB(t_dtb * dtb, int code);

int eventoSAFA(t_dtb ** dtb, int code);

int ejecucionDAM(t_dtb ** dtb);

int ejecucionFM9(t_dtb ** dtb, int socket);
int finEjecucionFM9(int idGDT);

t_cpu_operacion obtenerInstruccionMemoria(char * direccionEscriptorio, int idGDT, int posicion);
bool encontrarPath(char * direccion);
#endif /* CPU_H_ */
