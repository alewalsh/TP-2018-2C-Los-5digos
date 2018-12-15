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
#include <pthread.h>
#include <semaphore.h>
#include <commons/collections/list.h>


typedef struct {
	int idDTB;
    int tiempoEnNEW;
} t_metricaLP;

extern configSAFA *conf;
extern t_list* colaNew;			// Lista New.
extern t_list* colaReady;			// Lista Ready.
extern t_list* listaMetricasLP;
extern sem_t mandadosPorConsola;
extern sem_t semaforoGradoMultiprgramacion;


int consolaNuevoGDT(char*);
t_metricaLP *nuevaMetrica(int id);
void agregarDTBaMetricasLP(int id);
int consolaMetricaDTB(char* dtbSolicitado);
void actualizarMetricasDTBNew(int instruccionesEjecutadas);
t_metricaLP * buscarMetricaPorPIDenCola(t_list * cola, int pid);

void planificadorLP();

#endif /* PLANIFICADORLARGO_H_ */
