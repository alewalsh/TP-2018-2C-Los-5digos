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

    while(1){

    	/**
    	 * FUNCIONALIDAD 1:
    	 * Si hay un proceso en new y tiene el flag de inicializacion en 1:
    	 *      -> Debo desbloquear el dtbdummy y agregarlo al a lista de ready
    	 */

    	if(list_size(colaNew) > 0 && dummyBloqueado == 1){
    		int index = buscarDtbParaInicializar();
    		if(index > 0){
    			//Se desbloquea el dummy y se agrega a la lista de ready
    			sem_wait(&semDummy);
    			desbloquearDummy();
    		}
    	}

    	/* FUNCIONALIDAD 2:
    	 * Si hay procesos en la cola de ready y hay cpus libres
    	 * 		->Se manda a ejecutar s/ el algoritmo
    	 */
    	if(list_size(colaReady) > 0 || list_size(colaReadyEspecial) >0){
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
			            ejecutarVRR(socketCPU);
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
	int sizeList = list_size(colaNew);

	if(sizeList<=0) return -1;

	for(int i = 0; i<sizeList;i++){
		t_dtb * dtb = list_get(colaNew,i);
		if(dtb->flagInicializado == 1 && dtb->realizOpDummy == 0){
			//Actualizar dummy con el path
			dummyDTB->dirEscriptorio = dtb->dirEscriptorio;
			dtb->realizOpDummy = 1;
			list_add_in_index(colaNew,i,dtb);
			return i;
		}
	}
	return -1;
}

void planificadorCPdesbloquearDummy(int idGDT, char *dirScript){

	//Recibo la solicitud y desbloqueo el dummy.
	dummyBloqueado = 1;
}

void ejecutarRR(int socketCpu){

	//Se pasa el primer proceso de los Ready a CPU a Ejecutar
	//Se cambia de cola
	t_dtb *dtb;
	if(list_size(colaReadyEspecial)>0){
		//contemplo el caso de que se haya cambiado de algoritmo en medio de la ejecucion y
		//haya quedado algun proceso en la cola de ready especial
		dtb = pasarDTBdeREADYESPaEXEC();
	} else if(list_size(colaReady)>0){

		dtb = pasarDTBdeREADYaEXEC();

	}

	//Se envía a cpu
	enviarDTBaCPU(dtb,socketCpu);

}

void ejecutarVRR(int socketCPU){
	t_dtb *dtb ;

	if(list_size(colaReadyEspecial) > 0){
		dtb= pasarDTBdeREADYESPaEXEC();
	}else{
		dtb = pasarDTBdeREADYaEXEC();
	}

	enviarDTBaCPU(dtb,socketCPU);

}
/*
 * FUNCION PARA PASAR EL PRIMER PROCESO DE LA COLA READY A EJECUTAR
 * return: (t_dtb) dtb que se pasó de cola de ready para mandar a ejecutar
 */
t_dtb *pasarDTBdeREADYaEXEC(){

    pthread_mutex_lock(&mutexReadyList);
    pthread_mutex_lock(&mutexEjecutandoList);
    t_dtb *primerDTBenReady = (t_dtb *) list_remove(colaReady,0);
    list_add(colaEjecutando, primerDTBenReady);
    pthread_mutex_unlock(&mutexEjecutandoList);
    pthread_mutex_unlock(&mutexReadyList);
    return primerDTBenReady;
}

/*
 * FUNCION PARA PASAR EL PRIMER PROCESO DE LA COLA READY ESPECIAL A EJECUTAR
 * return: (t_dtb) dtb que se pasó de la cola de ready especial para mandar a ejecutar
 */
t_dtb * pasarDTBdeREADYESPaEXEC(){

	pthread_mutex_lock(&mutexReadyEspList);
	pthread_mutex_lock(&mutexEjecutandoList);
	t_dtb *primerDTBenReadyEsp = (t_dtb *) list_remove(colaReadyEspecial,0);
	list_add(colaEjecutando, primerDTBenReadyEsp);
	pthread_mutex_unlock(&mutexEjecutandoList);
	pthread_mutex_unlock(&mutexReadyEspList);
	return primerDTBenReadyEsp;
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

int pasarDTBdeEXECaFINALIZADO(t_dtb * dtbABloq){

    pthread_mutex_lock(&mutexEjecutandoList);
    pthread_mutex_lock(&mutexExitList);

    int index = buscarDTBEnCola(colaEjecutando,dtbABloq);

	if(index > 0){
		t_dtb * dtbEjecutandoAFinalizar = (t_dtb *) list_remove(colaEjecutando,index);
	    list_add(colaExit, dtbEjecutandoAFinalizar);
	    //TODO: SE DEBE HACER UN SIGNAL DEL MUTEX PARA EL GRADO DE MULTIPROGRAMACION
	}else{
		//Error
		pthread_mutex_unlock(&mutexExitList);
		pthread_mutex_unlock(&mutexEjecutandoList);
		return EXIT_FAILURE;
	}

    pthread_mutex_unlock(&mutexExitList);
	pthread_mutex_unlock(&mutexEjecutandoList);

	sem_post(&semaforpGradoMultiprgramacion);
	return EXIT_SUCCESS;
}

int pasarDTBdeBLOQUEADOaFINALIZADO(t_dtb * dtbABloq){

    pthread_mutex_lock(&mutexBloqueadosList);
    pthread_mutex_lock(&mutexExitList);

    int index = buscarDTBEnCola(colaBloqueados,dtbABloq);

	if(index > 0){
		t_dtb * dtbBloqueadoAFinalizar = (t_dtb *) list_remove(colaBloqueados,index);
	    list_add(colaExit, dtbBloqueadoAFinalizar);
	    //TODO: SE DEBE HACER UN SIGNAL DEL MUTEX PARA EL GRADO DE MULTIPROGRAMACION
	}else{
		//Error
		pthread_mutex_unlock(&mutexExitList);
		pthread_mutex_unlock(&mutexBloqueadosList);
		return EXIT_FAILURE;
	}

    pthread_mutex_unlock(&mutexExitList);
	pthread_mutex_unlock(&mutexBloqueadosList);
	sem_post(&semaforpGradoMultiprgramacion);
	return EXIT_SUCCESS;
}

void pasarDTBdeBLOQaREADYESP(t_dtb * dtbAReadyEsp){

	pthread_mutex_lock(&mutexBloqueadosList);
	pthread_mutex_lock(&mutexReadyEspList);

	int index = buscarDTBEnCola(colaReadyEspecial,dtbAReadyEsp);

	if(index > 0){
		t_dtb * dtbBloqAReadyEsp = (t_dtb *) list_remove(colaBloqueados,index);
		list_add(colaReadyEspecial, dtbBloqAReadyEsp);
	}

	pthread_mutex_unlock(&mutexReadyEspList);
	pthread_mutex_unlock(&mutexBloqueadosList);
}

void pasarDTBdeBLOQaREADY(t_dtb * dtbAReady){

	pthread_mutex_lock(&mutexBloqueadosList);
	pthread_mutex_lock(&mutexReadyList);

	int index = buscarDTBEnCola(colaReadyEspecial,dtbAReady);

	if(index > 0){
		t_dtb * dtbBloqAReady = (t_dtb *) list_remove(colaBloqueados,index);
		list_add(colaReady, dtbBloqAReady);
	}

	pthread_mutex_unlock(&mutexReadyList);
	pthread_mutex_unlock(&mutexBloqueadosList);
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
    char *paquete;

    copyIntToBuffer(&paquete,dtbAEnviar->idGDT);
    copyStringToBuffer(&paquete,dtbAEnviar->dirEscriptorio);
    copyIntToBuffer(&paquete,dtbAEnviar->programCounter);
    copyIntToBuffer(&paquete,dtbAEnviar->flagInicializado);
    copyStringToBuffer(&paquete,dtbAEnviar->tablaDirecciones);
    copyIntToBuffer(&paquete,dtbAEnviar->cantidadLineas);
    copyIntToBuffer(&paquete, dtbAEnviar->quantumRestante);

    int pqtSize = sizeof(int)*4 +
    		(strlen(dtbAEnviar->dirEscriptorio) + strlen(dtbAEnviar->tablaDirecciones)) * sizeof(char);

    //MANDO EL PAQUETE CON EL MENSAJE A LA CPU LIBRE
	if(enviar(socketCpu,SAFA_CPU_EJECUTAR,paquete,pqtSize,logger->logger)){
		//Error al enviar
		log_error_mutex(logger, "No se pudo enviar el DTB al CPU..");
		int result = pasarDTBdeEXECaFINALIZADO(dtbAEnviar);
		if(result == EXIT_FAILURE){
			log_error_mutex(logger, "Error al finalizar el DTB..");
	   }
	}
	log_info_mutex(logger, "Se envió el DTB a ejecutar a la CPU: %d",socketCpu);


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



