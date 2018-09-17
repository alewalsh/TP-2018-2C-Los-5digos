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
	configure_logger();
	cargarArchivoDeConfig();
	iniciarHilosDelDMA();

	exit_gracefully(0);
	return EXIT_SUCCESS;
}

void cargarArchivoDeConfig() {
	configDMA = cargarConfiguracion("config.cfg", DAM, logger);
	log_info(logger, "Archivo de configuraciones cargado correctamente");
}

void configure_logger() {
	logger = log_create("DAM.log", "DAM", true, LOG_LEVEL_INFO);
	log_info(logger, "Inicia proceso: Diego Armando Maradona (DAM)");
}

void iniciarHilosDelDMA() {
	log_info(logger,"Creando sockets");

	int res;
	//TODO: Manejar conexiones con hilos
	res = pthread_create(&hiloSafa,NULL,(void *)conectarseConSafa(), NULL);
	if(res > 0){
		log_error(logger, "Error al crear el hilo del S-Afa");
		exit_gracefully(1);
	}

	res = pthread_create(&hiloMdj,NULL,(void *)conectarseConMdj(), NULL);
	if(res > 0){
			log_error(logger, "Error al crear el hilo del MDJ");
			exit_gracefully(1);
	}

	res = pthread_create(&hiloFm9,NULL,(void *)conectarseConFm9(), NULL);
	if(res > 0){
			log_error(logger, "Error al crear el hilo del FM9");
			exit_gracefully(1);
	}

	pthread_join(hiloSafa, NULL);
	pthread_join(hiloMdj, NULL);
	pthread_join(hiloFm9, NULL);

	log_info(logger, "Los hilos finalizaron correctamente");

}

void * conectarseConSafa(){
	//TODO: Crear hilo que quede a la escucha de S-Afa
	socketSafa = inicializarTSocket(1,logger);
	int res;
	res = cargarSoket(configDMA->puertoSAFA,&configDMA->ipSAFA,&socketSafa,logger);
	if(res > 0){
		log_error(logger, "Error al cargar el socket de S-Afa");
		exit_gracefully(1);
	}
	res = enviarHandshake(1,1234,1234,&logger);
	if(res > 0){
		log_error(logger, "Error al hacer el handkshake con S-afa");
		exit_gracefully(1);
	}else{
		log_info(logger, "Se realizó la conexión con el proceso S-Afa");
	}

	return NULL;
}

void * conectarseConMdj(){
	//TODO: Crear hilo que quede a la escucha de MDJ
	socketMdj = inicializarTSocket(2,logger);
	int res;
	res = cargarSoket(configDMA->puertoMDJ,&configDMA->ipMDJ,&socketMdj,logger);
	if(res > 0){
		log_error(logger, "Error al cargar el socket de MDJ");
		exit_gracefully(1);
	}
	res = enviarHandshake(1,1324,1324,&logger);
	if(res > 0){
		log_error(logger, "Error al hacer el handkshake con MDJ");
		exit_gracefully(1);
	}else{
		log_info(logger, "Se realizó la conexión con el proceso MDJ");
	}

	return NULL;
}

void * conectarseConFm9(){
	//TODO: Crear hilo que quede a la escucha de FM9
	socketFm9 = inicializarTSocket(3,logger);
	int res;
	res = cargarSoket(configDMA->puertoFM9,&configDMA->ipFM9,&socketFm9,logger);
	if(res > 0){
		log_error(logger, "Error al cargar el socket de FM9");
		exit_gracefully(1);
	}
	res = enviarHandshake(1,1432,1432,&logger);
	if(res > 0){
		log_error(logger, "Error al hacer el handkshake con FM9");
		exit_gracefully(1);
	}else{
		log_info(logger, "Se realizó la conexión con el proceso FM9");
	}

	return NULL;
}

//Funcion para cerrar el programa
void exit_gracefully(int return_nr) {

	bool returnCerrarSockets = cerrarSockets();
	if(return_nr > 0 ){
		log_error(logger, "Fin del proceso: DAM");
	}else{
		log_info(logger, "Fin del proceso: DAM");
	}

	log_destroy(logger);
	exit(return_nr);
}

bool cerrarSockets(){
	int ret1 = close(socketSafa->socket);
	int ret2 = close(socketFm9->socket);
	int ret3 = close(socketMdj->socket);

	return ret1 < 0 || ret2 < 0 || ret3 < 0;
}
