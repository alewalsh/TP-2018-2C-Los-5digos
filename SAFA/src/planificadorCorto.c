/*
 * planificadorCorto.c
 *
 *  Created on: 20 nov. 2018
 *      Author: utnso
 */

#include "planificadorCorto.h"

void planificadorCP() {

	log_info_mutex(logger, "Hilo planificador Corto Plazo iniciado");
	dummyDTB = (t_dtb *) crearDummyDTB();

	pthread_t threadDummy;
	pthread_t threadDispatcher;

	pthread_mutex_init(&mutexPlanificando, NULL);

	pthread_create(&threadDummy, NULL, (void *) manejarDummy, NULL);
	pthread_create(&threadDispatcher, NULL, (void *) manejarDispatcher, NULL);

	pthread_join(threadDummy,NULL);
	pthread_join(threadDispatcher,NULL);

}

/**
 * FUNCIONALIDAD 1:
 * Si hay un proceso en new y tiene el flag de inicializacion en 1:
 *      -> Debo desbloquear el dtbdummy y agregarlo al a lista de ready
 */
void manejarDummy() {
	while (1) {
		sem_wait(&desbloquearDTBDummy);

		int index = buscarDtbParaInicializar();
		if (index >= 0) {
			//Se desbloquea el dummy y se agrega a la lista de ready
//			pthread_mutex_lock(&mutexDummy);
			desbloquearDummy();
			//Se hace un signal para avisar que hay procesos en ready para ejecutar
			sem_post(&hayProcesosEnReady);
//			pthread_mutex_unlock(&mutexDummy);
		}
	}

}

/* FUNCIONALIDAD 2:
 * Si hay procesos en la cola de ready y hay cpus libres
 * 		->Se manda a ejecutar s/ el algoritmo
 */
void manejarDispatcher() {
	while (1) {
		sem_wait(&hayProcesosEnReady);
		sem_wait(&semaforoCpu);
		usleep(conf->retardo * 1000);

		int socketCPU = buscarCPULibre();
//		log_info_mutex(logger, "CPU donde fue %d", socketCPU);
		if (socketCPU > 0) {
			switch (conf->algoritmo) {
			case RR:
				log_info_mutex(logger, "PCP mediante Round Robin");
				pthread_mutex_lock(&mutexPlanificando);
				ejecutarRR(socketCPU);
				pthread_mutex_unlock(&mutexPlanificando);
				break;
			case VRR:
				log_info_mutex(logger, "PCP mediante Virtual Round Robin");
				pthread_mutex_lock(&mutexPlanificando);
				ejecutarVRR(socketCPU);
				pthread_mutex_unlock(&mutexPlanificando);
				break;
			default:
				log_info_mutex(logger, "PCP mediante Propio");
				pthread_mutex_lock(&mutexPlanificando);
				ejecutarIOBF(socketCPU);
				pthread_mutex_unlock(&mutexPlanificando);
				break;

			}
		}

	}
}

int buscarDtbParaInicializar()
{
	pthread_mutex_lock(&mutexNewList);
	int sizeList = list_size(colaNew);

	if(sizeList <= 0) return -1;

	for(int i = 0; i<sizeList;i++){
		t_dtb * dtb = list_get(colaNew,i);
		if(dtb->flagInicializado == 1 && dtb->realizOpDummy == 0){
			t_dtb * dtbActualizado = list_remove(colaNew,i);
			dtbActualizado->realizOpDummy = 1;
			list_add_in_index(colaNew,i,dtbActualizado);
			pthread_mutex_unlock(&mutexNewList);
			return i;
		}
	}
	pthread_mutex_unlock(&mutexNewList);
	return -1;
}

void planificadorCPdesbloquearDummy(int idGDT, char * dirScript)
{
	// Me fijo cual es el primer elemento de la lista, no lo saco, solo tengo los datos
	pthread_mutex_lock(&mutexBloqueadosList);
	t_dtb * dummy = list_remove_by_condition(colaBloqueados, (void *) obtenerDummy);
	dummy->idGDT = idGDT;
	dummy->dirEscriptorio = dirScript;
	list_add(colaBloqueados, dummy);
	pthread_mutex_unlock(&mutexBloqueadosList);
	//Recibo la solicitud y desbloqueo el dummy.
	sem_post(&desbloquearDTBDummy);
	dummyBloqueado = 1;
}

bool obtenerDummy(t_dtb * dtb)
{
	if (dtb->esDummy)
		return true;
	return false;
}

void ejecutarRR(int socketCpu){

	//Se pasa el primer proceso de los Ready a CPU a Ejecutar
	//Se cambia de cola
	t_dtb *dtb;
    pthread_mutex_lock(&mutexReadyEspList);

	if(list_size(colaReadyEspecial) > 0){
		pthread_mutex_unlock(&mutexReadyEspList);
		//contemplo el caso de que se haya cambiado de algoritmo en medio de la ejecucion y
		//haya quedado algun proceso en la cola de ready especial
		dtb = pasarDTBdeREADYESPaEXEC();
	}else{
		pthread_mutex_unlock(&mutexReadyEspList);
		pthread_mutex_lock(&mutexReadyList);
		if(list_size(colaReady)>0){
			pthread_mutex_unlock(&mutexReadyList);
			dtb = pasarDTBdeREADYaEXEC();
		}else{
			pthread_mutex_unlock(&mutexReadyList);
		}
	}
	//Se envía a cpu
	enviarDTBaCPU(dtb,socketCpu);
}

void ejecutarVRR(int socketCPU){
	t_dtb *dtb ;

    pthread_mutex_lock(&mutexReadyEspList);
	if(list_size(colaReadyEspecial) > 0)
	{
		pthread_mutex_unlock(&mutexReadyEspList);
		dtb= pasarDTBdeREADYESPaEXEC();
	}
	else
	{
		pthread_mutex_unlock(&mutexReadyEspList);
		dtb = pasarDTBdeREADYaEXEC();
	}

	enviarDTBaCPU(dtb,socketCPU);

}

void ejecutarIOBF(int socketCPU){

	pthread_mutex_lock(&mutexReadyList);
	if(list_size(colaReady) > 0){
		list_sort(colaReady,(void *) procesoConMayorCantIO);
	}
	pthread_mutex_unlock(&mutexReadyList);

//	for(int i=0; i < list_size(colaReady);i++){
//		dtb = list_get(colaReady,i);
//		log_info_mutex(logger,"El proceso %d tiene %d operaciones de I/O \n",dtb->idGDT, dtb->cantIO);
//	}
	pthread_mutex_lock(&mutexReadyList);
	t_dtb * dtb = list_get(colaReady,0);
	pthread_mutex_unlock(&mutexReadyList);

	pasarDTBdeREADYaEXEC(dtb);
	log_info_mutex(logger,"Se va a enviar el proceso %d al CPU",dtb->idGDT);
	enviarDTBaCPU(dtb,socketCPU);

}

bool procesoConMayorCantIO(t_dtb * p1, t_dtb * p2){
	return (p1->cantIO >= p2->cantIO);
}
/*
 * FUNCION PARA PASAR EL PRIMER PROCESO DE LA COLA READY A EJECUTAR
 * return: (t_dtb) dtb que se pasó de cola de ready para mandar a ejecutar
 */
t_dtb *pasarDTBdeREADYaEXEC(){

    pthread_mutex_lock(&mutexReadyList);
    t_dtb *primerDTBenReady = (t_dtb *) list_remove(colaReady,0);
    pthread_mutex_unlock(&mutexReadyList);

    pthread_mutex_lock(&mutexEjecutandoList);
    list_add(colaEjecutando, primerDTBenReady);
    pthread_mutex_unlock(&mutexEjecutandoList);
    return primerDTBenReady;
}

/*
 * FUNCION PARA PASAR EL PRIMER PROCESO DE LA COLA READY ESPECIAL A EJECUTAR
 * return: (t_dtb) dtb que se pasó de la cola de ready especial para mandar a ejecutar
 */
t_dtb * pasarDTBdeREADYESPaEXEC(){

	pthread_mutex_lock(&mutexReadyEspList);
	t_dtb *primerDTBenReadyEsp = (t_dtb *) list_remove(colaReadyEspecial,0);
	pthread_mutex_unlock(&mutexReadyEspList);

	pthread_mutex_lock(&mutexEjecutandoList);
	list_add(colaEjecutando, primerDTBenReadyEsp);
	pthread_mutex_unlock(&mutexEjecutandoList);
	return primerDTBenReadyEsp;
}
int pasarDTBdeEXECaREADY(t_dtb * dtbABloq){

	pthread_mutex_lock(&mutexEjecutandoList);
	int index = buscarDTBEnCola(colaEjecutando,dtbABloq);
	pthread_mutex_unlock(&mutexEjecutandoList);

	if(index >= 0){
		pthread_mutex_lock(&mutexEjecutandoList);
		t_dtb * dtbEjecutandoABloquear = (t_dtb *) list_remove(colaEjecutando,index);
		pthread_mutex_unlock(&mutexEjecutandoList);
		dtbEjecutandoABloquear->programCounter = dtbABloq->programCounter;
		pthread_mutex_lock(&mutexReadyList);
		list_add(colaReady, dtbEjecutandoABloquear);
		pthread_mutex_unlock(&mutexReadyList);

		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

int pasarDTBdeEXECaBLOQUED(t_dtb * dtbABloq){

    pthread_mutex_lock(&mutexEjecutandoList);
    int index = buscarDTBEnCola(colaEjecutando,dtbABloq);
    pthread_mutex_unlock(&mutexEjecutandoList);
	if(index >= 0){

		pthread_mutex_lock(&mutexEjecutandoList);
		t_dtb * dtbEjecutandoABloquear = (t_dtb *) list_remove(colaEjecutando,index);
		pthread_mutex_unlock(&mutexEjecutandoList);
		dtbEjecutandoABloquear->programCounter = dtbABloq->programCounter;
		dtbEjecutandoABloquear->quantumRestante = dtbABloq->quantumRestante;
		pthread_mutex_lock(&mutexBloqueadosList);
	    list_add(colaBloqueados, dtbEjecutandoABloquear);
	    pthread_mutex_unlock(&mutexBloqueadosList);

	}else{
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int pasarDTBdeEXECaFINALIZADO(t_dtb * dtbAFinalizar){

    pthread_mutex_lock(&mutexEjecutandoList);
    int index = buscarDTBEnCola(colaEjecutando,dtbAFinalizar);
    pthread_mutex_unlock(&mutexEjecutandoList);

	if(index >= 0){
		pthread_mutex_lock(&mutexEjecutandoList);
		t_dtb * dtbEjecutandoAFinalizar = (t_dtb *) list_remove(colaEjecutando,index);
		pthread_mutex_unlock(&mutexEjecutandoList);

		dtbEjecutandoAFinalizar->programCounter = dtbAFinalizar->programCounter;

		pthread_mutex_lock(&mutexExitList);
	    list_add(colaExit, dtbEjecutandoAFinalizar);
	    pthread_mutex_unlock(&mutexExitList);
	}
	else
	{
		return EXIT_FAILURE;
	}

	sem_post(&semaforoGradoMultiprgramacion);
	return EXIT_SUCCESS;
}

int pasarDTBdeBLOQUEADOaFINALIZADO(t_dtb * dtbABloq){

    pthread_mutex_lock(&mutexBloqueadosList);
    int index = buscarDTBEnCola(colaBloqueados,dtbABloq);
    pthread_mutex_unlock(&mutexBloqueadosList);

	if(index >= 0){
		pthread_mutex_lock(&mutexBloqueadosList);
		t_dtb * dtbBloqueadoAFinalizar = (t_dtb *) list_remove(colaBloqueados,index);
		pthread_mutex_unlock(&mutexBloqueadosList);

		dtbBloqueadoAFinalizar->programCounter = dtbABloq->programCounter;

		pthread_mutex_lock(&mutexExitList);
	    list_add(colaExit, dtbBloqueadoAFinalizar);
	    pthread_mutex_unlock(&mutexExitList);
	}else{
		return EXIT_FAILURE;
	}
	sem_post(&semaforoGradoMultiprgramacion);
	return EXIT_SUCCESS;
}

void pasarDTBdeBLOQaREADYESP(t_dtb * dtbAReadyEsp){

	pthread_mutex_lock(&mutexReadyEspList);
	int index = buscarDTBEnCola(colaReadyEspecial,dtbAReadyEsp);
	pthread_mutex_unlock(&mutexReadyEspList);
	if(index >= 0){
		pthread_mutex_lock(&mutexBloqueadosList);
		t_dtb * dtbBloqAReadyEsp = (t_dtb *) list_remove(colaBloqueados,index);
		pthread_mutex_unlock(&mutexBloqueadosList);

		pthread_mutex_lock(&mutexReadyEspList);
		list_add(colaReadyEspecial, dtbBloqAReadyEsp);
		pthread_mutex_unlock(&mutexReadyEspList);

		sem_post(&hayProcesosEnReady);
	}

}

void pasarDTBdeBLOQaREADY(t_dtb * dtbAReady){

	pthread_mutex_lock(&mutexBloqueadosList);
	int index = buscarDTBEnCola(colaBloqueados,dtbAReady);
	pthread_mutex_unlock(&mutexBloqueadosList);
	if(index >= 0){
		pthread_mutex_lock(&mutexBloqueadosList);
		t_dtb * dtbBloqAReady = (t_dtb *) list_remove(colaBloqueados,index);
		pthread_mutex_unlock(&mutexBloqueadosList);

		pthread_mutex_lock(&mutexReadyList);
		list_add(colaReady, dtbBloqAReady);
		pthread_mutex_unlock(&mutexReadyList);

		sem_post(&hayProcesosEnReady);
	}

}

void pasarDTBdeNEWaREADY(t_dtb * dtbAReady){

	pthread_mutex_lock(&mutexNewList);
	int index = buscarDTBEnCola(colaNew,dtbAReady);
	pthread_mutex_unlock(&mutexNewList);
	if(index >= 0){
		pthread_mutex_lock(&mutexNewList);
		t_dtb * dtbNewAReady = (t_dtb *) list_remove(colaNew,index);
		pthread_mutex_unlock(&mutexNewList);

		pthread_mutex_lock(&mutexReadyList);
		list_add(colaReady, dtbNewAReady);
		pthread_mutex_unlock(&mutexReadyList);

		sem_post(&hayProcesosEnReady);
	}
}

int pasarDTBdeNEWaEXIT(t_dtb * dtbAExit){

	pthread_mutex_lock(&mutexNewList);
	int index = buscarDTBEnCola(colaNew,dtbAExit);
	pthread_mutex_unlock(&mutexNewList);

	if(index >= 0){

		pthread_mutex_lock(&mutexNewList);
		t_dtb * dtbNewAExit = (t_dtb *) list_remove(colaNew,index);
		pthread_mutex_unlock(&mutexNewList);

		pthread_mutex_lock(&mutexExitList);
		list_add(colaExit, dtbNewAExit);
		pthread_mutex_unlock(&mutexExitList);
	}else{
		return EXIT_FAILURE;
	}

	sem_post(&semaforoGradoMultiprgramacion);
	return EXIT_SUCCESS;
}

void pasarDTBSegunQuantumRestante(t_dtb * dtb){

	if(dtb->quantumRestante > 0){
		pasarDTBdeBLOQaREADYESP(dtb);
	}else{
		pasarDTBdeBLOQaREADY(dtb);
	}

}

void enviarDTBaCPU(t_dtb *dtbAEnviar, int socketCpu){

    log_info_mutex(logger, "PCP: Se enviara un DTB a CPU");

    //CREO EL PAQUETE Y LO COMPRIMO
    t_package paquete = transformarDTBAPaquete(dtbAEnviar);

    //MANDO EL PAQUETE CON EL MENSAJE A LA CPU LIBRE
	if(enviar(socketCpu,SAFA_CPU_EJECUTAR,paquete.data,paquete.size,logger->logger)){
		//Error al enviar
		log_error_mutex(logger, "No se pudo enviar el DTB al CPU..");
		int result = pasarDTBdeEXECaFINALIZADO(dtbAEnviar);
		if(result == EXIT_FAILURE){
			log_error_mutex(logger, "Error al finalizar el DTB..");
	   }else{
		   log_error_mutex(logger, "SE FINALIZÓ EL PROCESO ID: %d POR INCONVENIENTES DE ENVIOS", dtbAEnviar->idGDT);
		   //si estaba ejecutando -> Se hace signal del semaforo y se libera la cpu
			liberarCpu(socketCpu);
	   }
	}
	log_info_mutex(logger, "Se envió el DTB a ejecutar a la CPU: %d",socketCpu);
	free(paquete.data);
}

int buscarCPULibre(){
	for(int i = 0; i<list_size(listaCpus);i++){
		t_cpus * cpu = list_remove(listaCpus,i);
		if(cpu->libre== 0)
		{
			cpu->libre = 1;
			list_add(listaCpus,cpu);
			return cpu->socket;
		}
		list_add(listaCpus,cpu);
	}
	return -1;
}
