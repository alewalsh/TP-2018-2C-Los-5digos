/*
 ============================================================================
 Name        : DMA.c
 Author      : Franco Lopez
 Version     :
 Copyright   : Your copyright notice
 Description : Proyecto destinado al DMA
 ============================================================================
 */

#include "DMA.h"

int main(int argc, char ** argv) {

	initVariables();
	configure_logger();
	cargarArchivoDeConfig();
	iniciarHilosDelDMA();

	while (1) {
		aceptarConexionesDelCpu();
	}

	exit_gracefully(0);
}

void aceptarConexionesDelCpu() {
	t_package pkg;
	// Recibo y acepto las conexiones del cpu
	updateReadset();

	//Hago un select sobre el conjunto de sockets activo
	int result = select(getMaxfd() + 1, &readset, NULL, NULL, NULL);
	if (result == -1) {
		log_error_mutex(logger, "Error en el select: %s", strerror(errno));
		exit(1);
	}

	log_trace_mutex(logger, "El valor del select es: %d", result);
	log_trace_mutex(logger, "Analizo resultado del select para ver quien me hablo");

	for (int i = 0; i <= getMaxfd(); i++) {

		if (isSetted(i)) { // ¡¡tenemos datos!!

			if (i == socketEscucha) {
				// CAMBIOS EN EL SOCKET QUE ESCUCHA, acepto las nuevas conexiones
				log_trace_mutex(logger,
						"Cambios en Listener de CPU, se gestionara la conexion correspondiente");
				if (acceptConnection(socketEscucha, &nuevoFd, DAM_HSK, &handshake,
						logger->logger)) {
					log_error_mutex(logger,
							"No se acepto la nueva conexion solicitada");
				} else {
					// añadir al conjunto maestro
					log_trace_mutex(logger,
							"Se acepto la nueva conexion solicitada en el SELECT");
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

void manejarSolicitud(t_package pkg, int socketFD) {

    switch (pkg.code) {
        case CPU_FM9_CONNECT:
			printf("Se ha conectado el CPU.");
//            if (esiConnection(socketFD, pkg, logger)) {
//                log_error_mutex_mutex(logger, "Hubo un error en la conexion con la CPU");
//                break;
//            }
//            sem_post(&sem_newEsi);
            break;

        case CPU_DAM_BUSQUEDA_ESCRIPTORIO:
        	printf("Se debe buscar un escriptorio en mdj y cargar en fm9");
        	//TODO
        	break;

		case DAM_FM9_CONNECT:
			printf("Se ha conectado el DAM.");
//			if (esiConnection(socketFD, pkg, logger)) {
//				log_error_mutex_mutex(logger, "Hubo un error en la conexion con el DAM");
//				break;
//			}
//            sem_post(&sem_newEsi);
			break;
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



void initVariables() {
	cpusConectadas = 0;
}

void cargarArchivoDeConfig() {
	configDMA = cargarConfiguracion("config.cfg", DAM, logger->logger);
	log_info_mutex(logger, "Archivo de configuraciones cargado correctamente");
}

void configure_logger() {
	//logger = log_create("DAM.log", "DAM", true, LOG_LEVEL_INFO);
	logger = log_create_mutex("DAM.log", "DAM", true, LOG_LEVEL_TRACE);
	log_info_mutex(logger, "Inicia proceso: Diego Armando Maradona (DAM)");
}

void iniciarHilosDelDMA() {
	log_info_mutex(logger, "Creando sockets");

	int res;
	res = conectarseConSafa();
	if (res > 0) {
		log_error_mutex(logger, "Error al crear el hilo del S-Afa");
		exit_gracefully(1);
	}else{
		log_info_mutex(logger, "Se realizó la conexion del S-Afa");
	}

	res = conectarseConMdj();
	if (res > 0) {
		log_error_mutex(logger, "Error al crear el hilo del MDJ");
		exit_gracefully(1);
	}else{
		log_info_mutex(logger, "Se realizó la conexion del MDJ");
	}


	res = conectarseConFm9();
	if (res > 0) {
		log_error_mutex(logger, "Error al crear el hilo del FM9");
		exit_gracefully(1);
	}else{
		log_info_mutex(logger, "Se realizó la conexion del FM9");
	}


	res = conectarseConCPU();
	if (res > 0) {
		log_error_mutex(logger, "Error al crear el hilo del CPU");
		exit_gracefully(1);
	}else{
		log_info_mutex(logger, "Se realizó la conexion del CPU");
	}


	//TODO: Manejar conexiones con hilos
//	res = pthread_create(&hiloSafa,NULL,(void *)conectarseConSafa(), NULL);
//	if(res > 0){
//		log_error_mutex(logger, "Error al crear el hilo del S-Afa");
//		exit_gracefully(1);
//	}

//	res = pthread_create(&hiloMdj,NULL,(void *)conectarseConMdj(), NULL);
//	if(res > 0){
//			log_error_mutex(logger, "Error al crear el hilo del MDJ");
//			exit_gracefully(1);
//	}

//	res = pthread_create(&hiloFm9,NULL,(void *)conectarseConFm9(), NULL);
//	if(res > 0){
//			log_error_mutex(logger, "Error al crear el hilo del FM9");
//			exit_gracefully(1);
//	}

//	res = pthread_create(&hiloFm9,NULL,(void *)conectarseConCPU(), NULL);
//		if(res > 0){
//				log_error_mutex(logger, "Error al crear el hilo del FM9");
//				exit_gracefully(1);
//	}

//	pthread_join(hiloSafa, NULL);
//	pthread_join(hiloMdj, NULL);
//	pthread_join(hiloFm9, NULL);
//	pthread_join(hiloCPU, NULL);

	log_info_mutex(logger, "Los hilos finalizaron correctamente");

}

/*FUNCION DEL HILO MDJ
 * Crea UN hilo que queda conectado y a la escucha del S-Afa
 */
void * conectarseConSafa() {

	socketSafa = malloc(sizeof(int));
	conectarYenviarHandshake(configDMA->puertoSAFA, configDMA->ipSAFA,
			socketSafa, SAFA_HSK, t_socketSafa);
	return 0;
}

/*FUNCION DEL HILO MDJ
 * Crea UN hilo que queda conectado al MDJ
 */
void * conectarseConMdj() {

	socketMdj = malloc(sizeof(int));
	conectarYenviarHandshake(configDMA->puertoMDJ, configDMA->ipMDJ, socketMdj,
			MDJ_HSK, t_socketMdj);
	return 0;
}

/*FUNCION DEL HILO FM9
 * Crea UN hilo que queda conectado al FM9
 */
void * conectarseConFm9() {

	socketFm9 = malloc(sizeof(int));
	conectarYenviarHandshake(configDMA->puertoFM9, configDMA->ipFM9, socketFm9,
			FM9_HSK, t_socketFm9);
	return 0;
}

void * conectarseConCPU() {
	socketCPU = malloc(sizeof(int));
	conectarYRecibirHandshake(configDMA->puertoDAM);
	return 0;
}

void conectarYenviarHandshake(int puerto, char *ip, int * socket, int handshakeProceso, t_socket* TSocket) {
	if (!cargarSocket(puerto, ip, socket, logger->logger)) {
		TSocket = inicializarTSocket(*socket, logger->logger);
		enviarHandshake(TSocket->socket, DAM_HSK, handshakeProceso, logger->logger);
	} else {
		log_error_mutex(logger, "Error al conectarse al %s", enumToProcess(handshakeProceso));
		exit_gracefully(ERROR_SOCKET);
	}
}

void conectarYRecibirHandshake(int puertoEscucha) {

//	uint16_t handshake;
	if (escuchar(puertoEscucha, &socketEscucha, logger->logger)) {
		//liberar recursos/
		exit_gracefully(1);
	}
	log_trace_mutex(logger, "El socket de escucha del DMA es: %d", socketEscucha);
    log_info_mutex(logger, "El socket de escucha de DMA es: %d", socketEscucha);
	addNewSocketToMaster(socketEscucha);

	printf("Se conecto el CPU");
	// pthread_create(&threadDAM, &tattr, (void *) esperarInstruccionDAM, NULL);
}

char * enumToProcess(int proceso) {
	char * nombreProceso = "";
	switch (proceso) {
	case FM9_HSK:
		nombreProceso = "FM9";
		break;
	case MDJ_HSK:
		nombreProceso = "MDJ";
		break;
	case SAFA_HSK:
		nombreProceso = "SAFA";
		break;
	case DAM_HSK:
		nombreProceso = "DMA";
		break;
	default:
		log_error_mutex(logger, "Enum proceso error.");
		break;
	}
	if (strcmp(nombreProceso, "") == 0)
		exit_gracefully(-1);
	return nombreProceso;
}

//Funcion para cerrar el programa
void exit_gracefully(int return_nr) {

	bool returnCerrarSockets = cerrarSockets();
	if (return_nr > 0 || returnCerrarSockets) {
		log_error_mutex(logger, "Fin del proceso: DAM");
	} else {
		log_info_mutex(logger, "Fin del proceso: DAM");
	}

	log_destroy_mutex(logger);
	exit(return_nr);
}

bool cerrarSockets() {
	free(t_socketSafa);
	free(t_socketFm9);
	free(t_socketMdj);
	int ret1 = close(*socketSafa);
	int ret2 = close(*socketFm9);
	int ret3 = close(*socketMdj);

	return ret1 < 0 || ret2 < 0 || ret3 < 0;
}
