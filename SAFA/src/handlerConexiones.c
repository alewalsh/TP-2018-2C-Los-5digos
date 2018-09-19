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
#include "SAFA.h"


void manejarConexiones(){

    int socketListen, i,nuevoFd;
    uint16_t handshake;
    t_package pkg;

    int estadoSAFA = 2;  // 2 corrupto - 0 operativo
	log_trace_mutex(logger, "Se inicializa SAFA en estado Corrupto");

    //Creo el socket y me quedo escuchando
	if (escuchar(conf->puerto, &socketListen, logger->logger)) {
		liberarRecursos();
		pthread_exit(NULL);
    }

	log_trace_mutex(logger, "El socket de escucha de SAFA es: %d", socketListen);
    log_info_mutex(logger, "El socket de escucha de SAFA es: %d", socketListen);

    addNewSocketToMaster(socketListen);


    //TODO: Voy a tener que agregar que no se empiece a planificar hasta esto
    while(!estadoSAFA == 0){
    	//Escucho conexiones. Cuando se me conecta el DAM bajo estado a 0, y cuando hay 1 cpu
    	// tmb bajo a 0
        if (acceptConnection(socketListen, &nuevoFd, SAFA_HSK, &handshake, logger->logger)) {
            log_error_mutex(logger, "No se acepta la conexion");
        }

        switch (handshake) {
            case DAM_HSK:
            	log_trace_mutex(logger, "Se me conecto el DAM, socket: %d", nuevoFd);
                addNewSocketToMaster(nuevoFd);
            	estadoSAFA--;
                break;
            case CPU_HSK:
            	log_trace_mutex(logger, "Se me conecto un CPU, socket: %d", nuevoFd);
                addNewSocketToMaster(nuevoFd);
            	estadoSAFA--;
                break;
            default:
                log_warning_mutex(logger, "Se me quizo conectar alguien que no espero");
                close(nuevoFd);
                break;
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
                        //manageRequest(pkg, i);
                    }

                }
            }
        }
    }

}
