/*
 * handlerConexiones.c
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#include "handlerConexiones.h"
#include <configuracion.h>
#include <socket.h>
#include <mutex_log.h>
#include <structCommons.h>
#include "SAFA.h"


void manejarConexiones(){

    int socketListen, i,nuevoFd;
    uint16_t handshake;

    //Creo el socket y me quedo escuchando
	if (escuchar(conf->puerto, &socketListen, logger->logger)) {
		liberarRecursos();
		pthread_exit(NULL);
    }
    log_trace_mutex(logger, "El socket de escucha de SAFA es: %d", socketListen);

    addNewSocketToMaster(socketListen);

    while (1) {
        updateReadset();

        //Hago un select sobre el conjunto de sockets activo
        int result = select(getMaxfd() + 1, &readset, NULL, NULL, NULL);
        if (result == -1) {
            log_error_mutex(logger, "Error en el select: %s", strerror(errno));
            exit(1);
        }

        log_trace_mutex(logger, "El valor del select es: %d", result);
//        log_trace_mutex(logger, "Arranco de nuevo con el select");
        log_trace_mutex(logger, "Analizo resultado del select para ver quien me hablo");

        for (i = 0; i <= getMaxfd(); i++) {

        	if (isSetted(i)) { // ¡¡tenemos datos!!

        		if (i == socketListen) {
                    // gestionar nuevas conexiones
                    log_trace_mutex(logger, "Cambios en Listener de SAFA, se gestionara la conexion correspondiente");
                    if (acceptConnection(socketListen, &nuevoFd, SAFA_HSK, &handshake, logger->logger)) {
                        log_error_mutex(logger, "No se acepto la nueva conexion solicitada");
                    } else {
                        // añadir al conjunto maestro
                        log_trace_mutex(logger, "Se acepto la nueva conexion solicitada en el SELECT");
                        addNewSocketToMaster(nuevoFd);
                    }
                } else {
                    // gestionar datos de un cliente
                    //if (recibir(i, &pkg, logger->logger)) {
                    //    log_error_mutex(logger, "No se pudo recibir el mensaje");
                    //    handlerDisconnect(i);
                    //} else {
                    //    manageRequest(pkg, i);
                    //}

                }
            }
        }
    }

}
