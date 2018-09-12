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
#include <configuracion.h>


pthread_attr_t tattr;


int main(int argc, char ** argv) {

	//TODO: CREAR EL ESTADO CORRUPTO Y OPERATIVO DEL SAFA

    //Creo las Variables locales
    pthread_t threadConsola;
    pthread_t threadConexiones;

    //inicializacion de recursos y carga de configuracion
    inicializarRecursos();

    //Inicializo la consola del Planificador y los threads correspondientes
    pthread_create(&threadConsola, &tattr, (void *) mainConsola, NULL);
    pthread_create(&threadConexiones, &tattr, (void *) manejarConexiones, NULL);

    while(1){

    }

    liberarRecursos();
	return EXIT_SUCCESS;
}


void inicializarRecursos(){

    logger = log_create_mutex("SAFA.log", "SAFA", 0, LOG_LEVEL_INFO);

    //Cargo el archivo configuracion
    conf = (configSAFA *) cargarConfiguracion("config", SAFA, logger->logger);

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
}


void liberarRecursos(){

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
//	pthread_mutex_init(&mutexExit, NULL);
//	pthread_mutex_init(&mutexStop, NULL);
//	pthread_mutex_init(&mutexReadyExecute, NULL);
//	pthread_mutex_init(&mutexConsole, NULL);
}



