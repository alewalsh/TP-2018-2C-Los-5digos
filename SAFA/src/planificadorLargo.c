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
	printf("(%d)",newDTB->idGDT);
	printf("(%d)",newDTB->programCounter);
	printf("(%s)",newDTB->tablaDirecciones);
    return EXIT_SUCCESS;
}
