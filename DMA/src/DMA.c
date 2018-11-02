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
	cargarArchivoDeConfig(argv[1]);
	iniciarConexionesDelDMA();

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
						"Cambios en Listener del DMA, se gestionara la conexion correspondiente");
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
					log_error_mutex(logger, "No se pudo recibir el mensaje del CPU");
					//handlerDisconnect(i);
				} else {
					manejarSolicitudDelCPU(pkg, i);
				}

			}
		}
	}
}

void manejarSolicitudDelCPU(t_package pkg, int socketFD) {

	//SE DETERMINA CUAL ES LA SOLICITUD DEL CPU Y SE REALIZA LA ACCION CORRESPONDIENTE
    switch (pkg.code) {


    	/*
    	 * Se conectó una CPU
    	 *
    	 */
        case CPU_FM9_CONNECT:
        	cpusConectadas ++;
			log_info_mutex(logger,"Se ha conectado el CPU. CPU conectadas: %d", cpusConectadas);
            break;

		/*
		 * Se carga un escriptorio en memoria
		 * Params: path del archivo y pid del DTB
		 *
		 */

        case CPU_DAM_BUSQUEDA_ESCRIPTORIO:
        	if(leerEscriptorio(pkg,socketFD)){
				log_error_mutex(logger, "Hubo un error en la inicializacion del escriptorio");
				break;
			}
        	log_info_mutex(logger, "Se cargo el escriptorio en MDJ y se notificó a S-AFA");
			break;

		/*
		 * Procedimiento para abrir un archivo (Escritura en el FM9)
		 *	Params: Path del archivo a cargar
		 *
		 */
        case CPU_DAM_ABRIR_ARCHIVO:
        	//Recibe un pkg con el path de archivo a abrir
        	if(abrirArchivo(pkg,socketFD)){
        		log_error_mutex(logger, "Hubo un error al abrir el archivo: %s", pkg.data);
				break;
        	}
        	log_info_mutex(logger, "Se abrió el archivo correctamente");
        	break;

        /*
        	Procedimiento para escribir en MDJ
        *   Params:
        *		path --> donde guardar los datos
        *		posicion en memoria -> donde buscar los datos
        *
        */
        case CPU_DAM_FLUSH:
        	if(hacerFlush(pkg,socketFD)){
				log_error_mutex(logger, "Hubo un error al abrir el archivo: %s",
						pkg.data);
				break;
        	}
        	log_info_mutex(logger, "Se realizó el flush correctamente");
        	break;


        case CPU_DAM_CREAR:
        	if(crearArchivo(pkg,socketFD)){
				log_error_mutex(logger, "Hubo un error al abrir el archivo: %s",
						pkg.data);
				break;
			}

        	log_info_mutex(logger, "Se creó el archivo correctamente");
        	break;

        case CPU_DAM_BORRAR:
        	if(borrarArchivo(pkg,socketFD)){
				log_error_mutex(logger, "Hubo un error al abrir el archivo: %s",
						pkg.data);
				break;
			}

        	log_info_mutex(logger, "Se borró el archivo correctamente");
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

void cargarArchivoDeConfig(char * pathConfig) {

	log_info_mutex(logger, "Archivo de configuraciones cargado correctamente");

	if (pathConfig != NULL){
	configDMA = cargarConfiguracion(pathConfig, DAM, logger->logger);
	}
	else{
		log_error_mutex(logger, "No hay un path correcto a un archivo de configuracion");
		exit_gracefully(ERROR_PATH_CONFIG);
	}
	if (configDMA == NULL){
		log_error_mutex(logger, "Error en el archivo de configuracion");
		exit_gracefully(ERROR_CONFIG);
	}

}

void configure_logger() {
	//logger = log_create("DAM.log", "DAM", true, LOG_LEVEL_INFO);
	logger = log_create_mutex("DAM.log", "DAM", true, LOG_LEVEL_TRACE);
	log_info_mutex(logger, "Inicia proceso: Diego Armando Maradona (DAM)");
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
