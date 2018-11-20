/*
 * planificadorLargo.h
 *
 *  Created on: 17 nov. 2018
 *      Author: utnso
 */

#ifndef PLANIFICADORLARGO_H_
#define PLANIFICADORLARGO_H_

#include "funcionesSAFA.h"
#include <grantp/configuracion.h>


extern configSAFA *conf;
extern t_list* colaNew;			// Lista New.
extern t_list* colaReady;			// Lista Ready.


int consolaNuevoGDT(char* scriptIngresado);
void planificadorLP();

#endif /* PLANIFICADORLARGO_H_ */
