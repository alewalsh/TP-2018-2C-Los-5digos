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
#include <configuracion.h>
#include <socket.h>
#include <limits.h>

/*
 * Config
 */
t_log * logger;
configCPU * config;

/*
 * Variables globales
 */

/*
 * Sockets
 */
t_socket * t_socketDAM;
t_socket * t_socketSAFA;
int * socketDAM;
int * socketSAFA;

/*
 * Estructuras particulares CPU
 */

enum codigosError
{
	ERROR_SOCKET_DAM = INT_MIN,
	ERROR_SOCKET_SAFA,
	ERROR_PATH_CONFIG,
	ERROR_CONFIG,
	FIN_EXITOSO
};

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
 * @DESC: finaliza la ejecucion del programa, liberando los recursos que correspondan.
 * @PARAMS: {int} error Indica un codigo para verificar si finaliza la ejecucion por error o por fin de ejecucion.
 */
void exit_gracefully(int error);

#endif /* CPU_H_ */
