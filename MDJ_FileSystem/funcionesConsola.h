/*
 * funcionesConsola.h
 *
 *  Created on: 25 sep. 2018
 *      Author: utnso
 */

#ifndef FUNCIONESCONSOLA_H_
#define FUNCIONESCONSOLA_H_

#include <pthread.h>
#include <semaphore.h>

int shouldExit;
pthread_mutex_t mutexExit;


void consoleExit();

int getExit();
void setExit();
#endif /* FUNCIONESCONSOLA_H_ */
