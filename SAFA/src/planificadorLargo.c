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

    	//TODO: AGREGAR UN SEMAFORO QUE ME MANDE EL AVISO PARA CHEQUEAR ESTO
    	if((list_size(colaReady) < gradoMulti) && (list_size(colaNew) > 0)){
    	    //log_info_mutex(logger, "Debo agregar GDTs en ready");
    		if(pasarDTBdeNEWaREADY()){
        	    log_info_mutex(logger, "DTB pasado a ready");
    		}
    	}

    }

}
