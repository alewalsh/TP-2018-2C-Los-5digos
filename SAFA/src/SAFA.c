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


//pthread_attr_t tattr;


int main(void) {

//	t_socket * socket = inicializarTSocket(1, logger);

    //Creo las Variables locales
//    pthread_t threadConsola;

    //inicializacion de recursos y carga de configuracion
    inicializarRecursos();

    //Inicializo la consola del Planificador y los threads correspondientes
//    pthread_create(&threadConsola, &tattr, (void *) mainConsola, NULL);
    mainConsola();

    while(1){

    }

    liberarRecursos();
	return EXIT_SUCCESS;
}


void inicializarRecursos(){

    logger = log_create_mutex("SAFA.log", "SAFA", 0, LOG_LEVEL_INFO);

    //Cargo el archivo configuracion
    conf = (configSAFA *) cargarConfiguracion(
            "config", SAFA, logger->logger);

    log_info_mutex(logger, "Algoritmo de Planificacion: %d", conf->algoritmo);

//    pthread_attr_init(&tattr);
//    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
//    initList();
//    initMutexs();
//    initSems();
//    FD_ZERO(&readset);
//    FD_ZERO(&master);
}


void liberarRecursos(){

    log_destroy_mutex(logger);
    freeConfig(conf, SAFA);
}
