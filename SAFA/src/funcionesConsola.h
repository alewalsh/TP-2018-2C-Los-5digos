/*
 * funcionesConsola.h
 *
 *  Created on: 11 sep. 2018
 *      Author: utnso
 */

#ifndef FUNCIONESCONSOLA_H_
#define FUNCIONESCONSOLA_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "funcionesSAFA.h"
#include <semaphore.h>
//#include "handlerConnections.h"


extern int shouldExit;
extern pthread_mutex_t mutexExit;

//extern pthread_mutex_t mutexReadyExecute;
//
//extern sem_t sem_newEsi;
//extern int coordinador;
//


//void consoleStop();

void consolaEjecutar(char *args);

//void consoleBlock(char *args);
//
//void consoleUnblock(char *esi);
//
//void consoleList(char *key);
//
//void consoleKill(char *id);
//
//void consoleStatus(char *key);
//
//void consoleDeadlock();
//
//void consolePrintMan();
//

void consoleExit();

void consoleHelp();

//void consoleStatusAllESI();

void consoleClear();

//char *statusToString(uint16_t status);
//
//char *statusKeyToSting(uint16_t status);
//
//void consoleStatusKeys();
//
//void detectDeadlock(int e, int k, int need[e][k], int alloc[e][k], int avail[k], t_list *esis);
//

int getIdFunction(char *function);
void parseCommand(char *line, char **command, char **args);
void freeCommand(char *command, char *args);

//--------------------------------------------------------------------------------------------------------------------------------------
// FUNCIONES PARA LOS HILOS
//--------------------------------------------------------------------------------------------------------------------------------------
int getExit();
void setExit();



#endif /* FUNCIONESCONSOLA_H_ */
