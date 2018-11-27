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
//#include "handlerCoordinator.h"
//#include "handlerEsi.h"


extern configSAFA *conf;
extern t_log_mutex *logger;
extern fd_set master;
extern fd_set readset;
//extern sem_t sem_newEsi;
//extern pthread_mutex_t mutexReadyExecute;


// ------------------------------------------------------------------------------
//	VARIABLES GLOBALES
// ------------------------------------------------------------------------------
int estadoSAFA;

void manejarConexiones();

#endif /* HANDLERCONEXIONES_H_ */
