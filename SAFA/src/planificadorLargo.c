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



	return 1;
}

void planificadorLP() {
    log_info_mutex(logger, "Hilo Planificador Largo levantado");

    int gradoMulti = conf->grado_mp;

    while(1){

    	if(list_size(colaReady) < gradoMulti){
    	    //log_info_mutex(logger, "Debo agregar GDTs en ready");

//    		pasarDTBdeNEWaREADY();


    	}

    }

}
