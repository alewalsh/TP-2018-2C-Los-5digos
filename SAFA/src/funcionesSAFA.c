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

//TODO: HACER LOS FREE PARA CUANDO SE TENGAN QUE LIBTERAR LOS DTBS!!!!!

t_dtb *crearNuevoDTB(char *dirScript) {

	//TODO: Por ahora inicializo la tabla y lineas en nada. se va a cambiar?
	sumarGDTCounter();
	int aux = obtenerGDTCounter();
    t_dtb *newDTB = malloc(sizeof(t_dtb));
	newDTB->idGDT = aux;
	newDTB->dirEscriptorio = dirScript;
	newDTB->programCounter = 0;
	newDTB->flagInicializado = true;
	newDTB->tablaDirecciones = NULL;
	newDTB->cantidadLineas = 0;
	return newDTB;
}

int agregarDTBaNEW(t_dtb *dtb) {
    pthread_mutex_lock(&mutexNewList);
    int i = colaNew->elements_count;
    if (list_add(colaNew, dtb) != i) {
        log_info_mutex(logger, "no se pudo agregar el elemento a la lista");
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


t_dtb *crearDummyDTB() {
	//Se inicializa vacio, despues el PLP lo rellena cuando le pide al PCP
    t_dtb *newDTB = malloc(sizeof(t_dtb));
	newDTB->idGDT = 0;
	newDTB->dirEscriptorio = NULL;
	newDTB->programCounter = 0;
	newDTB->flagInicializado = false;
	newDTB->tablaDirecciones = NULL;
	newDTB->cantidadLineas = 0;
	return newDTB;
}

t_cpus *crearCpu() {
	//Se inicializa vacio, despues el PLP lo rellena cuando le pide al PCP
    t_cpus *cpu = malloc(sizeof(t_cpus));
	cpu->socket = 0;
	cpu->libre = 0;//0 = libre, 1= en uso
	return cpu;
}

void desbloquearDummy(){
    pthread_mutex_lock(&mutexDummy);
    dummyBloqueado = 0;
    pthread_mutex_unlock(&mutexDummy);
}

void bloquearDummy(){
    pthread_mutex_lock(&mutexDummy);
    dummyBloqueado = 1;
    pthread_mutex_unlock(&mutexDummy);
}

int obtenerEstadoDummy() {
    int aux = 0;
    pthread_mutex_lock(&mutexDummy);
    aux = dummyBloqueado;
    pthread_mutex_unlock(&mutexDummy);
    return aux;
}

t_dtb * transformarPaqueteADTB(t_package paquete)
{
	// Se realiza lo que sería una deserializacion de la info dentro de paquete->data
	t_dtb * dtb = malloc(sizeof(t_dtb));
	char *buffer = paquete.data;
	dtb->idGDT = copyIntFromBuffer(&buffer);
	dtb->dirEscriptorio = copyStringFromBuffer(&buffer);
	dtb->programCounter = copyIntFromBuffer(&buffer);
	dtb->flagInicializado = copyIntFromBuffer(&buffer);
	dtb->tablaDirecciones = copyStringFromBuffer(&buffer);
	dtb->cantidadLineas = copyIntFromBuffer(&buffer);
	return dtb;
}

t_package transformarDTBAPaquete(t_dtb * dtb)
{
	// Se realiza lo que sería una deserializacion de la info dentro de paquete->data
	t_package paquete;
	char *buffer;
	copyIntToBuffer(&buffer, dtb->idGDT);
	copyStringToBuffer(&buffer, dtb->dirEscriptorio);
	copyIntToBuffer(&buffer, dtb->programCounter);
	copyIntToBuffer(&buffer, dtb->flagInicializado);
	copyStringToBuffer(&buffer, dtb->tablaDirecciones);
	copyIntToBuffer(&buffer, dtb->cantidadLineas);
	paquete.data = buffer;
	paquete.size = 4*sizeof(int)+strlen(dtb->dirEscriptorio)+strlen(dtb->tablaDirecciones);
	return paquete;
}

int bloquearDTB(t_package pkg){
	//logica para bloquear dtb
	t_dtb * dtb = transformarPaqueteADTB(pkg);
	return EXIT_SUCCESS;
}

int abortarDTB(t_package pkg){
	//logica para abortar dtb
	t_dtb * dtb = transformarPaqueteADTB(pkg);
	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA .....
//------------------------------------------------------------------------------------------------------------------



