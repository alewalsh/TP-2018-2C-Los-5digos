/*
 * funcionesSAFA.c
 *
 *  Created on: 11 sep. 2018
 *      Author: utnso
 */

#include "funcionesSAFA.h"
#include <grantp/structCommons.h>
#include <grantp/configuracion.h>


//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA EL MANEJO DEL SELECT
//------------------------------------------------------------------------------------------------------------------

void addNewSocketToMaster(int socket) {
    pthread_mutex_lock(&mutexMaster);
    FD_SET(socket, &master);
    log_trace_mutex(logger, "Se agrega el socket %d a la lista de sockets", socket);
    updateMaxfd(socket);
    pthread_mutex_unlock(&mutexMaster);
}

int getMaxfd() {
    int m;
    pthread_mutex_lock(&mutexMaxfd);
    m = maxfd;
    pthread_mutex_unlock(&mutexMaxfd);
    return m;
}

void updateMaxfd(int socket) {
    if (socket > getMaxfd()) {
        pthread_mutex_lock(&mutexMaxfd);
        maxfd = socket;
        pthread_mutex_unlock(&mutexMaxfd);
    }
}

void deleteSocketFromMaster(int socket) {
    pthread_mutex_lock(&mutexMaster);
    FD_CLR(socket, &master);
    log_trace_mutex(logger, "Se saca el socket %d de la lista de sockets", socket);
    pthread_mutex_unlock(&mutexMaster);
}

void updateReadset() {
    pthread_mutex_lock(&mutexMaster);
    pthread_mutex_lock(&mutexReadset);
    readset = master;
    pthread_mutex_unlock(&mutexReadset);
    pthread_mutex_unlock(&mutexMaster);
}

int isSetted(int socket) {
    int s;
    pthread_mutex_lock(&mutexReadset);
    s = FD_ISSET(socket, &readset);
    pthread_mutex_unlock(&mutexReadset);
    return s;
}


//======================================================================================================================================
//============================================FUNCIONES Consola=========================================================================
//======================================================================================================================================

void setPlay() {
    pthread_mutex_lock(&mutexConsole);
    console = 1;
    pthread_mutex_unlock(&mutexConsole);
}

void setPause() {
    pthread_mutex_lock(&mutexConsole);
    console = 0;
    pthread_mutex_unlock(&mutexConsole);
}

int getConsoleStatus() {
    int aux = 0;
    pthread_mutex_lock(&mutexConsole);
    aux = console;
    pthread_mutex_unlock(&mutexConsole);
    return aux;
}

void playExecute() {
    pthread_mutex_lock(&mutexConsole);
    scheduler = 1;
    pthread_mutex_unlock(&mutexConsole);
}

void stopExecute() {
    pthread_mutex_lock(&mutexConsole);
    scheduler = 0;
    pthread_mutex_unlock(&mutexConsole);
}

int getExecute() {
    int aux = 0;
    pthread_mutex_lock(&mutexConsole);
    aux = scheduler;
    pthread_mutex_unlock(&mutexConsole);
    return aux;
}



//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES GDT y DTB
//------------------------------------------------------------------------------------------------------------------

t_dtb *crearNuevoDTB(char *dirScript) {

	sumarGDTCounter();
	int aux = obtenerGDTCounter();

    t_dtb *newDTB = malloc(sizeof(t_dtb));
	newDTB->idGDT = aux;
	newDTB->dirEscriptorio = dirScript;
	newDTB->programCounter = aux;
	newDTB->flagInicializado = false;
//	newDTB->tablaDirecciones //char *tablaDirecciones;
//	newDTB->cantidadLineas //int cantidadLineas;
	return newDTB;
}

int agregarDTBaNEW(t_dtb *dtb) {
    pthread_mutex_lock(&mutexNewList);
    if (list_add(colaNew, dtb)) {
        pthread_mutex_unlock(&mutexNewList);
        return EXIT_FAILURE;
    }
    pthread_mutex_unlock(&mutexNewList);
    return EXIT_SUCCESS;
}

void sumarGDTCounter() {
    pthread_mutex_lock(&mutexgdtCounter);
    gdtCounter++;
    pthread_mutex_unlock(&mutexgdtCounter);
}

int obtenerGDTCounter() {
    int aux = 0;
    pthread_mutex_lock(&mutexgdtCounter);
    aux = gdtCounter;
    pthread_mutex_unlock(&mutexgdtCounter);
    return aux;
}

//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA .....
//------------------------------------------------------------------------------------------------------------------



