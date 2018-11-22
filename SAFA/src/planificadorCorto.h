/*
 * planificadorCorto.h
 *
 *  Created on: 20 nov. 2018
 *      Author: utnso
 */

#ifndef PLANIFICADORCORTO_H_
#define PLANIFICADORCORTO_H_

#include "funcionesSAFA.h"
#include <grantp/configuracion.h>


extern configSAFA *conf;
extern t_list* colaNew;			// Lista New.
extern t_list* colaReady;			// Lista Ready.
extern t_list* colaBloqueados;		// Lista Bloqueados.
extern t_list* colaExit;		// Lista Terminados.

t_dtb *dummyDTB;


void planificadorCP();



#endif /* PLANIFICADORCORTO_H_ */
