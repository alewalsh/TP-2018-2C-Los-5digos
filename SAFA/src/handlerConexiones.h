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
void hacerWaitDeRecurso(char * recursoSolicitado, int pid);
void hacerSignalDeRecurso(char * recursoSolicitado);
#endif /* HANDLERCONEXIONES_H_ */
