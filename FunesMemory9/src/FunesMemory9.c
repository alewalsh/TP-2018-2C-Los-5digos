/*
 ============================================================================
 Name        : FunesMemory9.c
 Author      : Nicolas Barrelier
 Version     :
 Copyright   : 
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "FunesMemory9.h"

int main(int argc, char** argv) {
	logger = log_create_mutex("CPU.log", "CPU", true, LOG_LEVEL_INFO);
	config = cargarConfiguracion(argv[1], CPU, logger->logger);

	manejarConexiones();
	return EXIT_SUCCESS;
}

void manejarConexiones(){
	int socketListen, i,nuevoFd;
	uint16_t handshake;
	t_package pkg;

	//Creo el socket y me quedo escuchando
	if (escuchar(config->puertoFM9, &socketListen, logger->logger)) {
		liberarRecursos();
		pthread_exit(NULL);
	}

	log_trace_mutex(logger, "El socket de escucha de FM9 es: %d", socketListen);
	log_info_mutex(logger, "El socket de escucha de FM9 es: %d", socketListen);

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
		log_trace_mutex(logger, "Analizo resultado del select para ver quien me hablo");

		for (i = 0; i <= getMaxfd(); i++) {

			if (isSetted(i)) { // ¡¡tenemos datos!!

				if (i == socketListen) {
					// CAMBIOS EN EL SOCKET QUE ESCUCHA, acepto las nuevas conexiones
					log_trace_mutex(logger, "Cambios en Listener de FM9, se gestionara la conexion correspondiente");
					if (acceptConnection(socketListen, &nuevoFd, FM9_HSK, &handshake, logger->logger)) {
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

void liberarRecursos()
{
	pthread_mutex_destroy(&mutexMaster);
	pthread_mutex_destroy(&mutexReadset);
	log_destroy_mutex(logger);
	freeConfig(config, FM9);
}

void manejarSolicitud(t_package pkg, int socketFD) {

    switch (pkg.code) {
        case CPU_FM9_CONNECT:
			printf("Se ha conectado el CPU.");
//            if (esiConnection(socketFD, pkg, logger)) {
//                log_error_mutex(logger, "Hubo un error en la conexion con la CPU");
//                break;
//            }
//            sem_post(&sem_newEsi);
            break;
		case DAM_FM9_CONNECT:
			printf("Se ha conectado el DAM.");
//			if (esiConnection(socketFD, pkg, logger)) {
//				log_error_mutex(logger, "Hubo un error en la conexion con el DAM");
//				break;
//			}
//            sem_post(&sem_newEsi);
			break;
//        case COORD_PLAN_BLOCK:
//            //log_info_mutex(logger, "El coordinador me pide que bloquee un recurso");
//            if (blockKey(socketFD, pkg, logger)) {
//                log_error_mutex(logger, "No se pudo completar la operacion de bloqueo");
//            }
//            break;
//        case COORD_PLAN_STORE:
//            //log_info_mutex(logger, "El coordinador me pide que desbloque un recurso");
//            if (storeKey(socketFD, pkg, logger)) {
//                log_error_mutex(logger, "No se pudo completar la operacion de desbloqueo");
//            }
//            break;
        case SOCKET_DISCONECT:
//            handlerDisconnect(socketFD);
            close(socketFD);
            deleteSocketFromMaster(socketFD);
            break;
        default:
            log_warning_mutex(logger, "El mensaje recibido es: %s", codigoIDToString(pkg.code));
            log_warning_mutex(logger, "Ojo, estas recibiendo un mensaje que no esperabas.");
            break;

    }

    free(pkg.data);

}
