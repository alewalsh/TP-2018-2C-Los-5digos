/*
 * funcionesSAFA.h
 *
 *  Created on: 11 sep. 2018
 *      Author: utnso
 */

#ifndef FUNCIONESSAFA_H_
#define FUNCIONESSAFA_H_

#include <structCommons.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <commons/string.h>
#include <configuracion.h>
#include <mutex_log.h>
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



#endif /* FUNCIONESSAFA_H_ */
