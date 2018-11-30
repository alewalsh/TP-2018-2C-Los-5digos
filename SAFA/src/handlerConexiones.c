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
//#include "SAFA.h"
#include "funcionesSAFA.h"


void manejarConexiones(){

    int socketListen, i,nuevoFd;
    uint16_t handshake;
    t_package pkg;

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

                char *buffer = copyIntToBuffer(&buffer,conf->quantum);
            	int size;
            	char *keyCompress = compressKey(buffer, &size);
            	if(enviar(nuevoFd, SAFA_CPU_QUANTUM, keyCompress, size, logger->logger))
            	{
            		log_error_mutex(logger, "No se pudo enviar el quantum al CPU.");
            		free(keyCompress);
            	}
        		free(keyCompress);

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
        case CPU_SAFA_BLOQUEAR_DUMMMY:
        	//Se bloquea el dummy
        	bloquearDummy();
        	sem_post(&semDummy);
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
        	//TODO: VER QUE SE HACE CON EL PLANIFICADOR DE LARGO PLAZO CUANDO SE INCIALIZÓ EL SCRIPTORIO
        	break;
        }

		//LA PETICION "ABRIR" SE FINALIZO
        //LA PETICION "FLUSH" SE FINALIZÓ
        //LA PETICION "CREAR ARCHIVO" SE FINALIZÓ
        //LA PETICION "BORRAR ARCHIVO" SE FINALIZÓ
		case (DAM_SAFA_CONFIRMACION_PID_CARGADO||
		DAM_SAFA_CONFIRMACION_DATOS_GUARDADOS||
		DAM_SAFA_CONFIRMACION_CREAR_ARCHIVO||
		DAM_SAFA_CONFIRMACION_BORRAR_ARCHIVO):{
			int pid = copyIntFromBuffer(&pkg.data);
			int result = copyIntFromBuffer(&pkg.data);

			if(confirmacionDMA(pid, result)){
				//Error
				log_error_mutex(logger, "No se pudo cargar el proceso pid: %d en memoria", pid);
			}
			log_info_mutex(logger,"Se cargó en memoria y se desbloqueó el proceso pid: %d", pid);

			break;
	   }

        case CPU_SAFA_SIGNAL_RECURSO: break;
        case CPU_SAFA_WAIT_RECURSO: break;

        case SOCKET_DISCONECT:
            close(socketFD);
            break;
        default:
            log_warning_mutex(logger, "El mensaje recibido es: %s", codigoIDToString(pkg.code));
            log_warning_mutex(logger, "Ojo, estas recibiendo un mensaje que no esperabas.");
            break;

    }

    free(pkg.data);

}

void initCpuList(){
	listaCpus = list_create();
}
