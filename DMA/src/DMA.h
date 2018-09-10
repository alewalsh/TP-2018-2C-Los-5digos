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

t_log* logger;

void sig_handler(int signo);
void exit_gracefully(int error);


#endif /* DMA_H_ */
