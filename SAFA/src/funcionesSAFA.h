/*
 * funcionesSAFA.h
 *
 *  Created on: 11 sep. 2018
 *      Author: utnso
 */

#ifndef FUNCIONESSAFA_H_
#define FUNCIONESSAFA_H_
#include <grantp/structCommons.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <commons/string.h>
#include <grantp/configuracion.h>
#include <grantp/mutex_log.h>
#include "semaphore.h"
#include "stdbool.h"

//DTB DUMMY
t_dtb * dummyDTB;
extern pthread_mutex_t semCargadoEnMemoria;
extern sem_t semaforoGradoMultiprgramacion;
//Semaforo para el dummy
extern pthread_mutex_t semDummy;

extern t_list* colaNew;			// Lista New.
extern t_list* colaReady;			// Lista Ready.
extern t_list* colaBloqueados;		// Lista Bloqueados.
extern t_list* colaEjecutando;     // Lista DTBs en ejec
extern t_list* colaExit;		// Lista Terminados.

extern pthread_mutex_t mutexMaster;
extern pthread_mutex_t mutexReadset;
extern pthread_mutex_t mutexMaxfd;
extern pthread_mutex_t mutexStop;
extern pthread_mutex_t mutexConsole;
extern pthread_mutex_t mutexgdtCounter;
extern pthread_mutex_t mutexNewList;
extern pthread_mutex_t mutexReadyList;
extern pthread_mutex_t mutexBloqueadosList;
extern pthread_mutex_t mutexEjecutandoList;
extern pthread_mutex_t mutexExitList;
extern pthread_mutex_t mutexDummy;
extern pthread_mutex_t mutexReadyEspList;
extern pthread_mutex_t mutexCpus;

extern pthread_mutex_t mutexTotalSentencias;
extern pthread_mutex_t mutexSentenciasXDAM;

extern sem_t hayProcesosEnReady;
extern sem_t semaforoCpu;

extern int maxfd;
extern int console;
extern int scheduler;
extern int gdtCounter;

extern int totalSentenciasEjecutadas;
extern int sentenciasXDAM;

extern fd_set master;
extern fd_set readset;
extern configSAFA *conf;
extern t_log_mutex *logger;

extern int dummyBloqueado;
extern t_list* listaMetricasTRDefinitiva;
extern t_list* listaMetricasTR;


//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA EL MANEJO DEL SELECT
//------------------------------------------------------------------------------------------------------------------

void addNewSocketToMaster(int socket);
int getMaxfd();
void updateMaxfd(int socket);
void deleteSocketFromMaster(int socket);
void updateReadset();
int isSetted(int socket);

//======================================================================================================================================
//============================================FUNCIONES Consola=========================================================================
//======================================================================================================================================

void setPlay();
void liberarCpu(int socketCpu);
void setPause();
int getConsoleStatus();
void playExecute();
void stopExecute();
int getExecute();


//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES GDT y DTB
//------------------------------------------------------------------------------------------------------------------

t_dtb *crearNuevoDTB(char *dirScript);
int agregarDTBaNEW(t_dtb *dtb);
void sumarGDTCounter();
int obtenerGDTCounter();
t_dtb * transformarPaqueteADTB(t_package paquete);
t_package transformarDTBAPaquete(t_dtb * dtb);
int bloquearDTB(t_dtb * dtb);
int desbloquearDTB(t_dtb * dtb);
int abortarDTB(t_dtb * dtb, int socketCPU);
int finEjecucionPorQuantum(t_dtb * dtb);
int confirmacionDMA(int pid, int result);
void actualizarListaDefinitivaMetricaTR(int pid);
int abortarDTBNuevo(t_dtb * dtb);
void actualizarIODtb(t_dtb * dtb, int cantIo, int cantLineasProceso);
void actualizarTablaDirecciones(int pid, char * path);
void liberarCpu(int socketCpu);
//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA MANEJO DEL DUMMY
//------------------------------------------------------------------------------------------------------------------
t_dtb *crearDummyDTB();
void desbloquearDummy();
void bloquearDummy();
int obtenerEstadoDummy();
void setearFlagInicializacionDummy(int num);
void setearPathEnDummy(char * path);
int obtenerFlagDummy();
void planificadorCPdesbloquearDummy();
//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA PLANIFICADOR CP
//------------------------------------------------------------------------------------------------------------------
int buscarDTBEnCola(t_list * cola, t_dtb * dtbABuscar);
int buscarDTBEnColaMetricasNew(t_dtb * dtbAbuscar);
t_dtb * buscarDTBPorPIDenCola(t_list * cola, int pid);
int buscarPosicionPorPIDenCola(t_list * cola, int pid);
t_cpus *crearCpu();
int pasarDTBdeEXECaFINALIZADO(t_dtb * dtbABloq);
int pasarDTBdeBLOQUEADOaFINALIZADO(t_dtb * dtbABloq);
int desbloquearDTBsegunAlgoritmo(int pid);

//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA METRICAS
//------------------------------------------------------------------------------------------------------------------
void actualizarTotalSentenciasEjecutadas(int cantidadDeSentencias);
void actualizarSentenciasPasaronPorDAM(int cantidadDeSentencias);
void actualizarMetricaTiempoDeRespuesta(int cantidadDeSentencias);
//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA INOTIFY
//------------------------------------------------------------------------------------------------------------------
void notificarCambioQuantumCPUS(int nuevoQuantum);
void notificarCambioGradoMultiprogramacion(int viejoGradoMP, int nuevoGradoMP);

#endif /* FUNCIONESSAFA_H_ */
