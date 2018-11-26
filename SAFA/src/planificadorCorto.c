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

void enviarDTBaCPU(t_dtb *dtbAEnviar){

    log_info_mutex(logger, "PCP: Se enviara un DTB a CPU");

    //CREO EL PAQUETE Y LO COMPRIMO
    char *paquete;
    int pqtSize = sizeof(int);

    copyIntToBuffer(&paquete,dtbAEnviar->idGDT);
    copyStringToBuffer(&paquete,dtbAEnviar->dirEscriptorio);
    copyIntToBuffer(&paquete,dtbAEnviar->programCounter);
    copyIntToBuffer(&paquete,dtbAEnviar->flagInicializado);
    copyStringToBuffer(&paquete,dtbAEnviar->tablaDirecciones);
    copyIntToBuffer(&paquete,dtbAEnviar->cantidadLineas);

    pqtSize = pqtSize + strlen(paquete);

    //MANDO EL PAQUETE CON EL MENSAJE

    //TODO: Crear una lista con los int de los socketCpus, cuando vot a mandar, tengo que sacar el socket
    // del masterSet, hacer un update del readset, y mandar a ese socket.
    // Una vez que se mando, tengo que agregar el socket denuevo al master y update denuevo.

    //SI TODO OK, HAGO EL FREE, SINO, HAGO EL FREE EN EL FAIL

}



//int enviar(int socket, uint16_t code, char *data, uint32_t size, t_log *logger) {

//char* copyStringToBuffer(char **buffer, char* origin) {
//char* copyIntToBuffer(char** buffer, int value) {
//char *copySizeToBuffer(char**buffer, char*data, int size) {

// Enviar la informacion del recurso y del idGDT que lo bloquea (o desbloquea) al SAFA
//	int size = strlen(recurso) + sizeof(int);
//	char * buffer;
//	copyStringToBuffer(&buffer, recurso);
//	copyIntToBuffer(&buffer,idGDT);
//	size = strlen(buffer);
//	if(enviar(t_socketSAFA->socket,code,buffer, size, loggerCPU->logger))
//	{
//		log_error_mutex(loggerCPU, "No se pudo enviar el bloqueo o desbloqueo del recurso al SAFA..");
//		free(buffer);
//		return EXIT_FAILURE;
//	}
//	free(buffer);
//	return EXIT_SUCCESS;




