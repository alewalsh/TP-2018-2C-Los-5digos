/*
 * planificadorCorto.c
 *
 *  Created on: 20 nov. 2018
 *      Author: utnso
 */

#include "planificadorCorto.h"

//    pthread_mutex_destroy(&mutexNewList);
//	pthread_mutex_init(&mutexMaster, NULL);

void planificadorCP() {

	log_info_mutex(logger, "Hilo planificador Corto Plazo iniciado");
	dummyDTB = (t_dtb *) crearDummyDTB();
	bloquearDummy();

	pthread_mutex_init(&mutexPlanificando, NULL);
	int planificando = 0; // 0=No planificando  1=planificando

    while(1){
    	//TODO: Esto va a tener que estar ejecutando siempre que haya elementos en la cola de READY!!!!

    	if( (list_size(colaReady) > 0) && (planificando == 0) ){

            switch (conf->algoritmo) {
                case RR:
                    log_info_mutex(logger, "PCP mediante Round Robin");
                    pthread_mutex_lock(&mutexPlanificando);
                    planificando=1;
                    ejecutarRR();
                    pthread_mutex_unlock(&mutexPlanificando);
                    break;
                case VRR:
                	log_info_mutex(logger, "PCP mediante Virtual Round Robin");
        //            planificarVRR();
                    break;
                default:
                	log_info_mutex(logger, "PCP mediante Propio");
        //            playExecute();
                    break;
            }


    	}
    }
}

void planificadorCPdesbloquearDummy(int idGDT, char *dirScript){

	//Recibo la solicitud y desbloqueo el dummy. Lo pongo en READY
	// No hace falta verificar si ready tiene lugar porque lo mutexeo el de Largo Plazo
	desbloquearDummy();
	dummyDTB->idGDT = idGDT;
	dummyDTB->dirEscriptorio = dirScript;

	list_add(colaReady, dummyDTB);
}



void ejecutarRR(){

	pasarDTBaEjecutando();

}

void pasarDTBaEjecutando(){

    pthread_mutex_lock(&mutexReadyList);
    pthread_mutex_lock(&mutexEjecutandoList);
    t_dtb *primerDTBenReady = (t_dtb *) list_remove(colaReady,0);
    list_add(colaEjecutando, primerDTBenReady);
    pthread_mutex_unlock(&mutexEjecutandoList);
    pthread_mutex_unlock(&mutexReadyList);
}








