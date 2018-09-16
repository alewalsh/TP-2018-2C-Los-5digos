/*
 * SAFA.h
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#ifndef SAFA_H_
#define SAFA_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <commons/log.h>
#include <pthread.h>
#include <grantp/socket.h>
#include "handlerConexiones.h"

t_log* logger;

void sig_handler(int signo);
void exit_gracefully(int error);

#endif /* SAFA_H_ */
