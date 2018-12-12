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
            	log_trace_mutex(logger, "Se me conecto un CPU, socket: %d", nuevoFd);
                addNewSocketToMaster(nuevoFd);
                CPUConectado++;

                //agregar socket a lista de cpus
                t_cpus * cpu = crearCpu();
                cpu->libre= 0;
                cpu->socket= nuevoFd;
                list_add(listaCpus, cpu);

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
            exit(1);
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
                    }
                } else {
                     //gestionar datos de un cliente
                    if (recibir(i, &pkg, logger->logger)) {
                        log_error_mutex(logger, "No se pudo recibir el mensaje");
                        //handlerDisconnect(i);
                    } else {
                        manejarSolicitud(pkg, i);
                    }

                }
            }
        }
    }

}


void manejarSolicitud(t_package pkg, int socketFD) {
    switch (pkg.code) {

    	  /*
           * -----------------CONFIRMACIONES DEL CPU-----------------
           */
        case CPU_SAFA_BLOQUEAR_DTB:{
        	t_dtb * dtb = transformarPaqueteADTB(pkg);
        	if(bloquearDTB(dtb))
        	{
        		log_error_mutex(logger, "Hubo un error al bloquear el DTB");
        	}
        	break;
        }
        case CPU_SAFA_ABORTAR_DTB:{
        	t_dtb * dtb = transformarPaqueteADTB(pkg);
        	if(abortarDTB(dtb))
        	{
        		log_error_mutex(logger, "Hubo un error al abortar el DTB.");
        	}
        	break;
        }
        case CPU_SAFA_ABORTAR_DTB_NUEVO:{
			t_dtb * dtb = transformarPaqueteADTB(pkg);
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
        	break;
        case CPU_SAFA_FIN_EJECUCION_DTB:{
        	t_dtb * dtb = transformarPaqueteADTB(pkg);
        	if(abortarDTB(dtb))
			{
				log_error_mutex(logger, "Hubo un error al abortar el DTB.");
			}
        	break;
        }
        case CPU_SAFA_FIN_EJECUCION_X_QUANTUM_DTB:{
        	t_dtb * dtb = transformarPaqueteADTB(pkg);
        	if(finEjecucionPorQuantum(dtb)){
        		log_error_mutex(logger, "Hubo un error al llevar el DTB a la cola de READY por finalizacion de quantum.");
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
		//LA PETICION "ABRIR" SE FINALIZO
        //LA PETICION "FLUSH" SE FINALIZÓ
        //LA PETICION "CREAR ARCHIVO" SE FINALIZÓ
        //LA PETICION "BORRAR ARCHIVO" SE FINALIZÓ
		case DAM_SAFA_CONFIRMACION_DATOS_GUARDADOS:
		case DAM_SAFA_CONFIRMACION_CREAR_ARCHIVO:
		case DAM_SAFA_CONFIRMACION_BORRAR_ARCHIVO:{
			int pid = copyIntFromBuffer(&pkg.data);
			int result = copyIntFromBuffer(&pkg.data);

			if(confirmacionDMA(pid, result)){
				//Error
				log_error_mutex(logger, "No se pudo cargar el proceso pid: %d en memoria", pid);
			}
			log_info_mutex(logger,"Se cargó en memoria y se desbloqueó el proceso pid: %d", pid);

			break;
	   }

        case CPU_SAFA_SIGNAL_RECURSO:{
        	char * recurso = copyStringFromBuffer(&pkg.data);
			hacerSignalDeRecurso(recurso);
			break;
        }
        case CPU_SAFA_WAIT_RECURSO:{
        	char * recurso = copyStringFromBuffer(&pkg.data);
			int pid = copyIntFromBuffer(&pkg.data);
			hacerWaitDeRecurso(recurso,pid);
			break;
        }

        case SOCKET_DISCONECT:
            close(socketFD);
            break;
        default:
            log_warning_mutex(logger, "El mensaje recibido es: %s", codigoIDToString(pkg.code));
            log_warning_mutex(logger, "Ojo, estas recibiendo un mensaje que no esperabas.");
            break;

    }

//    free(pkg.data);

}

void initCpuList(){
	listaCpus = list_create();
}

void hacerSignalDeRecurso(char * recursoSolicitado){

	for(int i =0; i<list_size(listaRecursoAsignados); i++){

		t_recurso * recurso = list_get(listaRecursoAsignados,i);
		int estaEnLaLista = strcmp(recurso->recursoId, recursoSolicitado);
		if(estaEnLaLista == 0){
			t_recurso * recursoUsado = list_remove(listaRecursoAsignados, i);
			if(list_size(recursoUsado->listProcesos)>0){
				//TIENE RECURSOS SOLICITANDOLO -> Se toma el primero y se desbloquea
				int pidSolicitante = (int) list_remove(recursoUsado->listProcesos,0);
				recursoUsado->procesoDuenio = pidSolicitante;

				//Se desbloquea el proceso pid
				desbloquearDTBsegunAlgoritmo(pidSolicitante);
			}else{
				//NO TIENE RECURSOS SOLICITANDOLO
				recursoUsado->procesoDuenio = 0;
			}
			list_add_in_index(listaRecursoAsignados, i, recursoUsado);
		}
	}
}

void hacerWaitDeRecurso(char * recursoSolicitado, int pid){

	int posicion = -1;

	for(int i =0; i<list_size(listaRecursoAsignados); i++){

		t_recurso * recurso = list_get(listaRecursoAsignados,i);
		int estaEnLaLista = strcmp(recurso->recursoId, recursoSolicitado);
		if(estaEnLaLista == 0){
			posicion = i;
			t_recurso * recursoUsado = list_remove(listaRecursoAsignados, posicion);
			if(recursoUsado->procesoDuenio == 0){
				//EXISTE Y NO ESTA SIENDO USADO
				//el recurso no se está utilizando
				recursoUsado->procesoDuenio = pid;
			}else{
				//EXISTE Y ESTA SIENDO USADO
				//Esta siendo usado y lo tengo que bloquear
				list_add(recursoUsado->listProcesos,&pid);

				//Se bloquea el proceso
				pthread_mutex_lock(&mutexEjecutandoList);
				t_dtb * dtb = buscarDTBPorPIDenCola(colaEjecutando,pid);
				pthread_mutex_unlock(&mutexEjecutandoList);
				pasarDTBdeEXECaBLOQUED(dtb);
			}
			list_add_in_index(listaRecursoAsignados, posicion, recursoUsado);
		}
	}


	if(posicion >= 0){
		//NO EXISTE
		//Si no estaba en la lista lo tengo que crear
		t_recurso * recursoCreado = crearRecurso(recursoSolicitado,pid);
		list_add(listaRecursoAsignados, recursoCreado);
	}

}

t_recurso* crearRecurso(char * recurso, int pid){
	t_list * listaProcesos = list_create();
	t_recurso * recursoStruct = malloc(sizeof(t_recurso));
	recursoStruct->recursoId =recurso;
	recursoStruct->procesoDuenio = pid;
	recursoStruct->listProcesos = listaProcesos;
	return recursoStruct;
}
