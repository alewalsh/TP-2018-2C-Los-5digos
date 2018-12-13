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
	agregarDTBaMetricasLP(newDTB->idGDT);
    return EXIT_SUCCESS;
}

int consolaMetricaDTB(char* dtbSolicitado){
	int tiempo;
	int idSolicitado = atoi(dtbSolicitado);
	t_metricaLP *metricaDelDTB = list_get(listaMetricasLP, idSolicitado);
	tiempo = metricaDelDTB->tiempoEnNEW;

//	log_info_mutex(logger, "Tiempo traido antes de actualizar: %d", tiempo);
//    sumarQuantumEjecutadoMetricaNEW();
//	t_metricaLP *metricaDelDTB2 = list_get(listaMetricasLP, idSolicitado);
//	int tiempo2 = metricaDelDTB2->tiempoEnNEW;
//    log_info_mutex(logger, "Tiempo traido desp de actualizar: %d", tiempo2);

    return tiempo;
}

void agregarDTBaMetricasLP(int id){
	t_metricaLP *metrica = nuevaMetrica(id);
	list_add(listaMetricasLP, metrica);
}

t_metricaLP *nuevaMetrica(int id) {
	t_metricaLP *nuevaMetrica = malloc(sizeof(t_metricaLP));
	nuevaMetrica->idDTB = id;
	nuevaMetrica->tiempoEnNEW = 0;
	return nuevaMetrica;
}

void sumarQuantumEjecutadoMetricaNEW(){

	// Tengo que buscar todos los dtbs que esten en NEW, y a esos buscarlos en la cola de Metricas. Solo
	// los que coincidan, deben sumarse el quantum esperado
    for (int i = 0; i < list_size(colaNew); i++) {
    	t_dtb * dtbEnNEW  = list_get(colaNew, i);
    	int idDTBenNEW = dtbEnNEW->idGDT;

    	t_metricaLP *dtbEnMetrica = list_get(listaMetricasLP, idDTBenNEW);
    	dtbEnMetrica->tiempoEnNEW = dtbEnMetrica->tiempoEnNEW + conf->quantum;
    }
}

void planificadorLP() {

    log_info_mutex(logger, "Hilo Planificador Largo levantado. Grado de multiprogramacion: %d", conf->grado_mp);
    agregarDTBaMetricasLP(0); //Creo el elemento 0 para encontrar mas facil los ids buscados

    //TODO: crear el free de esta lista y los elementos de adentro

    while(1){
    	//este no estoy seguro.
    	sem_wait(&mandadosPorConsola);

    	sem_wait(&semaforpGradoMultiprgramacion);
//        pthread_mutex_lock(&mutexReadyList);

   		//Me fijo cual es el primer elemento de la lista, no lo saco, solo tengo los datos
        pthread_mutex_lock(&mutexNewList);
   		t_dtb *primerDTB = list_get(colaNew,0);
        pthread_mutex_unlock(&mutexNewList);

   		pthread_mutex_lock(&semDummy);
  		planificadorCPdesbloquearDummy(primerDTB->idGDT,primerDTB->dirEscriptorio);

//        pthread_mutex_unlock(&mutexReadyList);
    }

}
