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
#include "grantp/parser.h"

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
extern int socketEscucha;

enum codConfirmSafaId{
	ARCHIVO_CREADO= 24000, //CREAR ARCHIVO
	ARCHIVO_BORRADO, //BORRAR ARCHIVO
	ARCHIVO_CARGADO,
	ARCHIVO_INICIALIZADO,//CARGAR SCRIPTORIO Y ABRIR ARCHIVO
	ARCHIVO_GUARDADO //FLUSH
};

int cantIODelProceso;
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
bool leerEscriptorio(t_package paquete, int socketEnUso);
bool abrirArchivo(t_package paquete, int socketEnUso);
bool hacerFlush(t_package paquete, int socketEnUso);
void enviarPaqueteAFm9(char * buffer);
int enviarPkgDeMdjAFm9(int pid, char * path, int size);
int enviarPkgDeFm9AMdj(char * path);
bool crearArchivo(t_package paquete, int socketEnUso);
bool borrarArchivo(t_package paquete, int socketEnUso);
int contarCantidadLineas(char * string);
int calcularCantidadPaquetes(int sizeOfFile);
void enviarConfirmacionSafa(int pid, int result, int cantidadIODelProceso, int code);
int confirmarExistenciaFile();
int esOperacionDeIO(t_cpu_operacion operacion);
#endif /* FUNCIONESDMA_H_ */
