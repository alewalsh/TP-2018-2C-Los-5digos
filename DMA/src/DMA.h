/*
 * DMA.h
 *
 *  Created on: 9 sep. 2018
 *      Author: Franco Lopez
 */

#ifndef DMA_H_
#define DMA_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <commons/log.h>
#include <pthread.h>
#include <grantp/socket.h>
#include <grantp/configuracion.h>

t_socket* t_socketSafa;
t_socket* t_socketMdj;
t_socket* t_socketFm9;

int * socketSafa;
int * socketMdj;
int * socketFm9;

t_log* logger;
configDAM* configDMA;

//Hilos
pthread_attr_t tattr;
pthread_t hiloSafa;
pthread_t hiloMdj;
pthread_t hiloFm9;

//Errores
enum codigosError
{
	ERROR_PATH_CONFIG = 3,
	ERROR_CONFIG = 4,
	ERROR_SOCKET = 5
};

//Declaracion de funciones
void cargarArchivoDeConfig();
void configure_logger();
void iniciarHilosDelDMA();
void sig_handler(int signo);
void conectAndHandskahe(int puerto, char *ip, int * socket, int handshakeProceso, t_socket* TSocket);
char * enumToProcess(int proceso);
void * conectarseConSafa();
void *conectarseConMdj();
void *conectarseConFm9();
void exit_gracefully(int error);
bool cerrarSockets();

#endif /* DMA_H_ */
