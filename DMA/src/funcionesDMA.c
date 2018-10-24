/*
 * funcionesDMA.c
 *
 *  Created on: 11 sep. 2018
 *      Author: utnso
 */

#include "funcionesDMA.h"
#include "DMA.h"
#include <grantp/structCommons.h>
#include <grantp/configuracion.h>

//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA EL MANEJO DEL SELECT
//------------------------------------------------------------------------------------------------------------------

void addNewSocketToMaster(int socket) {
    pthread_mutex_lock(&mutexMaster);
    FD_SET(socket, &master);
    log_trace_mutex(logger, "Se agrega el socket %d a la lista de sockets", socket);
    updateMaxfd(socket);
    pthread_mutex_unlock(&mutexMaster);
}

int getMaxfd() {
    int m;
    pthread_mutex_lock(&mutexMaxfd);
    m = maxfd;
    pthread_mutex_unlock(&mutexMaxfd);
    return m;
}

void updateMaxfd(int socket) {
    if (socket > getMaxfd()) {
        pthread_mutex_lock(&mutexMaxfd);
        maxfd = socket;
        pthread_mutex_unlock(&mutexMaxfd);
    }
}

void deleteSocketFromMaster(int socket) {
    pthread_mutex_lock(&mutexMaster);
    FD_CLR(socket, &master);
    log_trace_mutex(logger, "Se saca el socket %d de la lista de sockets", socket);
    pthread_mutex_unlock(&mutexMaster);
}

void updateReadset() {
    pthread_mutex_lock(&mutexMaster);
    pthread_mutex_lock(&mutexReadset);
    readset = master;
    pthread_mutex_unlock(&mutexReadset);
    pthread_mutex_unlock(&mutexMaster);
}

int isSetted(int socket) {
    int s;
    pthread_mutex_lock(&mutexReadset);
    s = FD_ISSET(socket, &readset);
    pthread_mutex_unlock(&mutexReadset);
    return s;
}



//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA Manejo de solicitudes del DMA
//------------------------------------------------------------------------------------------------------------------

//-----------------------------LEER ESCRIPTORIO-------------------------------------
int leerEscriptorio(t_package paquete, int socketEnUso){

	//Datos recibidos del cpu
	char *buffer = paquete.data;
	int pid = copyIntFromBuffer(&buffer);
	char * path = copyStringFromBuffer(&buffer);

	free(buffer);
	printf("Ehhh, voy a buscar %s para %d", path, pid);

	//Se envia el path del archivo al filesystem
	int size;
	char *keyCompress = compressKey(path, &size);

	if(enviar(t_socketMdj->socket,DAM_MDJ_CARGAR_ESCRIPTORIO,keyCompress, size, logger->logger))
	{
		log_error_mutex(logger, "No se pudo enviar la busqueda del escriptorio al MDJ");
		free(keyCompress);
		return EXIT_FAILURE;
	}

	//Se reciben los paquetes del mdj y se envian al fm9
	int baseMemoriaDeEscriptorio = enviarPkgDeMdjAFm9(pid);

	if(baseMemoriaDeEscriptorio >= 0){
		//Se envia la confirmacion a SAFA del proceso y la posicion en memoria
		//itsLoaded = true
		//itsFlushed = false
		enviarMsjASafaPidCargado(pid,1,baseMemoriaDeEscriptorio);
	}else{
		//Comunicarle un error al safa
		//itsLoaded = false
		//itsFlushed = false
		enviarMsjASafaPidCargado(pid,0,baseMemoriaDeEscriptorio);
	}

	free(keyCompress);

	return EXIT_SUCCESS;
}


//Se recibe uno o varios paquetes del MDJ y se envian al FM9
//Return: Base en memoria de los paquetes guardados.
int enviarPkgDeMdjAFm9(int pid){
	//Se recibe el archivo desde filesystem
	t_package package;
	int baseMemoriaDeEscriptorio = -1;

	//Recibo un primer mensaje para saber cuanto pesa el archivo
	if(recibir(t_socketMdj->socket, &package, logger->logger)){
		log_error_mutex(logger, "No se pudo recibir el mensaje del MDJ");
	}else{
		//Recibo el tamaño del archivo a cargar
		int sizeOfFile = (int) package.data;

		//Lo divido por mi transfer Size y me quedo con la parte entera + 1
		double num = sizeOfFile / configDMA->transferSize;

		double p_entera;
		double p_decimal;
		//Separo la parte entera
		p_decimal = modf(num, &p_entera);
		//Calculo la cantidad de paquetes
		int cantPart = p_entera + 1;
		log_info_mutex(logger, "Se recibiran %d paquetes", cantPart);

		//Se envia cantidad de paquetes a enviar a memoria
		char *buffer;
		int size = sizeof(int);
		copyIntToBuffer(&buffer, cantPart);
		if(enviar(t_socketFm9->socket,DAM_FM9_AVISO_CANT_ENVIOS,buffer,size,logger->logger)){
			log_error_mutex(logger, "Error al enviar info del escriptorio a FM9");
			//TODO ver que hacer ante el error
		}

		//TODO RECIBIR RESPUESTA DEL FM9 SI PUEDE GUARDAR EL ARCHIVO EN MEMORIA
	//	if(puedeGuardarArchivo){
	//	se Solicita archivo y se manda --> se empezó a desarrollar abajo
	//	}else{
	// 	se envia mensaje a safa diciendo que no se pudo cargar archivo en memoria
	//	}

		//Ahora se reciben los paquetes y se envia a memoria
		//TODO VER COMO SE RECIBEN LOS PAQUETES Y COMO SE ENVIAN
		for(int i = 0; i<cantPart;i++){
			//TODO VER SI SE RECIBEN PAQUETES DE 16 Y SE MANDA UNO POR UNO
			//O SI RECIBO UN PAQUETE CON TODO EL ARCHIVO EN EL  <------------------------ACA SE PENSO COMO QUE RECIBO VARIOS PAQUETES DE 16 BYTES
			t_package pkgTransferSize;
			//bufferTransferSize.size = 16; TODO: ver si setea un maximo

			if(recibir(t_socketMdj->socket, &pkgTransferSize, logger->logger)){
				log_error_mutex(logger, "Error al recibir el paquete %d",i);
			}else{
				//Enviar paquete a memoria
				char * buffer;
				copyStringToBuffer(&buffer, pkgTransferSize.data);
				int sizeOfBuffer = strlen(buffer);
				if(enviar(t_socketMdj->socket,DAM_FM9_GUARDARLINEA,buffer, sizeOfBuffer,logger->logger)){
					log_error_mutex(logger, "Error al enviar el paquete %d", i);
				}

			}
		}
		log_info_mutex(logger,"Se enviaron todos los datos a memoria del proceso: %d",pid);

		//Se recibe los datos de la posicion en memoria
		baseMemoriaDeEscriptorio = recibirDatosMemoria();
	}
	return baseMemoriaDeEscriptorio;
}

int recibirDatosMemoria(){

	t_package package;
	int datosBaseMemoria;

	if(recibir(t_socketFm9->socket,&package,logger->logger)){
		log_error_mutex(logger, "Error al recibir los datos de memoria asociados al proceso");
		datosBaseMemoria = -1;
	}else{
		datosBaseMemoria = (int) package.data;
	}

	return datosBaseMemoria;
}

void enviarMsjASafaPidCargado(int pid,int itsLoaded, int base){

	//Está cargado(ItsLoaded)--> 1: Exito, 0: Fallo.
	//Se envía a S-afa un mensaje diciendo que el proceso ya esta cargado en memoria
	char *buffer;
	int size = sizeof(pid) + sizeof(itsLoaded) + sizeof(base) ;
	copyIntToBuffer(&buffer, pid);
	copyIntToBuffer(&buffer, itsLoaded);
	copyIntToBuffer(&buffer, base);

	if(enviar(t_socketSafa->socket,DAM_SAFA_CONFIRMACION_PID_CARGADO,buffer, size, logger->logger))
	{
		log_error_mutex(logger, "Error al enviar msj de confirmacion al SAFA.");
		free(buffer);
	}
	log_info_mutex(logger, "Mensaje de confirmacion a S-afa enviado");
	free(buffer);
}

void enviarMsjASafaArchivoGuardado(int pid,int itsFlushed, char * path){

	//Está cargado(ItsLoaded)--> 1: Exito, 0: Fallo.
	//Se envía a S-afa un mensaje diciendo que el proceso ya esta cargado en memoria
	char *buffer;

	copyIntToBuffer(&buffer, pid);
	copyIntToBuffer(&buffer, itsFlushed);
	copyStringToBuffer(&buffer, path);

	int size = sizeof(buffer);

	if(enviar(t_socketSafa->socket,DAM_SAFA_CONFIRMACION_DATOS_GUARDADOS,buffer, size, logger->logger))
	{
		log_error_mutex(logger, "Error al enviar msj de confirmacion al SAFA.");
		free(buffer);
	}
	log_info_mutex(logger, "Mensaje de confirmacion a S-afa enviado");
	free(buffer);
}

//--------------------ABRIR ARCHIVO ------------------------------------------------
int abrirArchivo(t_package paquete, int socketEnUso){

	//Datos recibidos del cpu
	char *buffer = paquete.data;
	int pid = copyIntFromBuffer(&buffer);
	char * path = copyStringFromBuffer(&buffer);

	free(buffer);
	printf("Ehhh, voy a buscar %s para %d", path, pid);

	//Se envia el path del archivo al filesystem
	int size;
	char *keyCompress = compressKey(path, &size);

	if(enviar(t_socketMdj->socket,DAM_MDJ_CARGAR_ESCRIPTORIO,keyCompress, size, logger->logger))
	{
		log_error_mutex(logger, "No se pudo enviar la busqueda del escriptorio al MDJ");
		free(keyCompress);
		return EXIT_FAILURE;
	}

	//Se reciben los paquetes del mdj y se envian al fm9
	int baseMemoriaDeEscriptorio = enviarPkgDeMdjAFm9(pid);

	if(baseMemoriaDeEscriptorio >= 0){
		//Se envia la confirmacion a SAFA del proceso y la posicion en memoria
		//itsLoaded = true
		//itsFlushed = false
		enviarMsjASafaPidCargado(pid,1,baseMemoriaDeEscriptorio);
	}else{
		//Comunicarle un error al safa
		//itsLoaded = false
		//itsFlushed = false
		enviarMsjASafaPidCargado(pid,0,baseMemoriaDeEscriptorio);
	}

	free(keyCompress);


	return EXIT_SUCCESS;
}

//--------------------------HACER FLUSH DE DATOS EN MEMORIA---------------------
int hacerFlush(t_package paquete, int socketEnUso){

	//Datos recibidos del cpu
	char *buffer = paquete.data;
	int pid = copyIntFromBuffer(&buffer);
	char * path = copyStringFromBuffer(&buffer);
	int baseEnMemoria = copyIntFromBuffer(&buffer);
	int limiteEnMemoria = copyIntFromBuffer(&buffer);
	free(buffer);

	//Se envia el path del archivo al filesystem
	int size;
	char *bufferToMdj;
	copyIntToBuffer(&bufferToMdj,baseEnMemoria);
	copyIntToBuffer(&bufferToMdj,limiteEnMemoria);
	size = sizeof(bufferToMdj);

	//Se envia las posiciones a FM9 y retorna los datos para guardar en MDJ
	if(enviar(t_socketFm9->socket,DAM_FM9_RETORNARlINEA,bufferToMdj, size, logger->logger)){
		log_error_mutex(logger, "Error al enviar solicitud de datos a FM9");
		free(bufferToMdj);
		return EXIT_FAILURE;
	}
	free(bufferToMdj);

	//Se reciben los paquetes del mdj y se envian al fm9
	int result = enviarPkgDeFm9AMdj(pid);

	if(result > 0){
		//ocurrio un fallo y se envia itFlushed = 0 y el pid
		//itsLoaded = false
		//itsFlushed = false
		enviarMsjASafaArchivoGuardado(pid,0,path);
	}else{
		//Se guardó correctamente y se envía itFlushed= 1 y el pid
		//itsLoaded = false
		//itsFlushed = true
		enviarMsjASafaArchivoGuardado(pid,1,path);
	}

	return EXIT_SUCCESS;
}

int enviarPkgDeFm9AMdj(int pid){
	//Se recibe el archivo desde FUNES MEMORY
	t_package package;
	int cantLineas = -1;

	//Recibo un primer mensaje para saber cuanto pesa el archivo
	if(recibir(t_socketMdj->socket, &package, logger->logger)){
		log_error_mutex(logger, "No se pudo recibir el mensaje del MDJ");
	}else{
		//Recibo el tamaño del archivo a cargar
		cantLineas = (int) package.data;

		log_info_mutex(logger, "Se recibiran %d paquetes", cantLineas);

		char * pkgToMdj; //Se crea el buffer para luego mandarlo al filesystem
		copyIntToBuffer(&pkgToMdj, pid);

		//Ahora se reciben los paquetes y se envia a FILESYSTEM
		//TODO VER COMO SE RECIBEN LOS PAQUETES Y COMO SE ENVIAN
		for(int i = 0; i<cantLineas;i++){
			//TODO VER SI SE RECIBEN PAQUETES DE 16 Y SE MANDA UNO POR UNO
			//O SI RECIBO UN PAQUETE CON TODO EL ARCHIVO EN EL  <------------------------ACA SE PENSO COMO QUE RECIBO VARIOS PAQUETES DE 16 BYTES
			t_package pkgTransferSize;
			//bufferTransferSize.size = 16; TODO: ver si setea un maximo

			if(recibir(t_socketFm9->socket, &pkgTransferSize, logger->logger)){
				log_error_mutex(logger, "Error al recibir el paquete %d",i);
				return EXIT_FAILURE;
			}else{
				//Cargo el buffer con toda la data
				copyStringToBuffer(&pkgToMdj,pkgTransferSize.data);
			}
		}
		//ENVIAR DATOS A MDJ
		int sizeOfBuffer = strlen(pkgToMdj); //calculo el size
		if(enviar(t_socketMdj->socket,DAM_MDJ_GUARDAR_DATOS,pkgToMdj, sizeOfBuffer,logger->logger)){
			log_error_mutex(logger, "Error al enviar el archivo a guardar al MDJ");
			return EXIT_FAILURE;
		}
		free(pkgToMdj);

		log_info_mutex(logger,"Se enviaron todos los datos  del proceso %d,al FileSystem.",pid);

	}
	return EXIT_SUCCESS;

}




//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA CONEXIONES INICIALES Y HANDSHAKE
//------------------------------------------------------------------------------------------------------------------



void iniciarConexionesDelDMA() {
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

	log_info_mutex(logger, "Se realizaron todas las conexiones correctamente");

}

/*FUNCION DEL HILO MDJ
 * Crea UN hilo que queda conectado y a la escucha del S-Afa
 */
int conectarseConSafa() {

	socketSafa = malloc(sizeof(int));
	conectarYenviarHandshake(configDMA->puertoSAFA, configDMA->ipSAFA,
			socketSafa, SAFA_HSK, t_socketSafa);
	return 0;
}

/*FUNCION DEL HILO MDJ
 * Crea UN hilo que queda conectado al MDJ
 */
int conectarseConMdj() {

	socketMdj = malloc(sizeof(int));
	conectarYenviarHandshake(configDMA->puertoMDJ, configDMA->ipMDJ, socketMdj,
			MDJ_HSK, t_socketMdj);
	return 0;
}

/*FUNCION DEL HILO FM9
 * Crea UN hilo que queda conectado al FM9
 */
int conectarseConFm9() {

	socketFm9 = malloc(sizeof(int));
	conectarYenviarHandshake(configDMA->puertoFM9, configDMA->ipFM9, socketFm9,
			FM9_HSK, t_socketFm9);
	return 0;
}

int conectarseConCPU() {
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

//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES PARA -------------------------
//------------------------------------------------------------------------------------------------------------------


