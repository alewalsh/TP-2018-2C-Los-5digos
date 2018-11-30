/*
 * planificadorLargo.c
 *
 *  Created on: 17 nov. 2018
 *      Author: utnso
 */

#include "planificadorLargo.h"

int consolaNuevoGDT(char* scriptIngresado){

	t_dtb *newDTB = crearNuevoDTB(scriptIngresado);
	if(agregarDTBaNEW(newDTB)){
		return EXIT_FAILURE;
	}
    return EXIT_SUCCESS;
}

void planificadorLP() {

    log_info_mutex(logger, "Hilo Planificador Largo levantado. Grado de multiprogramacion: %d", conf->grado_mp);

    while(1){
    	//este no estoy seguro.
    	sem_wait(&mandadosPorConsola);

    	sem_wait(&semaforpGradoMultiprgramacion);
        pthread_mutex_lock(&mutexNewList);
        pthread_mutex_lock(&mutexReadyList);

   		//Me fijo cual es el primer elemento de la lista, no lo saco, solo tengo los datos
   		t_dtb *primerDTB = list_get(colaNew,0);

   		pthread_mutex_lock(&semDummy);
  		planificadorCPdesbloquearDummy(primerDTB->idGDT,primerDTB->dirEscriptorio);

        pthread_mutex_unlock(&mutexNewList);
        pthread_mutex_unlock(&mutexReadyList);
    }

}
