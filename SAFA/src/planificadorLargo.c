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

	int gradoMulti = conf->grado_mp;
    log_info_mutex(logger, "Hilo Planificador Largo levantado. Grado de multiprogramacion: %d", gradoMulti);

    while(1){

    	//TODO: ANALIZAR COMO HACER CUANDO EL PLANI DE CORTO VAYA SACANDO COSAS DE READY
    	// QUIZA SE RESUELVE HACIENDO QUE EL CORTO EN VEZ DE SACAR, MUEVA EL PRIMERO AL FINAL ETC

        pthread_mutex_lock(&mutexNewList);
        pthread_mutex_lock(&mutexReadyList);

        //Si hay algo en new, si ready no tiene lo maximo, y si el dummy esta libre recien ahi podria mandar cosas
    	if((list_size(colaReady) < gradoMulti) && (list_size(colaNew) > 0) && (obtenerEstadoDummy() == 1)){

    		//Me fijo cual es el primer elemento de la lista, no lo saco, solo tengo los datos
    		t_dtb *primerDTB = list_get(colaNew,0);

    		//TODO: Tener en cuenta que el planificador corto o el diegote en algun momento va a tener que avisarme que estuvo ok y ahi
    		// voy a poder hacer el REMOVE de la cola, ME VA A LLEGAR POR EL LADO DE CONEXIONES CON CPU
    		planificadorCPdesbloquearDummy(primerDTB->idGDT,primerDTB->dirEscriptorio);

    	}
        pthread_mutex_unlock(&mutexNewList);
        pthread_mutex_unlock(&mutexReadyList);


    }

}
