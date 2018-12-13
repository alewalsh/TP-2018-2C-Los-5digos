/*
 * funcionesConsola.c
 *
 *  Created on: 11 sep. 2018
 *      Author: utnso
 */


#include "funcionesConsola.h"
#include <grantp/javaStrings.h>
#include <grantp/structCommons.h>
#include <grantp/mutex_log.h>
#include <grantp/socket.h>
#include "planificadorLargo.h"
#include <grantp/compression.h>

// ----------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------

int cantComandos = 7;
const char *functions[] = {"EJECUTAR", "STATUS", "FINALIZAR", "METRICAS", "EXIT", "HELP", "CLEAR"};


const char *descriptions[] = {"Ejecutara el Script indicado.",
                              "Detallara los estados de las colas de planificacion.",
                              "Se enviara el proceso indicado a la cola de EXIT, liberando lugar en READY.",
                              "Se brindara informacion de las metricas solicitadas",
                              "SE VA A CERRAR LA CONSOLA!",
							  "I NEED SOMEBODY HELP, NOT JUST ANYBODY HELP, I NEED SOMEONE HEEEEELP!",
                              "Console Clear"};


void consolaEjecutar(char *args) {

    log_info_mutex(logger, "Se solicitara al PLP crear el DTB asociado al nuevo programa.");
    if(consolaNuevoGDT(args)){
        log_error_mutex(logger, "NO SE HA PODIDO CREAR EL DTB CORRESPONDIENTE.");
        return;
    }
	log_info_mutex(logger, "DTB creado satisfactoriamente y enviado a NEW.");
	sem_post(&mandadosPorConsola);
}


void consolaStatus() {

	//TODO: Separar en si tiene argumento o no tiene argumento. Hacer una funcion para printear cada cola
	pthread_mutex_lock(&mutexNewList);
	if(list_size(colaNew) != 0)
	{
		pthread_mutex_unlock(&mutexNewList);
		imprimirNEW();
	}
	else
	{
		pthread_mutex_unlock(&mutexNewList);
		printf("Cola NEW sin elementos\n");
	}
	pthread_mutex_lock(&mutexReadyList);
	if(list_size(colaReady) != 0)
	{
		pthread_mutex_unlock(&mutexReadyList);
		imprimirREADY();
	}
	else
	{
		pthread_mutex_unlock(&mutexReadyList);
		printf("Cola READY sin elementos\n");
	}
	pthread_mutex_lock(&mutexEjecutandoList);
	if(list_size(colaEjecutando) != 0)
	{
		pthread_mutex_unlock(&mutexEjecutandoList);
		imprimirEJECUTANDO();
	}
	else
	{
		pthread_mutex_unlock(&mutexEjecutandoList);
		printf("Cola EJECUTANDO sin elementos\n");
	}
}


void imprimirNEW(){
    pthread_mutex_lock(&mutexNewList);
	int size = list_size(colaNew);
    printf("*---------------------COLA NEW -----------------------*\n");
    int i;
    for (i = 0; i < size; i++) {
        t_dtb *dtb = (t_dtb *) list_get(colaNew, i);
        printf("DTB numero: (%d) \n", dtb->idGDT);
    }
    printf("*-----------------------------------------------------*\n\n");
    pthread_mutex_unlock(&mutexNewList);
}

void imprimirREADY(){
	pthread_mutex_lock(&mutexReadyList);
	int size = list_size(colaReady);
    printf("*---------------------COLA READY ---------------------*\n");
    int i;
    for (i = 0; i < size; i++) {
        t_dtb *dtb = (t_dtb *) list_get(colaReady, i);
        printf("DTB numero: (%d) \n", dtb->idGDT);
    }
    printf("*-----------------------------------------------------*\n\n");
	pthread_mutex_unlock(&mutexReadyList);
}

void imprimirEJECUTANDO(){
    pthread_mutex_lock(&mutexEjecutandoList);
	int size = list_size(colaEjecutando);
    printf("*---------------------COLA EJECUTANDO -----------------------*\n");
    int i;
    for (i = 0; i < size; i++) {
        t_dtb *dtb = (t_dtb *) list_get(colaEjecutando, i);
        printf("DTB numero: (%d) \n", dtb->idGDT);
    }
    printf("*-----------------------------------------------------*\n\n");
    pthread_mutex_unlock(&mutexEjecutandoList);
}


void consolaLiberar(){
	bloquearDummy();
}


void consolaMetricasDTB(char *args){

	int asd = consolaMetricaDTB(args);
    printf("Tiempo en NEW del DTB seleccionado: %d \n", asd);
    //TODO: Verificar/arreglar ya que rompe cuando se ejecuta metricas dtb sin que se haya hecho
    //    un ejecutar alguna vez
}

void consolaMetricas(){
    printf("Metricas sin parametros\n");
}

void consoleClear() {
    system("clear");
}

void consoleHelp() {
    printf("*--------------------------------------------------------------------------*\n\n");
    int i;
    for (i = 0; i < cantComandos; ++i) {
        printf("%s - %s \n", functions[i], descriptions[i]);
    }
    printf("*--------------------------------------------------------------------------*\n\n");
}

void consoleExit() {
    printf("Comando EXIT seleccionado.\n");
    printf("Se procedera a cerrar el programa.\n");
    pthread_exit(NULL);
}

int getIdFunction(char *function) {
    int i;
    for (i = 0; (i < cantComandos) && (strcmp(function, functions[i]) != 0); i++);
    return (i <= cantComandos - 1) ? (i + 1) : -1;
}

void parseCommand(char *line, char **command, char **args) {
    char **lines = string_n_split(line, 2, " ");
    string_trim(&lines[0]);
    *command = lines[0];
    string_to_upper(*command);
    *args = lines[1];
    free(lines);
}

void freeCommand(char *command, char *args) {
    if (command)
        free(command);
    if (args)
        free(args);
}

