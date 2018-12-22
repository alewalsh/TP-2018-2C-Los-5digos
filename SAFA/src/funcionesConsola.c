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
		imprimirNEW();
	}
	else
	{
		printf("Cola NEW sin elementos\n");
	}

	pthread_mutex_unlock(&mutexNewList);

	pthread_mutex_lock(&mutexReadyList);

	if(list_size(colaReady) != 0)
	{
		imprimirREADY();
	}
	else
	{
		printf("Cola READY sin elementos\n");
	}

	pthread_mutex_unlock(&mutexReadyList);

	pthread_mutex_lock(&mutexEjecutandoList);

	if(list_size(colaEjecutando) != 0)
	{
		imprimirEJECUTANDO();
	}
	else
	{
		printf("Cola EJECUTANDO sin elementos\n");
	}
	pthread_mutex_unlock(&mutexEjecutandoList);

	pthread_mutex_lock(&mutexBloqueadosList);

	if(list_size(colaBloqueados) != 0)
	{
		imprimirBLOQUEADOS();
	}
	else
	{
		printf("Cola BLOQUEADOS sin elementos\n");
	}

	pthread_mutex_unlock(&mutexBloqueadosList);

	pthread_mutex_lock(&mutexExitList);

	if(list_size(colaExit) != 0)
	{
		imprimirEXIT();
	}
	else
	{
		printf("Cola EXIT sin elementos\n");
	}
	pthread_mutex_unlock(&mutexExitList);

	pthread_mutex_lock(&mutexReadyEspList);

	if(list_size(colaReadyEspecial) != 0)
	{
		imprimirREADYESPECIAL();
	}
	else
	{
		printf("Cola READY ESPECIAL sin elementos\n");
	}

	pthread_mutex_unlock(&mutexReadyEspList);
}

void imprimirNEW(){
    //pthread_mutex_lock(&mutexNewList);
	int size = list_size(colaNew);
    printf("\n*---------------------COLA NEW -----------------------*\n");
    int i;
    for (i = 0; i < size; i++) {
        t_dtb *dtb = (t_dtb *) list_get(colaNew, i);
        printf("DTB numero: (%d) \n", dtb->idGDT);
    }
    printf("*-----------------------------------------------------*\n\n");
   // pthread_mutex_unlock(&mutexNewList);
}

void imprimirREADY(){
	//pthread_mutex_lock(&mutexReadyList);
	int size = list_size(colaReady);
    printf("\n*---------------------COLA READY ---------------------*\n");
    int i;
    for (i = 0; i < size; i++) {
        t_dtb *dtb = (t_dtb *) list_get(colaReady, i);
        if(dtb->esDummy){
        	printf("DTB numero: (%d) <- Dummy \n ", dtb->idGDT);
        }else{
            printf("DTB numero: (%d) \n", dtb->idGDT);
        }
    }
    printf("*-----------------------------------------------------*\n\n");
	//pthread_mutex_unlock(&mutexReadyList);
}

void imprimirEJECUTANDO(){
    //pthread_mutex_lock(&mutexEjecutandoList);
	int size = list_size(colaEjecutando);
    printf("\n*---------------------COLA EJECUTANDO -----------------------*\n");
    int i;
    for (i = 0; i < size; i++) {
        t_dtb *dtb = (t_dtb *) list_get(colaEjecutando, i);
        if(dtb->esDummy){
        	printf("DTB numero: (%d) <- Dummy \n ", dtb->idGDT);
		}else{
			printf("DTB numero: (%d) \n", dtb->idGDT);
		}
    }
    printf("*-----------------------------------------------------*\n\n");
    //pthread_mutex_unlock(&mutexEjecutandoList);
}

void imprimirBLOQUEADOS(){
    //pthread_mutex_lock(&mutexBloqueadosList);
	int size = list_size(colaBloqueados);
    printf("\n*---------------------COLA BLOQUEADOS -----------------------*\n");
    int i;
    for (i = 0; i < size; i++) {
        t_dtb *dtb = (t_dtb *) list_get(colaBloqueados, i);
        if(dtb->esDummy){
		  printf("DTB numero: (%d) <- Dummy \n", dtb->idGDT);
	  }else{
		  printf("DTB numero: (%d) \n", dtb->idGDT);
	  }
    }
    printf("*-----------------------------------------------------*\n\n");
    //pthread_mutex_unlock(&mutexBloqueadosList);
}

void imprimirEXIT(){
    //pthread_mutex_lock(&mutexExitList);
	int size = list_size(colaExit);
    printf("\n*---------------------COLA EXIT -----------------------*\n");
    int i;
    for (i = 0; i < size; i++) {
        t_dtb *dtb = (t_dtb *) list_get(colaExit, i);
        printf("DTB numero: (%d) \n", dtb->idGDT);
    }
    printf("*-----------------------------------------------------*\n\n");
    //pthread_mutex_unlock(&mutexExitList);
}

void imprimirREADYESPECIAL(){
    //pthread_mutex_lock(&mutexReadyEspList);
	int size = list_size(colaReadyEspecial);
    printf("\n*---------------------COLA READY ESPECIAL -----------------------*\n");
    int i;
    for (i = 0; i < size; i++) {
        t_dtb *dtb = (t_dtb *) list_get(colaReadyEspecial, i);
        printf("DTB numero: (%d) \n", dtb->idGDT);
    }
    printf("*-----------------------------------------------------*\n\n");
    //pthread_mutex_unlock(&mutexReadyEspList);
}

void consolaStatusDTB(char *args){

	int idSolicitado = atoi(args);
	t_dtb * DTBBuscado;

	// Me fijo en que lista esta y lo mando a imprimit. No hace falta usar mutex de listas
	if(buscarPosicionPorPIDenCola(colaNew, idSolicitado) != -1){
		DTBBuscado = buscarDTBPorPIDenCola(colaNew, idSolicitado);
		imprimirDTB(DTBBuscado);
	}
	if(buscarPosicionPorPIDenCola(colaReady, idSolicitado) != -1){
		DTBBuscado = buscarDTBPorPIDenCola(colaReady, idSolicitado);
		imprimirDTB(DTBBuscado);
	}
	if(buscarPosicionPorPIDenCola(colaBloqueados, idSolicitado) != -1){
		DTBBuscado = buscarDTBPorPIDenCola(colaBloqueados, idSolicitado);
		imprimirDTB(DTBBuscado);
	}
	if(buscarPosicionPorPIDenCola(colaEjecutando, idSolicitado) != -1){
		DTBBuscado = buscarDTBPorPIDenCola(colaEjecutando, idSolicitado);
		imprimirDTB(DTBBuscado);
	}
	if(buscarPosicionPorPIDenCola(colaExit, idSolicitado) != -1){
		DTBBuscado = buscarDTBPorPIDenCola(colaExit, idSolicitado);
		imprimirDTB(DTBBuscado);
	}
	if(buscarPosicionPorPIDenCola(colaReadyEspecial, idSolicitado) != -1){
		DTBBuscado = buscarDTBPorPIDenCola(colaReadyEspecial, idSolicitado);
		imprimirDTB(DTBBuscado);
	}
}

void imprimirDTB(t_dtb * dtb){

    printf("\n*--------------------- DTB --------------------------*\n");
    printf("ID: %d \n", dtb->idGDT);
    printf("Direccion Script: %s \n", dtb->dirEscriptorio );
    printf("Program Counter: %d \n", dtb->programCounter);
    printf("Flag Inicializado: %d \n", dtb->flagInicializado);
    printf("Realizo Op Dummy: %d \n", dtb->realizOpDummy );
    if (dtb->tablaDirecciones != NULL){
        int tam;
    	tam = list_size(dtb->tablaDirecciones);
    	if (tam > 0){
    	    printf("Tabla de direcciones: ---------------------\n");
    	    int i;
    	    for (i = 0; i < tam; i++) {
    	        char *dir = (char *) list_get(dtb->tablaDirecciones, i);
    	        printf("%d - %s \n",i, dir);
    	    }
    		printf("--------------------------------------\n");
    	} else {
    	    printf("Tabla de direcciones vacia \n");
    	}
    }
    printf("Cantidad de Lineas: %d \n", dtb->cantidadLineas );
    printf("Quantum Restante: %d \n", dtb->quantumRestante );
    printf("Cantidad de I/O: %d \n", dtb->cantIO);
    printf("Dummy: %d \n",dtb->esDummy);
    printf("*-----------------------------------------------------*\n\n");
}

void consolaLiberar(){
	bloquearDummy();
}


void consolaMetricasDTB(char *args){
	int idSolicitado = atoi(args);
    //TODO: Verificar/arreglar ya que rompe cuando se ejecuta metricas dtb sin que se haya hecho
    //    un ejecutar alguna vez
	printf("*--------------------------------------------------------------------------*\n");
	printf("*-------------------------METRICAS PARA EL DTB: %d-------------------------* \n",idSolicitado);
	printf("*--------------------------------------------------------------------------*\n\n");

	printf("*--------------------------------------------------------------------------*\n");
	printf("1. Cant. de sentencias ejecutadas que esperó un DTB en la cola NEW\n\n");
	int tiempo = consolaMetricaDTBEnNew(idSolicitado);
	printf("Tiempo en NEW del DTB %d: %d \n", idSolicitado, tiempo);
	printf("\n*--------------------------------------------------------------------------*\n\n");

//	imprimirSentenciasDAM(); //METRICA 2
//	imprimirSentenciasPromEnExit(); //METRICA 3
//	imprimirPromedioSentenciasDam(); //METRICA 4
//	imprimirMetricaTiempoDeRespuestaPromedio(); //METRICA 5
}



void consolaMetricas(){

	printf("*--------------------------------------------------------------------------*\n");
	printf("*-------------------------------METRICAS-----------------------------------* \n");
	printf("*--------------------------------------------------------------------------*\n\n");
	imprimirDtbsEnNew(); //METRICA 1
	imprimirSentenciasDAM(); //METRICA 2
	imprimirSentenciasPromEnExit(); //METRICA 3
	imprimirPromedioSentenciasDam(); //METRICA 4
	imprimirMetricaTiempoDeRespuestaPromedio(); //METRICA 5
}

/**
 * METRICA 1
 */
void imprimirDtbsEnNew(){

	printf("*--------------------------------------------------------------------------*\n");
	printf("1. Cant. de sentencias ejecutadas que esperó un DTB en la cola NEW\n\n");
	pthread_mutex_lock(&mutexMetricasLP);
	for(int i = 0; i<list_size(listaMetricasLP); i++){
		t_metricaLP * dtb = list_get(listaMetricasLP,i);
		printf("DTB Id:%d -> %d | ",dtb->idDTB,dtb->tiempoEnNEW);
	}
	pthread_mutex_unlock(&mutexMetricasLP);
	printf("\n*--------------------------------------------------------------------------*\n\n");
}

/**
 * METRICA 2
 */
void imprimirSentenciasDAM(){

    pthread_mutex_lock(&mutexTotalSentencias);
    pthread_mutex_lock(&mutexSentenciasXDAM);
    int totSentenciasINT = totalSentenciasEjecutadas;
    int sentenciasPorDMAINT = sentenciasXDAM;
    pthread_mutex_unlock(&mutexTotalSentencias);
    pthread_mutex_unlock(&mutexSentenciasXDAM);

    float totSentenciasFloat = totSentenciasINT;
    float sentenciasPorDMAFloat = sentenciasPorDMAINT;
	float prom = sentenciasPorDMAFloat/totSentenciasFloat;

    printf("*--------------------------------------------------------------------------*\n");
    printf("2. Cant.de sentencias ejecutadas prom. del sistema que usaron a “El Diego” \n\n");
    printf("Cantidad de Total de Sentencias Ejecutadas: %d \n", totSentenciasINT);
    printf("Cantidad de Sentencias que pasaron por “El diego”: %d \n", sentenciasPorDMAINT);
    printf("Cantidad promedio de sentencias que pasaron por “El diego”: %f \n", prom);
    printf("*--------------------------------------------------------------------------*\n\n");
}

/**
 * METRICA 3
 */
void imprimirSentenciasPromEnExit(){
	int sentenciasPromEnExit = 0;
	int cantidadDTBEnExit = list_size(colaExit);
	pthread_mutex_lock(&mutexTotalSentencias);
	int totSentenciasINT = totalSentenciasEjecutadas;
	pthread_mutex_unlock(&mutexTotalSentencias);

	float cantSentencias = totSentenciasINT;
	float cantDtbEnExit = cantidadDTBEnExit;

	sentenciasPromEnExit = cantSentencias / cantDtbEnExit;
	printf("*--------------------------------------------------------------------------*\n");
	printf("3. Cant. de sentencias ejecutadas prom. del sistema para que un DTB termine en la cola EXIT\n\n");
	printf("La cantidad total de sentencias en la cola de EXIT es de: %d\n",totSentenciasINT);
	printf("La cantidad de DTBs en la cola de EXIT es de: %d\n", cantidadDTBEnExit);
	printf("La cantidad de sentencias promedio de la cola de EXIT es de: %d\n",sentenciasPromEnExit);
	printf("*--------------------------------------------------------------------------*\n\n");

}

/**
 * METRICA 4
 */
void imprimirPromedioSentenciasDam(){
	pthread_mutex_lock(&mutexTotalSentencias);
	pthread_mutex_lock(&mutexSentenciasXDAM);
	int totSentenciasINT = totalSentenciasEjecutadas;
	int sentenciasPorDMAINT = sentenciasXDAM;
	float totSentenciasFloat = totSentenciasINT;
	float sentenciasPorDMAFloat = sentenciasPorDMAINT;
	pthread_mutex_unlock(&mutexTotalSentencias);
	pthread_mutex_unlock(&mutexSentenciasXDAM);
	float prom = sentenciasPorDMAFloat/totSentenciasFloat;
	int porcentaje = (prom*100);

	printf("*--------------------------------------------------------------------------*\n");
	printf("4. Porcentaje de las sentencias ejecutadas promedio que fueron a “El Diego” \n\n");
	printf("Cantidad de Total de Sentencias Ejecutadas: %d \n", totSentenciasINT);
	printf("Cantidad promedio de sentencias que pasaron por “El diego”: %d \n", sentenciasPorDMAINT);
	printf("Porcentaje de sentencias ejecutadas promedio que pasaron por “El diego”: %d%s \n", porcentaje,"%");
	printf("*--------------------------------------------------------------------------*\n\n");
}

/**
 * METRICA 5
 */
void imprimirMetricaTiempoDeRespuestaPromedio(){

	int tiempoTotal = 0;
	int sizeMetricasTR = list_size(listaMetricasTRDefinitiva);
	for(int i = 0; i<list_size(listaMetricasTRDefinitiva); i++){
		t_metricaTR * dtb = list_get(listaMetricasTRDefinitiva,i);
		tiempoTotal += dtb->tiempoDeRespuesta;
	}
	float tiempoTotalFloat = tiempoTotal;
	float sizeMetricasTRFloat = sizeMetricasTR;
	float promedioTiempoDeRespuesta = tiempoTotalFloat/sizeMetricasTRFloat;
	printf("*--------------------------------------------------------------------------*\n");
	printf("5. Tiempo de Respuesta promedio del Sistema\n\n");
	printf("La cantidad de respuestas que hubo: %d\n", sizeMetricasTR);
	printf("El tiempo total de respuesta del sistema: %d\n", tiempoTotal);
	printf("El tiempo de respuesta promedio del sistema: %f\n",promedioTiempoDeRespuesta);
	printf("*--------------------------------------------------------------------------*\n\n");
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

