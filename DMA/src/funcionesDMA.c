/*
 * funcionesDMA.c
 *
 *  Created on: 11 sep. 2018
 *      Author: utnso
 */

#include "funcionesDMA.h"
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



//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA .....
//------------------------------------------------------------------------------------------------------------------



