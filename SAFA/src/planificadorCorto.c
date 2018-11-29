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

	//TODO: Si llego a crear alguna lista, acordarme de hacer los list destroy

	log_info_mutex(logger, "Hilo planificador Corto Plazo iniciado");
	dummyDTB = (t_dtb *) crearDummyDTB();
	bloquearDummy();

	pthread_mutex_init(&mutexPlanificando, NULL);
	int planificando = 0; // 0=No planificando  1=planificando

    while(1){

    	/**
    	 * FUNCIONALIDAD 1:
    	 * Si hay un proceso en new y tiene el flag de inicializacion en 1:
    	 *      -> Debo desbloquear el dtbdummy y agregarlo al a lista de ready
    	 */

    	if(list_size(colaNew) > 0){
    		int index = buscarDtbParaInicializar();
    		if(index > 0){
    			//Se desbloquea el dummy y se agrega a la lista de ready
    			desbloquearDummy();
    		}
    	}

    	/* FUNCIONALIDAD 2:
    	 * Si hay procesos en la cola de ready y hay cpus libres
    	 * 		->Se manda a ejecutar s/ el algoritmo
    	 */
    	if( (list_size(colaReady) > 0)){
    		int socketCPU = buscarCPULibre();
    			if(socketCPU > 0){
				switch (conf->algoritmo) {
					case RR:
						log_info_mutex(logger, "PCP mediante Round Robin");
						pthread_mutex_lock(&mutexPlanificando);
						ejecutarRR(socketCPU);
						pthread_mutex_unlock(&mutexPlanificando);
						break;
					case VRR:
						log_info_mutex(logger, "PCP mediante Virtual Round Robin");
			//          planificarVRR();
						break;
					default:
						log_info_mutex(logger, "PCP mediante Propio");
			//            playExecute();
						break;
				}
			}

    	}
    }
}

int buscarDtbParaInicializar(){
	int index = -1;
	int sizeList = list_size(colaNew);

	if(sizeList<=0) return index;

	for(int i = 0; i<sizeList;i++){
		t_dtb * dtb = list_get(colaNew,0);
		if(dtb->flagInicializado == 1){
			return index;
		}
	}
	return index;
}

void planificadorCPdesbloquearDummy(int idGDT, char *dirScript){

	//Recibo la solicitud y desbloqueo el dummy. Lo pongo en READY
	// No hace falta verificar si ready tiene lugar porque lo mutexeo el de Largo Plazo
	desbloquearDummy();
	dummyDTB->idGDT = idGDT;
	dummyDTB->dirEscriptorio = dirScript;

	list_add(colaReady, dummyDTB);
}

void ejecutarRR(int socketCpu){

	//Se pasa el primer proceso de los Ready a CPU a Ejecutar
	//Se cambia de cola
	t_dtb *dtb = pasarDTBdeREADYaEXEC();

	//Se envía a cpu
	enviarDTBaCPU(dtb,socketCpu);

}

t_dtb *pasarDTBdeREADYaEXEC(){

    pthread_mutex_lock(&mutexReadyList);
    pthread_mutex_lock(&mutexEjecutandoList);
    t_dtb *primerDTBenReady = (t_dtb *) list_remove(colaReady,0);
    list_add(colaEjecutando, primerDTBenReady);
    pthread_mutex_unlock(&mutexEjecutandoList);
    pthread_mutex_unlock(&mutexReadyList);
    return primerDTBenReady;
}

int pasarDTBdeEXECaREADY(t_dtb * dtbABloq){

	pthread_mutex_lock(&mutexBloqueadosList);
	pthread_mutex_lock(&mutexEjecutandoList);

	int index = buscarDTBEnCola(colaEjecutando,dtbABloq);

	if(index > 0){
		t_dtb * dtbEjecutandoABloquear = (t_dtb *) list_remove(colaEjecutando,index);
		list_add(colaReady, dtbEjecutandoABloquear);
	}else{
		//Error
		pthread_mutex_unlock(&mutexEjecutandoList);
		pthread_mutex_unlock(&mutexBloqueadosList);
		return EXIT_FAILURE;
	}

	pthread_mutex_unlock(&mutexEjecutandoList);
	pthread_mutex_unlock(&mutexBloqueadosList);
	return EXIT_SUCCESS;
}

int pasarDTBdeEXECaBLOQUED(t_dtb * dtbABloq){

    pthread_mutex_lock(&mutexBloqueadosList);
    pthread_mutex_lock(&mutexEjecutandoList);

    int index = buscarDTBEnCola(colaEjecutando,dtbABloq);

	if(index > 0){
		t_dtb * dtbEjecutandoABloquear = (t_dtb *) list_remove(colaEjecutando,index);
	    list_add(colaBloqueados, dtbEjecutandoABloquear);
	}else{
		//Error
		pthread_mutex_unlock(&mutexEjecutandoList);
		pthread_mutex_unlock(&mutexBloqueadosList);
		return EXIT_FAILURE;
	}

    pthread_mutex_unlock(&mutexEjecutandoList);
	pthread_mutex_unlock(&mutexBloqueadosList);
	return EXIT_SUCCESS;
}

void enviarDTBaCPU(t_dtb *dtbAEnviar, int socketCpu){

    log_info_mutex(logger, "PCP: Se enviara un DTB a CPU");

    //CREO EL PAQUETE Y LO COMPRIMO
    char *paquete;

    copyIntToBuffer(&paquete,dtbAEnviar->idGDT);
    copyStringToBuffer(&paquete,dtbAEnviar->dirEscriptorio);
    copyIntToBuffer(&paquete,dtbAEnviar->programCounter);
    copyIntToBuffer(&paquete,dtbAEnviar->flagInicializado);
    copyStringToBuffer(&paquete,dtbAEnviar->tablaDirecciones);
    copyIntToBuffer(&paquete,dtbAEnviar->cantidadLineas);

    int pqtSize = sizeof(int)*4 +
    		(strlen(dtbAEnviar->dirEscriptorio) + strlen(dtbAEnviar->tablaDirecciones)) * sizeof(char);

    //MANDO EL PAQUETE CON EL MENSAJE A LA CPU LIBRE
	if(enviar(socketCpu,SAFA_CPU_EJECUTAR,paquete,pqtSize,logger->logger)){
		//Error al enviar
		log_error_mutex(logger, "No se pudo enviar el DTB al CPU..");
		int result = pasarDTBdeEXECaBLOQUED(dtbAEnviar);
		if(result == EXIT_FAILURE){
			log_error_mutex(logger, "Error al bloquear el DTB..");
	   }
	}
	log_info_mutex(logger, "Se envió el DTB a ejecutar a la CPU: %d",socketCPU);


    free(paquete);
}

int buscarCPULibre(){
	int socketLibre;
	for(int i = 0; i<list_size(listaCpus);i++){
		t_cpus * cpu = list_get(listaCpus,i);
		if(cpu->libre== 0){
			return socketLibre = cpu->socket;
		}
	}
	return -1;
}



