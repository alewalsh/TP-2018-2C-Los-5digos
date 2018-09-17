#include "FileSystem.h"

int main(int argc, char ** argv) {
	inicializarCPU(argv[1]);
//	inicializarConexion();

	exit_gracefully(FIN_EXITOSO);
}

void inicializarCPU(char * pathConfig){
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
}

//void inicializarConexion(){
//	int socketEscucha;
//	socketDAM = inicializarTSocket(socketEscucha, logger);
//	escuchar(configuracion->puertoMDJ, socketDAM, logger);
//
//	//esto pasarlo a un thead propio
//	esperarInstruccionDAM(socketDAM);
//}
//
//void esperarInstruccionDAM(int socketEscucha){
//	while(1){
//		int socketCliente;
//		aceptar(socketEscucha, socketCliente, logger);
//
//		t_package instruccion;
//		recibir(socketCliente, instruccion, logger);
//
//		//switch segun contenido de instruccion
//	}
//}

void exit_gracefully(int error){
	if (error != ERROR_PATH_CONFIG)
	{
		free(configuracion);
	}
	log_destroy(logger);
	exit(error);
}
