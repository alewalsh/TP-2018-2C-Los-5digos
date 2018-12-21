/*
 * DMA.h
 *
 *  Created on: 9 sep. 2018
 *      Author: Franco Lopez
 */

#ifndef DMA_H_
#define DMA_H_

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <signal.h>
#include <grantp/mutex_log.h>
#include <pthread.h>
#include <grantp/socket.h>
#include <grantp/compression.h>
#include <grantp/configuracion.h>
#include "funcionesDMA.h"

int cpusConectadas;
t_package pkg;
uint16_t handshake;
int nuevoFd;

t_socket* t_socketSafa;
t_socket* t_socketMdj;
t_socket* t_socketFm9;
t_socket* t_socketEscucha;

fd_set master;
fd_set readset;
int maxfd;

pthread_mutex_t mutexMaster;
pthread_mutex_t mutexReadset;
pthread_mutex_t mutexMaxfd;
pthread_mutex_t mutexExit;
pthread_mutex_t mutexSolicitudes;

int * socketSafa;
int * socketMdj;
int * socketFm9;

int socketEscucha;
//t_log* logger;
t_log_mutex * logger;
configDAM* configDMA;

//Hilos
pthread_attr_t tattr;
pthread_t hiloSafa;
pthread_t hiloMdj;
pthread_t hiloFm9;
pthread_t hiloCPU;


//Errores
enum codigosError
{
	ERROR_PATH_CONFIG = 3,
	ERROR_CONFIG = 4,
	ERROR_SOCKET = 5
};

//Declaracion de funciones
void aceptarConexionesDelCpu();
void initMutex();
void initVariables();
void cargarArchivoDeConfig(char * pathConfig);
void configure_logger();
void iniciarConexionesDelDMA();
void sig_handler(int signo);
t_socket * conectarYenviarHandshake(int puerto, char *ip, int * socket, int handshakeProceso, t_socket* TSocket);
void conectarYRecibirHandshake(int puertoEscucha);
char * enumToProcess(int proceso);
int conectarseConSafa();
int conectarseConMdj();
int conectarseConFm9();
int conectarseConCPU();
void cargarEscriptorio();
void exit_gracefully(int error);
bool cerrarSockets();
void manejarSolicitudDelCPU(t_package pkg, int socketFD);
void finalizarEjecucionPID(t_package package);
#endif /* DMA_H_ */
