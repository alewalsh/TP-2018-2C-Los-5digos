/*
 * handlerConexiones.h
 *
 *  Created on: 2 sep. 2018
 *      Author: utnso
 */

#ifndef HANDLERCONEXIONES_H_
#define HANDLERCONEXIONES_H_

#include <stdint.h>
#include <grantp/socket.h>
#include <grantp/configuracion.h>
#include "funcionesSAFA.h"
#include <semaphore.h>

extern configSAFA *conf;
extern t_log_mutex *logger;
extern fd_set master;
extern fd_set readset;
extern pthread_mutex_t mutexCpus;
extern pthread_mutex_t mutexRecursoBuscado;

char * recursoBuscado;
int socketBuscado;

t_list* listaCpus;
t_list* listaRecursoAsignados;
// ------------------------------------------------------------------------------
//	VARIABLES GLOBALES
// ------------------------------------------------------------------------------
int estadoSAFA;
void liberarRecursos();
void manejarConexiones();
void manejarSolicitud(t_package pkg, int socketFD);
void initCpuList();
t_recurso* crearRecurso(char * recurso, int pid);
void hacerWaitDeRecurso(char * recursoSolicitado, int pid, int socketCPU, int PCActual);
void hacerSignalDeRecurso(char * recursoSolicitado);
bool existeRecurso(t_recurso * recurso);
void actualizarMetricas(t_dtb * dtb);
void manejarNuevaCPU(int nuevoFd);
void finEjecucionCPU(int socket);
bool buscarCPUPorSocket(t_cpus * cpu);
void exit_gracefully(int error);
void notificarDesconexionCPUs();
#endif /* HANDLERCONEXIONES_H_ */
