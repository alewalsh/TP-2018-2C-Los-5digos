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
	logger = log_create_mutex("FM9.log", "FM9", true, LOG_LEVEL_INFO);
	config = cargarConfiguracion(argv[1], FM9, logger->logger);
	storage = malloc(config->tamMemoria);
	manejarConexiones();
	free(storage);
	return EXIT_SUCCESS;
}

void manejarConexiones(){
	int socketListen, i,nuevoFd;
	uint16_t handshake;
	t_package pkg;

	printf("%d",config->puertoFM9);
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
            break;
		case DAM_FM9_CONNECT:
			printf("Se ha conectado el DAM.");
			break;
		case DAM_FM9_CARGAR_ESCRIPTORIO:
			if(cargarEscriptorioSegunEsquemaMemoria(pkg,socketFD)){
				log_error_mutex(logger,"Error al cargar el escriptorio en la memoria");
			}
			break;

        case DAM_FM9_GUARDARLINEA:
        	if(guardarLineaSegunEsquemaMemoria(pkg,socketFD)){
				log_error_mutex(logger,"Error al guardar la data recibida en memoria");
			}
			break;

        case DAM_FM9_RETORNARlINEA:
        	if(retornarLineaSolicitada(pkg,socketFD)){
				log_error_mutex(logger,"Error al retornar la memoria solicitada");
			}
			break;
        case SOCKET_DISCONECT:
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
//--------------------------------------GUARDAR DATOS EN MEMORIA SEGUN ESQUEMA ELEGIDO

int guardarLineaSegunEsquemaMemoria(t_package pkg, int socketSolicitud){
	switch (config->modoEjecucion){
	case SEG:

		if(ejecutarGuardarEsquemaSegmentacion(pkg)){
			log_error_mutex(logger,"Error al guardar la linea recibida en Memoria. Esquema: SEG");
			//ENVIAR ERROR AL DMA (socketSolicitud)
		}else{
			log_info_mutex(logger, "Se cargó correctamente la linea recibida en Memoria");
		}
	    break;

	case TPI:
		if(ejecutarGuardarEsquemaTPI(pkg)){
			log_error_mutex(logger,"Error al guardar la linea recibida en Memoria. Esquema: TPI");
			//ENVIAR ERROR AL DMA (socketSolicitud)
		}else{
			log_info_mutex(logger, "Se cargó correctamente la linea recibida en Memoria");
		}
	    break;

	case SPA:
		if(ejecutarGuardarEsquemaSegPag(pkg)){
			log_error_mutex(logger,"Error al guardar la linea recibida en Memoria. Esquema: SPA");
			//ENVIAR ERROR AL DMA (socketSolicitud)
		}else{
			log_info_mutex(logger, "Se cargó correctamente la linea recibida en Memoria");
		}
	    break;

	default:
		log_warning_mutex(logger, "No se especifico el esquema para el guardado de lineas");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


int ejecutarGuardarEsquemaSegmentacion(t_package pkg){
	//logica de segmentacion pura
	return EXIT_SUCCESS;

}

int ejecutarGuardarEsquemaTPI(t_package pkg){
	//logica de tabla de paginas invertida
	return EXIT_SUCCESS;
}

int ejecutarGuardarEsquemaSegPag(t_package pkg){
	//logica de segmentacion paginada
	return EXIT_SUCCESS;
}

//------------------------------CARGAR ESCRIPTORIO EN MEMORIA SEGUN ESQUEMA ELEGIDO
int cargarEscriptorioSegunEsquemaMemoria(t_package pkg, int socketSolicitud){
	switch (config->modoEjecucion){
	case SEG:
		ejecutarCargarEsquemaSegmentacion(pkg);
	    break;

	case TPI:
		ejecutarCargarEsquemaTPI(pkg);
	    break;

	case SPA:
		ejecutarCargarEsquemaSegPag(pkg);
	    break;

	default:
		log_warning_mutex(logger, "No se especifico el esquema para el guardado de lineas");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int ejecutarCargarEsquemaSegmentacion(t_package pkg){
	//logica de segmentacion pura
	return EXIT_SUCCESS;
}

int ejecutarCargarEsquemaTPI(t_package pkg){
	//logica de tabla de paginas invertida
	return EXIT_SUCCESS;
}

int ejecutarCargarEsquemaSegPag(t_package pkg){
	//logica de segmentacion paginada
	return EXIT_SUCCESS;
}

//--------------------------------------------------RETORNAR DATOS DE MEMORIA

int retornarLineaSolicitada(t_package pkg, int socketSolicitud){

	//logica para retornar una linea pedida
	return EXIT_SUCCESS;
}

