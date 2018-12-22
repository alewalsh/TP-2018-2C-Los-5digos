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
	agregarDTBaMetricasTR(newDTB->idGDT);
    return EXIT_SUCCESS;
}

int consolaMetricaDTBEnNew(int idSolicitado){
	int tiempo;
	pthread_mutex_lock(&mutexMetricasLP);
	t_metricaLP *metricaDelDTB = buscarMetricaLPPorPIDenCola(listaMetricasLP, idSolicitado);
	pthread_mutex_unlock(&mutexMetricasLP);
	tiempo = metricaDelDTB->tiempoEnNEW;

    return tiempo;
}

t_metricaLP * buscarMetricaLPPorPIDenCola(t_list * cola, int pid){
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
	pthread_mutex_lock(&mutexMetricasLP);
	list_add(listaMetricasLP, metrica);
	pthread_mutex_unlock(&mutexMetricasLP);
}

void agregarDTBaMetricasTR(int id){
	t_metricaTR *metrica = nuevaMetricaTR(id);
	list_add(listaMetricasTR, metrica);
}

t_metricaLP *nuevaMetrica(int id) {
	t_metricaLP *nuevaMetrica = malloc(sizeof(t_metricaLP));
	nuevaMetrica->idDTB = id;
	nuevaMetrica->tiempoEnNEW = 0;
	return nuevaMetrica;
}

t_metricaTR *nuevaMetricaTR(int id) {
	t_metricaTR *nuevaMetrica = malloc(sizeof(t_metricaTR));
	nuevaMetrica->idDTB = id;
	nuevaMetrica->tiempoDeRespuesta = 0;
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
			int posicion = buscarDTBEnColaMetricasNew(dtbNEW);

			if(posicion >= 0){
				pthread_mutex_lock(&mutexMetricasLP);
				t_metricaLP *dtbEnMetrica = list_remove(listaMetricasLP, posicion);
				dtbEnMetrica->tiempoEnNEW = dtbEnMetrica->tiempoEnNEW + instruccionesEjecutadas;
				list_add(listaMetricasLP,dtbEnMetrica);
				pthread_mutex_unlock(&mutexMetricasLP);
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

        log_info_mutex(logger, "Hilo Planificador Largo.  Wait al semaforo de grado de multiprogramacion.");
    	sem_wait(&semaforoGradoMultiprgramacion);
//        pthread_mutex_lock(&mutexReadyList);
        log_info_mutex(logger, "Hilo Planificador Largo.  Lock al semaforo del dummy.");
    	pthread_mutex_lock(&semDummy);
   		//Me fijo cual es el primer elemento de la lista, no lo saco, solo tengo los datos
        log_info_mutex(logger, "Hilo Planificador Largo.  Lock al semaforo de la lista de NEW.");
        pthread_mutex_lock(&mutexNewList);
        t_dtb *primerDTB;
        for(int i = 0; i < list_size(colaNew); i++){
        	primerDTB = list_get(colaNew,i);
        	if(primerDTB->realizOpDummy == 0){
                log_info_mutex(logger, "Hilo Planificador Largo.  DTB %d seleccionado.", primerDTB->idGDT);
        		break;
        	}
        }
        log_info_mutex(logger, "Hilo Planificador Largo.  Unlock al semaforo de la lista de NEW.");
        pthread_mutex_unlock(&mutexNewList);

  		planificadorCPdesbloquearDummy(primerDTB->idGDT,primerDTB->dirEscriptorio);
//        pthread_mutex_unlock(&mutexReadyList);
    }

}
