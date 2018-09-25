#include "FileSystem.h"

int main(int argc, char ** argv) {
	inicializarMDJ(argv[1]);
	//cargarFIFA(); //TODO proxixma entrega
	inicializarConexion();
	//inincializarConsola()

	esperarInstruccionDAM();

	exit_gracefully(FIN_EXITOSO);
}

void inicializarMDJ(char * pathConfig){

	logger = log_create("MDJ.log", "MDJ", true, LOG_LEVEL_INFO);
	if (pathConfig != NULL){
		configuracion = cargarConfiguracion(pathConfig, MDJ, logger);
	}
	else{
		log_error(logger, "No hay un path correcto a un archivo de configuracion");
		exit_gracefully(ERROR_PATH_CONFIG);
	}
	if (configuracion == NULL){
		log_error(logger, "Error en el archivo de configuracion");
		exit_gracefully(ERROR_CONFIG);
	}

	//Imprimo lo leido en config.cfg
	printf("Configuracion inicial: \n");
	log_info(logger, "Puerto = %d", configuracion->puertoMDJ);
	log_info(logger, "Punto montaje = %s", configuracion->puntoMontaje);
	log_info(logger, "Retardo = %d", configuracion->retardo);
	log_info(logger, "IP propia = %s", configuracion->ip_propia);
}

void inicializarConexion(){
	int * socketPropio;
    uint16_t handshake;
	if (escuchar(configuracion->puertoMDJ, &socketPropio, logger)) {
		exit_gracefully(1);
	}
	if (acceptConnection(socketPropio, &socketDAM, MDJ_HSK, &handshake, logger)) {
		log_error(logger, "No se acepta la conexion");
	}
	printf("Se conecto el DAM");
   // pthread_create(&threadDAM, &tattr, (void *) esperarInstruccionDAM, NULL);
}

void esperarInstruccionDAM(){
	while(!getExit()){

//		recibir(socketEscucha, /*package*/,logger);

		responderDAM();

	}
}


void exit_gracefully(int error){
	if (error != ERROR_PATH_CONFIG)
	{
		free(configuracion);
	}
	log_destroy(logger);
	exit(error);
}
