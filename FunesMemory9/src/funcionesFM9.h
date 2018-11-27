/*
 * funcionesFM9.h
 *
 *  Created on: 22 sep. 2018
 *      Author: utnso
 */

#ifndef FUNCIONESFM9_H_
#define FUNCIONESFM9_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <commons/string.h>
#include <grantp/configuracion.h>
#include <grantp/mutex_log.h>
#include <stdbool.h>

extern pthread_mutex_t mutexMaster;
extern pthread_mutex_t mutexReadset;
extern pthread_mutex_t mutexMaxfd;
extern int maxfd;
extern fd_set master;
extern fd_set readset;
extern configSAFA *conf;
extern t_log_mutex *logger;

void updateReadset();
int isSetted(int socket);
void addNewSocketToMaster(int socket);
int getMaxfd();
void updateMaxfd(int socket);
void deleteSocketFromMaster(int socket);

#endif /* SRC_FUNCIONESFM9_H_ */
