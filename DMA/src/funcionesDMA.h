/*
 * funcionesDMA.h
 *
 *  Created on: 11 sep. 2018
 *      Author: utnso
 */

#ifndef FUNCIONESDMA_H_
#define FUNCIONESDMA_H_
#include <grantp/structCommons.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <commons/string.h>
#include <grantp/configuracion.h>
#include <grantp/socket.h>
#include <grantp/compression.h>
#include <grantp/split.h>
#include <grantp/mutex_log.h>
#include "stdbool.h"


extern t_list *statusList;
extern pthread_mutex_t mutexStatusList;

extern pthread_mutex_t mutexMaster;
extern pthread_mutex_t mutexReadset;
extern pthread_mutex_t mutexMaxfd;
//extern pthread_mutex_t mutexTime;
//extern pthread_mutex_t mutexStop;
//extern pthread_mutex_t mutexConsole;
//extern int tiempo;
extern int maxfd;
//extern int console;
//extern int scheduler;
extern fd_set master;
extern fd_set readset;
extern configSAFA *conf;
extern t_log_mutex *logger;




//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA EL MANEJO DEL SELECT
//------------------------------------------------------------------------------------------------------------------

void addNewSocketToMaster(int socket);
int getMaxfd();
void updateMaxfd(int socket);
void deleteSocketFromMaster(int socket);
void updateReadset();
int isSetted(int socket);

//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES DEL DMA
//------------------------------------------------------------------------------------------------------------------

int recibirConfirmacionMemoria();
void enviarMsjASafaPidCargado(int pid,int itsLoaded);
void enviarMsjASafaArchivoGuardado(int pid,int result);
int leerEscriptorio(t_package paquete, int socketEnUso);
int abrirArchivo(t_package paquete, int socketEnUso);
int hacerFlush(t_package paquete, int socketEnUso);
void enviarPaqueteAFm9(char * buffer);
int enviarPkgDeMdjAFm9(int pid, char * path);
int enviarPkgDeFm9AMdj(int pid);
int crearArchivo(t_package paquete, int socketEnUso);
int borrarArchivo(t_package paquete, int socketEnUso);
void enviarMsjASafaArchivoCreado(int pid,int itsCreated, char * path);
void enviarMsjASafaArchivoBorrado(int pid,int itsDeleted, char * path);
int contarCantidadLineas(char * string);
int calcularCantidadPaquetes(int sizeOfFile);
#endif /* FUNCIONESDMA_H_ */
