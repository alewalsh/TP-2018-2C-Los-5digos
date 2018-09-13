/*
 * DMA.h
 *
 *  Created on: 9 sep. 2018
 *      Author: utnso
 */

#ifndef DMA_H_
#define DMA_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <commons/log.h>
#include <pthread.h>
#include <socket.h>
#include <configuracion.h>

t_log* logger;
configDAM* configDMA;


//Declaracion de funciones
void cargarArchivoDeConfig();
void configure_logger();
void inicializarDMA();
int  connect_to_server(char * ip, char * port, char * servidor);
void sig_handler(int signo);
void exit_gracefully(int error);


#endif /* DMA_H_ */
