/*
 * planificadorCorto.c
 *
 *  Created on: 20 nov. 2018
 *      Author: utnso
 */

#include "planificadorCorto.h"




void planificadorCP() {

	dummyDTB = (t_dtb *) crearDummyDTB();

    switch (conf->algoritmo) {
        case RR:
            log_info_mutex(logger, "Algoritmo Round Robin");
//            planificarRR();
            break;
        case VRR:
        	log_info_mutex(logger, "Algoritmo Virtual Round Robin");
//            planificarVRR();
            break;
        default:
        	log_info_mutex(logger, "Algoritmo Propio");
//            playExecute();
//            planificarPropio();
            break;
    }




}
