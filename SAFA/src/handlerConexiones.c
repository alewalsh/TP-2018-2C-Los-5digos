/*
 * handlerConexiones.c
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#include "handlerConexiones.h"
#include <grantp/configuracion.h>
#include <grantp/socket.h>
#include <grantp/mutex_log.h>
#include <grantp/structCommons.h>
#include "funcionesSAFA.h"
#include "planificadorCorto.h"
#include "planificadorLargo.h"


void manejarConexiones(){

    int socketListen, i,nuevoFd;
    uint16_t handshake;
    t_package pkg;

    listaRecursoAsignados = list_create();
    estadoSAFA = Corrupto;
	int CPUConectado, DAMConectado = 0;
	log_trace_mutex(logger, "Se inicializa SAFA en estado Corrupto");

	initCpuList();

    //Creo el socket y me quedo escuchando
	if (escuchar(conf->puerto, &socketListen, logger->logger)) {
		liberarRecursos();
		pthread_exit(NULL);
    }

	log_trace_mutex(logger, "El socket de escucha de SAFA es: %d", socketListen);
    log_info_mutex(logger, "El socket de escucha de SAFA es: %d", socketListen);

    addNewSocketToMaster(socketListen);

    //Se queda escuchando conexiones hasta estar en estado operativo (DAM CONECTADO Y 1 CPU CONECTADA)
    while(estadoSAFA != Operativo){

        if (acceptConnection(socketListen, &nuevoFd, SAFA_HSK, &handshake, logger->logger)) {
            log_error_mutex(logger, "No se acepta la conexion");
        }

        switch (handshake) {
            case DAM_HSK:
            	log_trace_mutex(logger, "Se me conecto el DAM, socket: %d", nuevoFd);
                addNewSocketToMaster(nuevoFd);
            	DAMConectado++;
                break;
            case CPU_HSK:
            	addNewSocketToMaster(nuevoFd);
            	manejarNuevaCPU(nuevoFd);
            	CPUConectado++;
                break;
            default:
                log_warning_mutex(logger, "Se me quizo conectar alguien que no espero");
                close(nuevoFd);
                break;
        }

        if ((CPUConectado != 0) && (DAMConectado != 0)){
        	estadoSAFA = Operativo;
        }
    }

	log_trace_mutex(logger, "Se pasa a estado OPERATIVO");


    while (1) {

    	updateReadset();

        //Hago un select sobre el conjunto de sockets activo
        int result = select(getMaxfd() + 1, &readset, NULL, NULL, NULL);
        if (result == -1) {
            log_error_mutex(logger, "Error en el select: %s", strerror(errno));
            exit_gracefully(EXIT_FAILURE);
        }

        log_trace_mutex(logger, "El valor del select es: %d", result);

        log_trace_mutex(logger, "Analizo resultado del select para ver quien me hablo");

        for (i = 0; i <= getMaxfd(); i++) {

        	if (isSetted(i)) { // ¡¡tenemos datos!!

        		if (i == socketListen) {

                    // CAMBIOS EN EL SOCKET QUE ESCUCHA, acepto las nuevas conexiones
                    log_trace_mutex(logger, "Cambios en Listener de SAFA, se gestionara la conexion correspondiente");
                    if (acceptConnection(socketListen, &nuevoFd, SAFA_HSK, &handshake, logger->logger)) {
                        log_error_mutex(logger, "No se acepto la nueva conexion solicitada");
                    } else {
                        // añadir al conjunto maestro
                        log_trace_mutex(logger, "Se acepto la nueva conexion solicitada en el SELECT");
                        addNewSocketToMaster(nuevoFd);
                        if (handshake == CPU_HSK)
                        {
                        	manejarNuevaCPU(nuevoFd);
                        }
                    }
                } else {
                     //gestionar datos de un cliente
                    if (recibir(i, &pkg, logger->logger)) {
                        log_error_mutex(logger, "No se pudo recibir el mensaje");
//                        handlerDisconnect(i);
                    } else {
                        manejarSolicitud(pkg, i);
                    }

                }
            }
        }
    }

}


void exit_gracefully(int error)
{
	notificarDesconexionCPUs();
	liberarRecursos();
    destruirListas();
	exit(error);
}

void notificarDesconexionCPUs()
{
	if (list_size(listaCpus) > 0)
	{
		for(int i =0; i < list_size(listaCpus); i++)
		{
			t_cpus * cpu = list_get(listaCpus, i);
			if(enviar(cpu->socket,CPU_SAFA_DISCONNECT,NULL, 0, logger->logger))
			{
				log_error_mutex(logger, "No se pudo enviar el fin de ejecución del SAFA a las CPU.");
			}
			else
			{
				log_info_mutex(logger, "La desconexión del SAFA se ha realizado exitosamente");
			}
		}
	}
	else
	{
		log_info_mutex(logger, "No hay CPUs a las cuales avisar la desconexión.");
	}
}

void manejarSolicitud(t_package pkg, int socketFD) {
    switch (pkg.code) {

    	  /*
           * -----------------CONFIRMACIONES DEL CPU-----------------
           */
        case CPU_SAFA_BLOQUEAR_DTB:{
        	t_dtb * dtb = transformarPaqueteADTB(pkg);
        	actualizarMetricas(dtb);
        	// SI BLOQUEO ES PORQUE FUE A DAM, SUMO 1 a LA METRICA
        	actualizarSentenciasPasaronPorDAM(1);

			archivoCerradoEnCPU(dtb->idGDT, dtb->tablaDirecciones);
        	if(bloquearDTB(dtb))
        	{
        		log_error_mutex(logger, "Hubo un error al bloquear el DTB");
        	}else{
        		log_info_mutex(logger, "SE BLOQUEÓ EL PROCESO ID: %d",dtb->idGDT);
        		//Se libera una cpu y se hace signal del semaforo
				liberarCpu(socketFD);
        	}
        	break;
        }
        case CPU_SAFA_ABORTAR_DTB:{

        	t_dtb * dtb = transformarPaqueteADTB(pkg);
        	actualizarMetricas(dtb);
			archivoCerradoEnCPU(dtb->idGDT, dtb->tablaDirecciones);

        	if(abortarDTB(dtb, socketFD))
        	{
        		log_error_mutex(logger, "Hubo un error al abortar el DTB.");
        	}else{
        		log_info_mutex(logger, "FINALIZO EL PROCESO ID: %d",dtb->idGDT);
        	}
        	break;
        }
        case CPU_SAFA_ABORTAR_DTB_NUEVO:{
			t_dtb * dtb = transformarPaqueteADTB(pkg);
			archivoCerradoEnCPU(dtb->idGDT, dtb->tablaDirecciones);
			if(abortarDTBNuevo(dtb))
			{
				log_error_mutex(logger, "Hubo un error al abortar el DTB.");
			}
			break;
		}
        case CPU_SAFA_BLOQUEAR_DUMMMY:
        	//Se bloquea el dummy
        	bloquearDummy();
        	pthread_mutex_unlock(&semDummy);
        	//Se libera una cpu y se hace signal del semaforo
			liberarCpu(socketFD);
        	break;
        case CPU_SAFA_FIN_EJECUCION_DTB:{
        	t_dtb * dtb = transformarPaqueteADTB(pkg);
        	actualizarMetricas(dtb);
			archivoCerradoEnCPU(dtb->idGDT, dtb->tablaDirecciones);

        	if(abortarDTB(dtb, socketFD))
			{
				log_error_mutex(logger, "Hubo un error al abortar el DTB.");
			}else{
				log_info_mutex(logger, "FINALIZO LA EJECUCION DEL PROCESO ID: %d",dtb->idGDT);
			}
        	break;
        }
        case CPU_SAFA_FIN_EJECUCION_X_QUANTUM_DTB:{
        	//TODO AGREGAR LIBERACION DE CPU
        	t_dtb * dtb = transformarPaqueteADTB(pkg);
        	actualizarMetricas(dtb);
			archivoCerradoEnCPU(dtb->idGDT, dtb->tablaDirecciones);

        	if(finEjecucionPorQuantum(dtb)){
        		log_error_mutex(logger, "Hubo un error al llevar el DTB a la cola de READY por finalizacion de quantum.");
        	}else{
				log_info_mutex(logger, "FINALIZO EL QUANTUM DEL PROCESO ID: %d",dtb->idGDT);
				//se hace un signal del semaforo para avisar que hay un proceso en ready
				sem_post(&hayProcesosEnReady);
				liberarCpu(socketFD);
			}
        	break;
        }

        /*
         * -----------------CONFIRMACIONES DEL DMA-------------------------------
         */

        //EL SCRIPTORIO SE INCIALIZÓ
        case DAM_SAFA_CONFIRMACION_SCRIPT_INICIALIZADO:{
        	char * buffer = pkg.data;
        	int pid = copyIntFromBuffer(&buffer);
			int result = copyIntFromBuffer(&buffer);
			int cantIOProcess = copyIntFromBuffer(&buffer);
			int cantLineasProceso = copyIntFromBuffer(&buffer);

			pthread_mutex_lock(&mutexNewList);
			t_dtb * dtb = buscarDTBPorPIDenCola(colaNew, pid);
			pthread_mutex_unlock(&mutexNewList);
			if(result == EXIT_SUCCESS){
				actualizarIODtb(dtb, cantIOProcess,cantLineasProceso);
				pasarDTBdeNEWaREADY(dtb); //Se cargó en memoria correctamente
				log_info_mutex(logger, "Se cargó correctamente en memoria el proceso: %d", pid);
			}else{
				abortarDTBNuevo(dtb); //No se pudo cargar a memoria
				log_error_mutex(logger, "No se pudo cargar en memoria el proceso: %d", pid);
			}

        	break;
        }

        //LA PETICION "ABRIR ARCHIVO" SE FINALIZO
        case DAM_SAFA_CONFIRMACION_PID_CARGADO:{
        	char * buffer = pkg.data;
			int pid = copyIntFromBuffer(&buffer);
			int result = copyIntFromBuffer(&buffer);
			char * path = copyStringFromBuffer(&buffer);

			if(result == EXIT_SUCCESS){ //SE CARGÓ CORRECTAMENTE
				//actualizar tabla de direcciones
				actualizarTablaDirecciones(pid, path);
			}

			if(confirmacionDMA(pid, result)){
				//Error
				log_error_mutex(logger, "No se pudo cargar el proceso pid: %d en memoria", pid);
			}
			log_info_mutex(logger,"Se cargó en memoria y se desbloqueó el proceso pid: %d", pid);

			break;
        }

        //LA PETICION "FLUSH" SE FINALIZÓ
        //LA PETICION "CREAR ARCHIVO" SE FINALIZÓ
        //LA PETICION "BORRAR ARCHIVO" SE FINALIZÓ
		case DAM_SAFA_CONFIRMACION_DATOS_GUARDADOS:
		case DAM_SAFA_CONFIRMACION_CREAR_ARCHIVO:
		case DAM_SAFA_CONFIRMACION_BORRAR_ARCHIVO:
		case DAM_SAFA_FAIL:
		{
			char * buffer = pkg.data;
			int pid = copyIntFromBuffer(&buffer);
			int result = copyIntFromBuffer(&buffer);
			int cantIOProcess = copyIntFromBuffer(&buffer);
			int cantLineasProceso = copyIntFromBuffer(&buffer);
			if (cantIOProcess != 0 || cantLineasProceso != 0)
				log_warning_mutex(logger, "Estas recibiendo información distinta a la que deberias recibir.");

			if(confirmacionDMA(pid, result))
			{
				//Error
				log_error_mutex(logger, "No se pudo cargar el proceso pid: %d en memoria", pid);
			}
			else
			{
				log_info_mutex(logger,"Se cargó en memoria y se desbloqueó el proceso pid: %d", pid);
			}

			break;
	   }

        case CPU_SAFA_SIGNAL_RECURSO:{
        	char * recurso = copyStringFromBuffer(&pkg.data);
			hacerSignalDeRecurso(recurso);
			break;
        }
        case CPU_SAFA_WAIT_RECURSO:{
        	//TODO Fijarse si hay que actualizar las metricas
        	char * recurso = copyStringFromBuffer(&pkg.data);
			int pid = copyIntFromBuffer(&pkg.data);
			int programCounterActual = copyIntFromBuffer(&pkg.data);
			hacerWaitDeRecurso(recurso,pid,socketFD,programCounterActual);
			break;
        }
        case CPU_SAFA_DISCONNECT:
        {
        	finEjecucionCPU(socketFD);
        	break;
        }
        case SOCKET_DISCONECT:
			deleteSocketFromMaster(socketFD);
            close(socketFD);
            break;
        default:
            log_warning_mutex(logger, "El mensaje recibido es: %s", codigoIDToString(pkg.code));
            log_warning_mutex(logger, "Ojo, estas recibiendo un mensaje que no esperabas.");
            break;

    }

//    free(pkg.data);

}

void finEjecucionCPU(int socket)
{
	pthread_mutex_lock(&mutexCpus);
	socketBuscado = socket;
	list_remove_by_condition(listaCpus, (void *) buscarCPUPorSocket);
	pthread_mutex_unlock(&mutexCpus);
	log_info_mutex(logger, "Se ha desconectado una CPU.");
}

bool buscarCPUPorSocket(t_cpus * cpu)
{
	if (cpu->socket == socketBuscado)
		return true;
	return false;
}

void initCpuList(){
	listaCpus = list_create();
}

void manejarNuevaCPU(int nuevoFd)
{
	log_trace_mutex(logger, "Se me conecto un CPU, socket: %d", nuevoFd);
//    addNewSocketToMaster(nuevoFd);

    //agregar socket a lista de cpus
    t_cpus * cpu = crearCpu();
    cpu->libre= 0;
    cpu->socket= nuevoFd;
    pthread_mutex_lock(&mutexCpus);
    list_add(listaCpus, cpu);
    pthread_mutex_unlock(&mutexCpus);
    sem_post(&semaforoCpu);

	int size = sizeof(int);
    char *buffer = (char *) malloc(size);
    char *p = buffer;
    copyIntToBuffer(&p,conf->quantum);
	if(enviar(nuevoFd, SAFA_CPU_QUANTUM, buffer, size, logger->logger))
	{
		log_error_mutex(logger, "No se pudo enviar el quantum al CPU.");
		free(buffer);
	}
	free(buffer);
}


/**
 * -------------FUNCIONES PARA EL WAIT Y SIGNAL DE RECURSOS
 */
void hacerSignalDeRecurso(char * recursoLiberado){

	bool realizoSignal = false;

	for(int i =0; i<list_size(listaRecursoAsignados); i++)
	{ //HAY O HUBO RECURSOS ASIGNADOS

		t_recurso * recurso = list_get(listaRecursoAsignados,i);
		int recursoBuscado = strcmp(recurso->recursoId, recursoLiberado); //BUSCO EL RECURSO LIBERADO
		if(recursoBuscado == 0)
		{ //ENCONTRE EL RECURSO BUSCADO
			realizoSignal = true;
			t_recurso * recursoUsado = list_remove(listaRecursoAsignados, i);
			recursoUsado->valorSemaforo ++;
			if(recurso->valorSemaforo <= 0)
			{//TIENE PROCESOS BLOQUEADOS SOLICITANDOLO ->
				wakeUp(recursoUsado);
			}
			else
			{
				//NO TIENE RECURSOS SOLICITANDOLO
				recursoUsado->procesoDuenio = 0;
			}
			list_add_in_index(listaRecursoAsignados, i, recursoUsado);
		}
	}

	if(!realizoSignal){ //NO EXISTIA EL RECURSO
		//CREO EL RECURSO CON EL SEMAFORO EN 1
		t_recurso * recursoCreado = crearRecurso(recursoLiberado,0, 1);
		list_add(listaRecursoAsignados, recursoCreado);

	}
}

void wakeUp(t_recurso * recursoUsado){
	//Se toma el primero y se desbloquea
	t_dtb * dtbSolicitante = list_remove(recursoUsado->listProcesos,0);
	recursoUsado->procesoDuenio = dtbSolicitante->idGDT;

	//Se desbloquea el proceso pid
	desbloquearDTBsegunAlgoritmo(dtbSolicitante->idGDT);

}

void hacerWaitDeRecurso(char * recursoSolicitado, int pid, int socketCPU, int PCActual)
{
	bool recursoYaCreado = false;
	for(int i =0; i < list_size(listaRecursoAsignados); i++)
	{	//HAY O HUBO RECURSOS ASIGNADOS
		t_recurso * recurso = list_get(listaRecursoAsignados,i);
		int esElRecursoBuscado = strcmp(recurso->recursoId, recursoSolicitado);
		if(esElRecursoBuscado == 0)
		{ //OBTUVE EL RECURSO SOLICITADO
			recursoYaCreado = true;
			t_recurso * recursoUsado = list_remove(listaRecursoAsignados, i);
			recursoUsado->valorSemaforo --; //Se disminuye en 1 el semaforo
			//CONSULTO EL VALOR DEL SEMAFORO DEL RECURSO
			if(recursoUsado->valorSemaforo >= 0)
			{//El recurso no se está utilizando
				recursoUsado->procesoDuenio = pid; //LO ASIGNO AL PROCESO QUE LO PIDIÓ
				//Le envio la confirmacion al safa para que continue el proceso
				if(enviar(socketCPU,SAFA_CPU_INICIO_WAIT,NULL, 0, logger->logger))
				{
					log_error_mutex(logger, "No se pudo enviar el bloqueo o desbloqueo del recurso al SAFA.");
				}
			}
			else
			{//EXISTE Y ESTA SIENDO USADO ->
				block(recursoUsado, pid, PCActual,socketCPU);
			}
			list_add_in_index(listaRecursoAsignados, i, recursoUsado);
			break;
		}
	}

	if (!recursoYaCreado)
	{//NO EXISTE -> TENGO QUE CREAR UN T_RECURSO NUEVO, ASIGNARLO (Semaforo = 0) Y AGREGARLO A LA LISTA
		t_recurso * recursoCreado = crearRecurso(recursoSolicitado,pid, 0);
		list_add(listaRecursoAsignados, recursoCreado);
		if(enviar(socketCPU,SAFA_CPU_INICIO_WAIT,NULL, 0, logger->logger))
		{
			log_error_mutex(logger, "No se pudo enviar el bloqueo o desbloqueo del recurso al SAFA.");
		}
	}
}

void block(t_recurso * recursoUsado, int pid, int PCActual, int socketCPU){
	//TENGO QUE BLOQUEAR EL PROCESO QUE LO PIDIO
	t_dtb * newDTB = malloc(sizeof(t_dtb));
	newDTB->idGDT = pid;
	list_add(recursoUsado->listProcesos, newDTB); //SE AGREGA A LA COLA DE PROCESOS QUE PIDEN EL RECURSO

	//Se bloquea el proceso
	pthread_mutex_lock(&mutexEjecutandoList);
	t_dtb * dtb = buscarDTBPorPIDenCola(colaEjecutando,pid);
	dtb->programCounter = PCActual;
	pthread_mutex_unlock(&mutexEjecutandoList);
	pasarDTBdeEXECaBLOQUED(dtb);

	if(enviar(socketCPU,SAFA_CPU_BLOQUEO_WAIT,NULL, 0, logger->logger))
	{
		log_error_mutex(logger, "No se pudo enviar el bloqueo o desbloqueo del recurso al SAFA.");
	}
	//Se libera una cpu que lo estaba ejecutando
	liberarCpu(socketCPU);
}

t_recurso* crearRecurso(char * recurso, int pid, int valorSemaforo){
	t_list * listaProcesos = list_create();
	t_recurso * recursoStruct = malloc(sizeof(t_recurso));
	recursoStruct->recursoId = recurso;
	recursoStruct->procesoDuenio = pid;
	recursoStruct->listProcesos = listaProcesos;
	recursoStruct->valorSemaforo = valorSemaforo;
	return recursoStruct;
}

bool existeRecurso(t_recurso * recurso)
{
	if (strcmp(recurso->recursoId, recursoBuscado) == 0)
		return true;
	return false;
}

/**
 * ----------------------------------------------------------------------------------------------------------
 */

void actualizarMetricas(t_dtb * dtb){

	t_dtb * dtbAnterior = buscarDTBPorPIDenCola(colaEjecutando,dtb->idGDT);
	if(dtbAnterior != NULL){
		int instruccionesEjecutadas = dtb->programCounter - dtbAnterior->programCounter;
		//Actualizo la metrica de sentencias que espera un dtb en NEW
		actualizarMetricasDTBNew(instruccionesEjecutadas);
		// Actualizo valor general para saber cuantas sentencias en total se ejecutaron.
		actualizarTotalSentenciasEjecutadas(instruccionesEjecutadas);
		// Actualizo metrica de tiempo de respuesta
		actualizarMetricaTiempoDeRespuesta(instruccionesEjecutadas);
	}
}
