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

int pasarDTBdeNEWaREADY(){

	t_dtb *dtb = list_remove(colaNew,0);
	list_add(colaReady, dtb);
	return 1;
}

void planificadorLP() {

	int gradoMulti = conf->grado_mp;
    log_info_mutex(logger, "Hilo Planificador Largo levantado. Grado de multiprogramacion: %d", gradoMulti);

    while(1){

    	//TODO: ANALIZAR COMO HACER CUANDO EL PLANI DE CORTO VAYA SACANDO COSAS DE READY
    	// QUIZA SE RESUELVE HACIENDO QUE EL CORTO EN VEZ DE SACAR, MUEVA EL PRIMERO AL FINAL ETC

    	//TODO: MISMO, QUIZA PUEDO HACER UN MUTEX QUE DIGA QUE EL PLANIFICADOR DE CORTO ESTA EJECUTANDO ENTONCES
    	// SE PUEDE CONFIRMAR SI ESTA BIEN LA COLA DE READY O NO

    	//TODO: OPCION 3, QUE EL PLANIFICADOR DE CORTO PLAZO MUTEXEE LA LISTA DE READY HASTA QUE TERMINA QUANTUM O ALGO ASI
    	// que libere la lista de ready solo cuadnto termino de ejecutar

        pthread_mutex_lock(&mutexNewList);
        pthread_mutex_lock(&mutexReadyList);
    	if((list_size(colaReady) < gradoMulti) && (list_size(colaNew) > 0)){
    	    //log_info_mutex(logger, "Debo agregar GDTs en ready");
    		if(pasarDTBdeNEWaREADY()){
        	    log_info_mutex(logger, "DTB pasado a ready");
    		}
    	}
        pthread_mutex_unlock(&mutexNewList);
        pthread_mutex_unlock(&mutexReadyList);


    }

}
