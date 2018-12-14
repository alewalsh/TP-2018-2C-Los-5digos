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

//	t_metricaLP *metricaDelDTB = list_get(listaMetricasLP, idSolicitado);
	t_metricaLP *metricaDelDTB = buscarMetricaPorPIDenCola(listaMetricasLP, idSolicitado);
	tiempo = metricaDelDTB->tiempoEnNEW;

    return tiempo;
}

t_metricaLP * buscarMetricaPorPIDenCola(t_list * cola, int pid){
	t_metricaLP *metricaDelDTB;
	int listSize = list_size(cola);
	if(listSize<= 0) return NULL;

	for(int i = 0; i<listSize;i++){
		metricaDelDTB = list_get(cola,i);
		if(metricaDelDTB->idDTB == pid){
			return metricaDelDTB;
			break;
		}
	}
	return metricaDelDTB;
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


/*
 * Se busca todos los elementos de la cola de NEW que tambien esten en la lista de metricas y se
 * actualiza el valor correspondiente en la ultima.
 *
 */
void actualizarMetricasDTBNew(int instruccionesEjecutadas){

		for(int i = 0; i < list_size(colaNew);i++){

			t_dtb * dtbNEW = list_get(colaNew,i);
			int posicion = buscarDTBEnCola(listaMetricasLP,dtbNEW);

			if(posicion >= 0){
				t_metricaLP *dtbEnMetrica = list_remove(listaMetricasLP, posicion);
				dtbEnMetrica->tiempoEnNEW = dtbEnMetrica->tiempoEnNEW + instruccionesEjecutadas;
				list_add(listaMetricasLP,dtbEnMetrica);
			}

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
