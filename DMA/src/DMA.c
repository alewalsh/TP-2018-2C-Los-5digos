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
	res = conectarseConSafa();
	if (res > 0)
	{
		log_error(logger, "Error al crear el hilo del S-Afa");
		exit_gracefully(1);
	}

	res = conectarseConMdj();
	if (res > 0)
	{
		log_error(logger, "Error al crear el hilo del MDJ");
		exit_gracefully(1);
	}

	res = conectarseConFm9();
	if (res > 0)
	{
		log_error(logger, "Error al crear el hilo del FM9");
		exit_gracefully(1);
	}

	res = conectarseConCPU();
	if (res > 0)
	{
		log_error(logger, "Error al crear el hilo del CPU");
		exit_gracefully(1);
	}

	//TODO: Manejar conexiones con hilos
//	res = pthread_create(&hiloSafa,NULL,(void *)conectarseConSafa(), NULL);
//	if(res > 0){
//		log_error(logger, "Error al crear el hilo del S-Afa");
//		exit_gracefully(1);
//	}

//	res = pthread_create(&hiloMdj,NULL,(void *)conectarseConMdj(), NULL);
//	if(res > 0){
//			log_error(logger, "Error al crear el hilo del MDJ");
//			exit_gracefully(1);
//	}

//	res = pthread_create(&hiloFm9,NULL,(void *)conectarseConFm9(), NULL);
//	if(res > 0){
//			log_error(logger, "Error al crear el hilo del FM9");
//			exit_gracefully(1);
//	}

//	res = pthread_create(&hiloFm9,NULL,(void *)conectarseConCPU(), NULL);
//		if(res > 0){
//				log_error(logger, "Error al crear el hilo del FM9");
//				exit_gracefully(1);
//	}

//	pthread_join(hiloSafa, NULL);
//	pthread_join(hiloMdj, NULL);
//	pthread_join(hiloFm9, NULL);
//	pthread_join(hiloCPU, NULL);

	log_info(logger, "Los hilos finalizaron correctamente");

}

/*FUNCION DEL HILO MDJ
 * Crea UN hilo que queda conectado y a la escucha del S-Afa
 */
void * conectarseConSafa(){

	conectarYenviarHandshake(configDMA->puertoSAFA,configDMA->ipSAFA,
			socketSafa,SAFA_HSK,t_socketSafa);
	return 0;
}

/*FUNCION DEL HILO MDJ
 * Crea UN hilo que queda conectado al MDJ
 */
void * conectarseConMdj(){

	conectarYenviarHandshake(configDMA->puertoMDJ,configDMA->ipMDJ,
			socketMdj,MDJ_HSK,t_socketMdj);
	return 0;
}

/*FUNCION DEL HILO FM9
 * Crea UN hilo que queda conectado al FM9
 */
void * conectarseConFm9(){

	conectarYenviarHandshake(configDMA->puertoFM9,configDMA->ipFM9,
			socketFm9,FM9_HSK,t_socketFm9);
	return 0;
}

void * conectarseConCPU(){

	conectarYRecibirHandshake(configDMA->puertoDAM,configDMA->ipDAM,
			socketFm9,CPU_HSK);
	return 0;
}

void conectarYenviarHandshake(int puerto, char *ip, int * socket, int handshakeProceso, t_socket* TSocket){
	cargarSocket(puerto,ip,&socket,logger);
	if (socket != 0)
	{
		TSocket = inicializarTSocket(socket, logger);
		enviarHandshake(TSocket->socket,DAM_HSK,handshakeProceso,logger);
	}
	else
	{
		log_error(logger, "Error al conectarse al %s", enumToProcess(handshakeProceso));
		exit_gracefully(ERROR_SOCKET);
	}
}

void conectarYRecibirHandshake(int puertoEscucha, char *ipPropia, int handshakeProceso){
	int * socketPropio;
	uint16_t handshake;
	if (escuchar(puertoEscucha, &socketPropio, logger)) {
		exit_gracefully(1);
	}
	if (acceptConnection(socketPropio, &socketCPU, DAM_HSK, &handshake, logger)) {
		log_error(logger, "No se acepta la conexion");
		exit_gracefully(1);
	}
//	printf("Se conecto el DAM");
//	int * socketPropio;
//	int socketCreado = cargarSocket(puertoEscucha,ipPropia, &socketPropio, logger);
//	t_socketEscucha = inicializarTSocket(socketCreado, logger);
//	recibirHandshake(t_socketEscucha->socket, DAM_HSK, handshakeProceso, logger);

	printf("Se conecto el CPU");
   // pthread_create(&threadDAM, &tattr, (void *) esperarInstruccionDAM, NULL);
}


char * enumToProcess(int proceso)
{
	char * nombreProceso = "";
	switch(proceso)
	{
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
			log_error(logger, "Enum proceso error.");
			break;
	}
	if (strcmp(nombreProceso,"") == 0)
		exit_gracefully(-1);
	return nombreProceso;
}



//Funcion para cerrar el programa
void exit_gracefully(int return_nr) {

	bool returnCerrarSockets = cerrarSockets();
	if(return_nr > 0 || returnCerrarSockets){
		log_error(logger, "Fin del proceso: DAM");
	}else{
		log_info(logger, "Fin del proceso: DAM");
	}

	log_destroy(logger);
	exit(return_nr);
}


bool cerrarSockets(){
	free(t_socketSafa);
	free(t_socketFm9);
	free(t_socketMdj);
	int ret1 = close(*socketSafa);
	int ret2 = close(*socketFm9);
	int ret3 = close(*socketMdj);

	return ret1 < 0 || ret2 < 0 || ret3 < 0;
}
