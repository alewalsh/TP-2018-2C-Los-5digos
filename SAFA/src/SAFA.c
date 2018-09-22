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

	//TODO: CREAR EL ESTADO CORRUPTO Y OPERATIVO DEL SAFA

    configFilePath = "/home/utnso/git/tp-2018-2c-Los-5digos/SAFA/config";

	//Creo las Variables locales
    pthread_t threadConsola;
    pthread_t threadConexiones;
    pthread_t threadCambioConfig;

    //inicializacion de recursos y carga de configuracion
    inicializarRecursos();


    //Empiezo inotif para despues ver si hubo cambios sobre el archivo de configuracion
	inotifyFd = inotify_init();
	inotifyWd = inotify_add_watch(inotifyFd,configFilePath,IN_CLOSE_WRITE);

    //Inicializo la consola del Planificador y los threads correspondientes
    pthread_create(&threadConexiones, &tattr, (void *) manejarConexiones, NULL);
    pthread_create(&threadCambioConfig, &tattr, (void *) cambiosConfig, NULL);
    pthread_create(&threadConsola, &tattr, (void *) mainConsola, NULL);

    while(!getExit()){
    }

    liberarRecursos();
	return EXIT_SUCCESS;
}


void inicializarRecursos(){

    logger = log_create_mutex("SAFA.log", "SAFA", 0, LOG_LEVEL_INFO);

    //Cargo el archivo configuracion
    conf = (configSAFA *) cargarConfiguracion(configFilePath, SAFA, logger->logger);

    if(conf==NULL){
        log_error_mutex(logger, "No existe archivo configuracion");
        exit(1);
    }

    log_info_mutex(logger, "Algoritmo de Planificacion: %d", conf->algoritmo);

    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);

//    initList();
    initMutexs();
//    initSems();
    FD_ZERO(&master);
    FD_ZERO(&readset);

    shouldExit = 0;

}


void liberarRecursos(){


//    list_destroy_and_destroy_elements(statusList, freeEsi);
//    list_destroy_and_destroy_elements(blockedKeys, freeBlockedKey);
//
//    pthread_mutex_destroy(&mutexStatusList);
//    pthread_mutex_destroy(&mutexBlockedKeys);
    pthread_mutex_destroy(&mutexMaster);
    pthread_mutex_destroy(&mutexReadset);
//    pthread_mutex_destroy(&mutexTime);
    pthread_mutex_destroy(&mutexExit);
//    pthread_mutex_destroy(&mutexStop);
//    pthread_mutex_destroy(&mutexReadyExecute);
//    pthread_mutex_destroy(&mutexConsole);
//
//    sem_destroy(&sem_shouldScheduler);
//    sem_destroy(&sem_newEsi);
//    sem_destroy(&sem_shouldExecute);
//    sem_destroy(&sem_preemptive);

    log_destroy_mutex(logger);
    freeConfig(conf, SAFA);
}


void initMutexs(){
	//Se inicializan todos los semaforos MUTEX a ser utilizados
	pthread_mutex_init(&mutexMaster, NULL);
//	pthread_mutex_init(&mutexStatusList, NULL);
//	pthread_mutex_init(&mutexBlockedKeys, NULL);
	pthread_mutex_init(&mutexReadset, NULL);
//	pthread_mutex_init(&mutexTime, NULL);

	pthread_mutex_init(&mutexExit, NULL);

//	pthread_mutex_init(&mutexStop, NULL);
//	pthread_mutex_init(&mutexReadyExecute, NULL);
//	pthread_mutex_init(&mutexConsole, NULL);
}



void cambiosConfig(){
	//TODO: Por ahora, si hay cambios me avisa. Tengo que ver como pausar la ejecucion
	// y mandar el resto de los avisos, tmb ver que cambios hubo en el doc
	read(inotifyFd,inotifyBuf,200);
	log_info_mutex(logger,"Archivo leido es: %s", ((struct inotify_event*)inotifyBuf)->name);
}


////Se inicializan las listas de Estado y Claves Bloqueadas
//void initList() {
//    statusList = list_create();
//    blockedKeys = list_create();
//}
//
////Se inicializan todos los semaforos a ser utilizados
//void initSems() {
//    sem_init(&sem_shouldScheduler, 0, 1);
//    sem_init(&sem_newEsi, 0, 0);
//    sem_init(&sem_shouldExecute, 0, 0);
//    sem_init(&sem_preemptive, 0, 0);
//}

