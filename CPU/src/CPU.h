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
#include <grantp/configuracion.h>
#include <grantp/compression.h>
#include <grantp/parser.h>
#include <grantp/socket.h>
#include <limits.h>
//#include <semaphore.h>

/*
 *  Ejemplo DTB: habria que ponerlo en GranTPCommons si utilizamos este.
 */

typedef struct {
	int idGDT;
    char *dirEscriptorio;
    int programCounter;
    bool flagInicializado;
    char *tablaDirecciones;
} t_dtb;

/*
 * Config
 */
configCPU * config;

/*
 * Variables globales
 */
t_log_mutex * loggerCPU;
int quantum;

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

/*
 * Semaforos
 */
//sem_t * sem_nuevoDummy;
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


char * enumToProcess(int proceso);

t_socket *  conectarseAProceso(int puerto, char *ip, int * socket, int handshakeProceso);

void manejarSolicitud(t_package pkg, int socketFD);
int nuevoDummy(t_package paquete);
int comenzarEjecucion(t_package paquete);
int setQuantum(t_package paquete);

void recibirDTB();
t_dtb * transformarPaqueteADTB(t_package paquete);
void gestionarSolicitud(t_package paquete);

#endif /* CPU_H_ */
