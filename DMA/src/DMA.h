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

t_socket* socketSafa;
t_socket* socketMdj;
t_socket* socketFm9;

t_log* logger;
configDAM* configDMA;

//Hilos
pthread_attr_t tattr;
pthread_t hiloSafa;
pthread_t hiloMdj;
pthread_t hiloFm9;


//Declaracion de funciones
void cargarArchivoDeConfig();
void configure_logger();
void iniciarHilosDelDMA();
void sig_handler(int signo);
void * conectarseConSafa();
void *conectarseConMdj();
void *conectarseConFm9();
void exit_gracefully(int error);
bool cerrarSockets();

#endif /* DMA_H_ */
