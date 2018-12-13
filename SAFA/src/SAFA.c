/*
 ============================================================================
 Name        : SAFA.c
 Author      : Alejandro Walsh
 Version     : 1.0
 Copyright   :
 Description : Módulo SAFA para el TP de SO 2c 2018
 ============================================================================
 */

#include "SAFA.h"
#include <grantp/configuracion.h>

pthread_attr_t tattr;


int main(int argc, char ** argv) {

	//Creo las Variables locales
    pthread_t threadConsola;
    pthread_t threadConexiones;
//    pthread_t threadCambioConfig;
    pthread_t threadCortoPlazo;
    pthread_t threadLargoPlazo;

    /*inicializacion de recursos y carga de configuracion
     /home/utnso/git/tp-2018-2c-Los-5digos/SAFA/config"*/
    if (argc > 0)
    	inicializarRecursos(argv[1]);
    else
    {
    	log_error_mutex(logger, "Faltan parametros para inicializar el SAFA.");
    	return EXIT_FAILURE;
	}

    printf("Recursos incializados");

    //Empiezo inotif para despues ver si hubo cambios sobre el archivo de configuracion
	inotifyFd = inotify_init();
	inotifyWd = inotify_add_watch(inotifyFd,argv[1],IN_CLOSE_WRITE);

	printf("Se crearan los hilos de safa");

	//Inicializo la consola del Planificador y los threads correspondientes
    pthread_create(&threadConexiones, &tattr, (void *) manejarConexiones, NULL);
//    pthread_create(&threadCambioConfig, &tattr, (void *) cambiosConfig, NULL);
    pthread_create(&threadCortoPlazo, &tattr, (void *) manejoCortoPlazo, NULL);
    pthread_create(&threadLargoPlazo, &tattr, (void *) manejoLargoPlazo, NULL);
    pthread_create(&threadConsola, &tattr, (void *) mainConsola, NULL);

    printf("Hilos creados");
    while(!getExit()){
    }

    liberarRecursos();
	return EXIT_SUCCESS;
}


void inicializarRecursos(char * pathConfig){

    logger = log_create_mutex("SAFA.log", "SAFA", 0, LOG_LEVEL_INFO);
    if (strcmp(pathConfig,"")==0)
    {
        log_error_mutex(logger, "No existe el archivo indicado");
        exit(1);
    }
    //Cargo el archivo configuracion
    conf = (configSAFA *) cargarConfiguracion(pathConfig, SAFA, logger->logger);

    if(conf==NULL){
        log_error_mutex(logger, "No existe archivo configuracion");
        exit(1);
    }

    log_info_mutex(logger, "Puerto de Escucha: %d", conf->puerto);
    log_info_mutex(logger, "Algoritmo de Planificacion leido de configuracion: %d", conf->algoritmo);

    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);

    initList();
    initMutexs();
//    initSems();
    FD_ZERO(&master);
    FD_ZERO(&readset);

    shouldExit = 0;

}


void liberarRecursos(){
	//TODO: Ver que elemento es el que va a estar dentro de la lista, entiendo va a ser un DTB
//    list_destroy_and_destroy_elements(statusList, freeEsi);
//    list_destroy_and_destroy_elements(blockedKeys, freeBlockedKey);

    pthread_mutex_destroy(&mutexNewList);
    pthread_mutex_destroy(&mutexReadyList);
    pthread_mutex_destroy(&mutexBloqueadosList);
    pthread_mutex_destroy(&mutexEjecutandoList);
    pthread_mutex_destroy(&mutexExitList);

    pthread_mutex_destroy(&mutexMaster);
    pthread_mutex_destroy(&mutexReadset);

//    pthread_mutex_destroy(&mutexTime);
    pthread_mutex_destroy(&mutexExit);
    pthread_mutex_destroy(&mutexStop);
//    pthread_mutex_destroy(&mutexReadyExecute);
    pthread_mutex_destroy(&mutexConsole);
    pthread_mutex_destroy(&mutexgdtCounter);
    pthread_mutex_destroy(&mutexDummy);
//
//    sem_destroy(&sem_shouldScheduler);
//    sem_destroy(&sem_newEsi);
//    sem_destroy(&sem_shouldExecute);
//    sem_destroy(&sem_preemptive);
    sem_destroy(&semaforpGradoMultiprgramacion);
    sem_destroy(&mandadosPorConsola);
    pthread_mutex_destroy(&semDummy);
    pthread_mutex_destroy(&semCargadoEnMemoria);


    log_destroy_mutex(logger);
    freeConfig(conf, SAFA);
}


void initMutexs(){

	//Se inicializan todos los semaforos MUTEX a ser utilizados
	pthread_mutex_init(&mutexMaster, NULL);
	pthread_mutex_init(&mutexReadset, NULL);

	//Mutex para las listas
	pthread_mutex_init(&mutexNewList, NULL);
	pthread_mutex_init(&mutexReadyList, NULL);
	pthread_mutex_init(&mutexBloqueadosList, NULL);
	pthread_mutex_init(&mutexEjecutandoList, NULL);
	pthread_mutex_init(&mutexExitList, NULL);

	//	pthread_mutex_init(&mutexTime, NULL);
	pthread_mutex_init(&mutexExit, NULL);
	pthread_mutex_init(&mutexStop, NULL);
//	pthread_mutex_init(&mutexReadyExecute, NULL);
	pthread_mutex_init(&mutexConsole, NULL);
	pthread_mutex_init(&mutexgdtCounter, NULL);
	pthread_mutex_init(&mutexDummy, NULL);
	pthread_mutex_init(&semCargadoEnMemoria, NULL);

	pthread_mutex_init(&semDummy, NULL);
	sem_init(&semaforpGradoMultiprgramacion, 0 ,conf->grado_mp);
	sem_init(&mandadosPorConsola, 0, 0);
}


void initList() {
	colaNew = list_create();
	colaReady = list_create();
	colaReadyEspecial = list_create();
	colaBloqueados = list_create();
	colaEjecutando = list_create();
	colaExit = list_create();
	listaMetricasLP = list_create();
}

void cambiosConfig(){
	//TODO: Por ahora, si hay cambios me avisa. Tengo que ver como pausar la ejecucion
	// y mandar el resto de los avisos, tmb ver que cambios hubo en el doc
	read(inotifyFd,inotifyBuf,200);
	log_info_mutex(logger,"Archivo leido es: %s", ((struct inotify_event*)inotifyBuf)->name);
}



////Se inicializan todos los semaforos a ser utilizados
//void initSems() {
//
////    sem_init(&sem_shouldScheduler, 0, 1);
////    sem_init(&sem_newEsi, 0, 0);
////    sem_init(&sem_shouldExecute, 0, 0);
////    sem_init(&sem_preemptive, 0, 0);
//}


void manejoLargoPlazo() {
	planificadorLP();
}

void manejoCortoPlazo() {
	planificadorCP();
}


