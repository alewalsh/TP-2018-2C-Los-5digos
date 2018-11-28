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

	t_dtb *dtb = pasarDTBdeREADYaEXEC();

	enviarDTBaCPU(dtb);

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

void pasarDTBdeEXECaBLOQUED(t_dtb dtbABloq){

    pthread_mutex_lock(&mutexBloqueadosList);
    pthread_mutex_lock(&mutexEjecutandoList);

    int dtbABloquear;

	for(int i = 0; i<list_size(colaEjecutando);i++){
		t_dtb dtb = list_get(colaEjecutando,i);
		if(dtb.idGDT == dtbABloq.idGDT){
			dtbABloquear = i;
		}
	}

	t_dtb * dtbEjecutandoABloquear = (t_dtb *) list_remove(colaEjecutando,dtbABloquear);
    list_add(colaBloqueados, dtbEjecutandoABloquear);

    pthread_mutex_unlock(&mutexEjecutandoList);
	pthread_mutex_unlock(&mutexBloqueadosList);
}

void enviarDTBaCPU(t_dtb *dtbAEnviar){

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
    int socketCPU = buscarCPULibre();

    if(socketCPU > 0){
    	if(enviar(socketCPU,SAFA_CPU_EJECUTAR,&paquete,pqtSize,logger)){
			//Error al enviar
			log_error_mutex(logger, "No se pudo enviar el DTB al CPU..");
			//TODO: se deberia pasar el proceso a bloqueado
			pasarDTBdeEXECaBLOQUED(dtbAEnviar);
		}
    }else{
    	//NO HAY CPUS LIBRES -> no debería pasar

    }

    free(paquete);
}

int buscarCPULibre(){
	int socketLibre;
	for(int i = 0; i<list_size(listaCpus);i++){
		t_cpus cpu = list_get(listaCpus,i);
		if(cpu.libre == 0){
			return socketLibre = cpu.socket;
		}
	}
	return -1;
}



