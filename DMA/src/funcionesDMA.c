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
	log_trace_mutex(logger, "Se agrega el socket %d a la lista de sockets",
			socket);
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
	log_trace_mutex(logger, "Se saca el socket %d de la lista de sockets",
			socket);
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

/*
 * -----------------------------LEER ESCRIPTORIO-------------------------------------
 */
int leerEscriptorio(t_package paquete, int socketEnUso) {

	//Datos recibidos del cpu
	char *buffer = paquete.data;
	int pid = copyIntFromBuffer(&buffer);
	char * path = copyStringFromBuffer(&buffer);

	free(buffer);
	printf("Ehhh, voy a buscar %s para %d", path, pid);

	//Se envia el path del archivo al filesystem
	int size;
	char *keyCompress = compressKey(path, &size);

	if (enviar(t_socketMdj->socket, DAM_MDJ_CARGAR_ESCRIPTORIO, keyCompress,
			size, logger->logger)) {
		log_error_mutex(logger,
				"No se pudo enviar la busqueda del escriptorio al MDJ");
		free(keyCompress);
		return EXIT_FAILURE;
	}

	//Se reciben los paquetes del mdj y se envian al fm9
	int result = enviarPkgDeMdjAFm9(pid, path);

	if (result == EXIT_SUCCESS) {
		//Se envia la confirmacion a SAFA del proceso y la posicion en memoria
		//itsLoaded = true
		enviarMsjASafaPidCargado(pid, 1);
	} else {
		//Comunicarle un error al safa
		//itsLoaded = false
		enviarMsjASafaPidCargado(pid, 0);
	}

	free(keyCompress);

	return EXIT_SUCCESS;
}

//Se recibe uno o varios paquetes del MDJ y se envian al FM9
//Return: Base en memoria de los paquetes guardados.
int enviarPkgDeMdjAFm9(int pid, char * path) {
	//Se recibe el archivo desde filesystem
	t_package package;
	int result;

	//Recibo un primer mensaje para saber cuanto pesa el archivo
	if (recibir(t_socketMdj->socket, &package, logger->logger)) {
		log_error_mutex(logger, "No se pudo recibir el mensaje del MDJ");
		return EXIT_FAILURE;
	} else {

		if (package.code == DAM_MDJ_FAIL) {
			return EXIT_FAILURE;
		}

		//Recibo el tamaño del archivo a cargar
		int sizeOfFile = atoi(package.data);

		//Calculo cuantos paquetes voy a recibir del fm9 segun mi transfer size
		int cantPkg = calcularCantidadPaquetes(sizeOfFile);

		//Ahora se reciben los paquetes y se concatena todo el archivo
		//(Si el archivo es mayor que mi Transfersize recibo n paquetes del tamaño de mi transfersize)

		char * bufferConcatenado = malloc(configDMA->transferSize * cantPkg);
		int sizeOfBuffer = 0;
		//Recibo x cantPkg del mdj y se concatenan en un bufferConcatenado
		for (int i = 0; i < cantPkg; i++) {
			t_package pkgTransferSize;
			pkgTransferSize.size = configDMA->transferSize; //TODO VER SI ESTO ESTA BIEN!!------------------------------------

			if (recibir(t_socketMdj->socket, &pkgTransferSize,
					logger->logger)) {
				log_error_mutex(logger, "Error al recibir el paquete %d", i);
			} else {
				//Concatenar y recalcular sizeOfBuffer
				copyStringToBuffer(&bufferConcatenado, pkgTransferSize.data);
				sizeOfBuffer = sizeOfBuffer
						+ (strlen(pkgTransferSize.data) * sizeof(char));
			}
		}

		//una vez recibido todo el archivo y de haberlo concatenado en un char *
		//realizo un split con cada /n y cuento la cantidad de lineas que contiene el mensaje
		int cantLineas;

		char** arrayLineas = str_split(bufferConcatenado, '\n', cantLineas);

		free(bufferConcatenado);

		//Se envia un msj al fm9 con los siguientes parametros
		char *buffer;
		copyIntToBuffer(&buffer, pid); //ProcesID
		copyIntToBuffer(&buffer, cantLineas); //Cantidad De lineas
		copyStringToBuffer(&buffer, path); //Path del archivo
		int size = sizeof(int) * 3 + (strlen(path) * sizeof(char));

		if (enviar(t_socketFm9->socket, DAM_FM9_CARGAR_ESCRIPTORIO, buffer,
				size, logger->logger)) {
			log_error_mutex(logger,
					"Error al enviar info del escriptorio a FM9");
			free(buffer);
			return EXIT_FAILURE;
		}
		free(buffer);

		//SE ENVIA LINEA POR LINEA AL FM9
		for (int i = 0; *(arrayLineas + i); i++) {
			//Linea -> buffer
			char * buffer = *(arrayLineas + i);
			//Tomo el tamaño total de la linea
			int tamanioLinea = strlen(buffer);

			//Si la linea es mayor a mi transfer size debo enviarlo en varios paquetes
			if (tamanioLinea > configDMA->transferSize) {
				//Calculo la cantidad de paquetes
				int cantPaquetes = calcularCantidadPaquetes(tamanioLinea);

				//por cada paquete...
				for (i = 0; i < cantPaquetes; i++) {
					char sub[configDMA->transferSize]; //substring a enviar
					int inicio = configDMA->transferSize * i, //posicion inicial del substring
					fin = configDMA->transferSize * (i + 1); //posicion final del substring

					//Si es el ultimo paquete a enviar el fin es el tamanio de linea
					if (i + 1 == cantPaquetes) {
						fin = tamanioLinea;
					}

					int count = 0;
					while (inicio < fin) {
						sub[count] = buffer[inicio];
						inicio++;
						count++;
					}
					//sub[c] = '\0'; <-----------------------TODO VER SI HACE FALTA ESTO

					char * bufferAEnviar;
					copyIntToBuffer(&bufferAEnviar, i + 1);
					copyIntToBuffer(&bufferAEnviar, count * sizeof(char));
					copyStringToBuffer(&bufferAEnviar, sub);
					int size = sizeof(int) * 2 + count * sizeof(char);

					if (enviar(t_socketFm9->socket, DAM_FM9_ENVIO_PKG,
							bufferAEnviar, size, logger->logger)) {
						log_error_mutex(logger,
								"Error al enviar info del escriptorio a FM9");
						free(bufferAEnviar);
						return EXIT_FAILURE;
					}
					//enviar
					free(bufferAEnviar);
					free(buffer);
				}
			} else {
				//Si está dentro del tamaño permitido se envía la linea
				char * bufferAEnviar;
				copyIntToBuffer(&bufferAEnviar, (i + 1));
				copyIntToBuffer(&bufferAEnviar, tamanioLinea * sizeof(char));
				copyStringToBuffer(&bufferAEnviar, buffer);
				int size = sizeof(int) * 2 + tamanioLinea * sizeof(char);

				if (enviar(t_socketFm9->socket, DAM_FM9_ENVIO_PKG,
						bufferAEnviar, size,
						logger->logger)) {
					log_error_mutex(logger,
							"Error al enviar info del escriptorio a FM9");
					free(buffer);
					return EXIT_FAILURE;
				}
			}
			free(*(arrayLineas + i));
			i++;
		}
		free(arrayLineas);
		log_info_mutex(logger,
				"Se enviaron todos los datos a memoria del proceso: %d", pid);

		//Se recibe confirmacion de datos guardados en memoria del FM9
		result = recibirConfirmacionMemoria();
	}
	return result;
}

int calcularCantidadPaquetes(int sizeOfFile) {
	//Lo divido por mi transfer Size y me quedo con la parte entera
	double num = sizeOfFile / configDMA->transferSize;

	double p_entera;
	double p_decimal;
	//Separo la parte entera
	p_decimal = modf(num, &p_entera);
	//Calculo la cantidad de paquetes
	int cantPart;
	if (p_decimal != 0) {
		cantPart = p_entera + 1;
	} else {
		cantPart = p_entera;
	}
	log_info_mutex(logger, "Se recibiran %d paquetes", cantPart);

	return cantPart;
}

int contarCantidadLineas(char * string) {
	int i = 0, cantidadLineas = 0;

	while (string[i] != '\0') {
		if (string[i] == '\n') {
			cantidadLineas++;
		}
	}
	return cantidadLineas;
}

int recibirConfirmacionMemoria() {

	t_package package;
	int result;

	if (recibir(t_socketFm9->socket, &package, logger->logger)) {
		log_error_mutex(logger,
				"Error al recibir los datos de memoria asociados al proceso");
		result = -1;
	} else {
		result = atoi(package.data);
	}

	if (result == FM9_DAM_ESCRIPTORIO_CARGADO) {
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}

void enviarMsjASafaPidCargado(int pid, int result) {

	//Está cargado(ItsLoaded)--> 1: Exito, 0: Fallo.
	//Se envía a S-afa un mensaje diciendo que el proceso ya esta cargado en memoria
	char *buffer;
	int size = sizeof(int) * 2;
	copyIntToBuffer(&buffer, pid);
	copyIntToBuffer(&buffer, result);

	if (enviar(t_socketSafa->socket, DAM_SAFA_CONFIRMACION_PID_CARGADO, buffer,
			size, logger->logger)) {
		log_error_mutex(logger, "Error al enviar msj de confirmacion al SAFA.");
		free(buffer);
	}
	log_info_mutex(logger, "Mensaje de confirmacion a S-afa enviado");
	free(buffer);
}

/*
 * -------------------------------ABRIR ARCHIVO ------------------------------------
 */
int abrirArchivo(t_package paquete, int socketEnUso) {

	//Datos recibidos del cpu
	char *buffer = paquete.data;
	int pid = copyIntFromBuffer(&buffer);
	char * path = copyStringFromBuffer(&buffer);

	free(buffer);
	printf("Ehhh, voy a buscar %s para %d", path, pid);

	//Se envia el path del archivo al filesystem
	int size;
	char *keyCompress = compressKey(path, &size);

	if (enviar(t_socketMdj->socket, DAM_MDJ_CARGAR_ESCRIPTORIO, keyCompress,
			size, logger->logger)) {
		log_error_mutex(logger,
				"No se pudo enviar la busqueda del escriptorio al MDJ");
		free(keyCompress);
		return EXIT_FAILURE;
	}

	//Se reciben los paquetes del mdj y se envian al fm9
	int result = enviarPkgDeMdjAFm9(pid, path);

	if (result == EXIT_SUCCESS) {
		//Se envia la confirmacion a SAFA del proceso y la posicion en memoria
		//itsLoaded = true
		enviarMsjASafaPidCargado(pid, 1);
	} else {
		//Comunicarle un error al safa
		//itsLoaded = false
		enviarMsjASafaPidCargado(pid, 0);
	}

	free(keyCompress);

	return EXIT_SUCCESS;
}

/*
 * --------------------------HACER FLUSH DE DATOS EN MEMORIA---------------------
 */
int hacerFlush(t_package paquete, int socketEnUso) {

	//Datos recibidos del cpu
	char *buffer = paquete.data;
	int pid = copyIntFromBuffer(&buffer);
	char * path = copyStringFromBuffer(&buffer);
	free(buffer);

	//Se envia el path del archivo al Fm9
	int size;
	char *bufferToFm9;
	copyIntToBuffer(&bufferToFm9, pid);
	copyStringToBuffer(&bufferToFm9, path);
	copyIntToBuffer(&bufferToFm9, configDMA->transferSize);

	size = sizeof(int) * 2 + strlen(path) * sizeof(char);

	//Se envia las posiciones a FM9 y retorna los datos para guardar en MDJ
	if (enviar(t_socketFm9->socket, DAM_FM9_FLUSH, bufferToFm9, size,
			logger->logger)) {
		log_error_mutex(logger, "Error al enviar solicitud de datos a FM9");
		free(bufferToFm9);
		return EXIT_FAILURE;
	}
	free(bufferToFm9);

	//Se reciben los paquetes del mdj y se envian al fm9
	int result = enviarPkgDeFm9AMdj(pid);

	if (result == EXIT_SUCCESS) {
		//Se guardó correctamente y se envía itFlushed= 0 y el pid
		//itsLoaded = false
		//itsFlushed = true
		enviarMsjASafaArchivoGuardado(pid, EXIT_SUCCESS);
	} else {
		//ocurrio un fallo y se envia itFlushed = 1 y el pid
		//itsLoaded = false
		//itsFlushed = false
		enviarMsjASafaArchivoGuardado(pid, EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}

int enviarPkgDeFm9AMdj(int pid) {
	//Se recibe el archivo desde FUNES MEMORY
	t_package pkgCantLineas;
	int cantLineasARecibir;
	//Recibo un primer mensaje para saber cuantas lineas voy a recibir
	if (recibir(t_socketMdj->socket, &pkgCantLineas, logger->logger)) {
		log_error_mutex(logger, "No se pudo recibir el mensaje del MDJ");
		return EXIT_FAILURE;
	} else {
		cantLineasARecibir = atoi(pkgCantLineas.data);
	}

	//realizo una iteracion por cada linea
	for (int linea = 0; linea < cantLineasARecibir; linea++) {
		t_package package;
		int cantidadPaquetes;
		int nroLinea;
		//int tamanioLinea;

		if (recibir(t_socketFm9->socket, &package, logger->logger)) {
			log_error_mutex(logger, "No se pudo recibir el mensaje del Fm9");
		} else {

			if (package.code == FM9_DAM_FLUSH) {
				//Recibo la cantidad de paquetes que voy a recibir para esta linea
				//tamanioLinea = copyIntFromBuffer(&package.data);
				nroLinea = copyIntFromBuffer(&package.data); //NRO DE LINEA
				cantidadPaquetes = copyIntFromBuffer(&package.data); //CANTIDAD DE PAQUETES EN LA LINEA
			} else {
				log_error_mutex(logger,
						"Error al recibir el tamanio de linea del fm9");
				return EXIT_FAILURE;
			}
			log_info_mutex(logger, "Se recibiran %d paquetes",
					cantidadPaquetes);
		}

		//ENVIAR DATOS A MDJ: le envio la linea y la cantidad de paquetes de esta linea
		char * pkgToMdj;
		copyIntToBuffer(&pkgToMdj, nroLinea);
		copyIntToBuffer(&pkgToMdj, cantidadPaquetes);
		int sizeOfBuffer = sizeof(int) * 2; //calculo el size
		if (enviar(t_socketMdj->socket, DAM_MDJ_HACER_FLUSH, pkgToMdj,
				sizeOfBuffer, logger->logger)) {
			log_error_mutex(logger,
					"Error al enviar cantidad de paquetes a MDJ");
			return EXIT_FAILURE;
		}
		free(pkgToMdj);

		//Ahora se reciben los paquetes y se envia a FILESYSTEM
		for (int i = 0; i < cantidadPaquetes; i++) {
			t_package pkgTransferSize;

			if (recibir(t_socketFm9->socket, &pkgTransferSize,
					logger->logger)) {
				log_error_mutex(logger, "Error al recibir el paquete %d", i);
				return EXIT_FAILURE;
			} else {
				//Envío el paquete al MDJ
				char * buffer;
				copyStringToBuffer(&buffer, pkgTransferSize.data);
				int size = strlen(buffer) * sizeof(char);
				if (enviar(t_socketMdj->socket, DAM_MDJ_GUARDAR_DATOS, buffer,
						size, logger->logger)) {
					log_error_mutex(logger,
							"Error al enviar el archivo a guardar al MDJ");
					free(buffer);
					return EXIT_FAILURE;
				}
				free(buffer);
			}
		}
		log_info_mutex(logger,
				"Se enviaron todos los datos  del proceso %d,al FileSystem.",
				pid);
	}

	return EXIT_SUCCESS;

}

void enviarMsjASafaArchivoGuardado(int pid, int itsFlushed) {

	//Está cargado(ItsLoaded)--> 0: Exito, 1: Fallo.
	//Se envía a S-afa un mensaje diciendo que el proceso ya esta cargado en memoria
	char *buffer;

	copyIntToBuffer(&buffer, pid);
	copyIntToBuffer(&buffer, itsFlushed);

	int size = sizeof(int) + sizeof(int);

	if (enviar(t_socketSafa->socket, DAM_SAFA_CONFIRMACION_DATOS_GUARDADOS,
			buffer, size, logger->logger)) {
		log_error_mutex(logger, "Error al enviar msj de confirmacion al SAFA.");
		free(buffer);
	}
	log_info_mutex(logger, "Mensaje de confirmacion a S-afa enviado");
	free(buffer);
}

//-------------------------CREAR ARCHIVO ---------------------------------------------

/*
 * Funcion para solicitar al MDJ la creacion de un archivo
 * params: pid,path,cantidad de lineas
 * return: 1 -> Fail
 * 		   0 -> Success
 */
int crearArchivo(t_package paquete, int socketEnUso) {

	//Datos recibidos del cpu
	char *buffer = paquete.data;

	int pid = copyIntFromBuffer(&buffer);
	char * path = copyStringFromBuffer(&buffer);

	int size = sizeof(int) + (strlen(buffer) * sizeof(char));

	//SE ENVIA EL PAQEUTE CON LOS DATOS A MDJ PARA CREAR ARCHIVO
	if (enviar(t_socketMdj->socket, DAM_MDJ_CREAR_ARCHIVO, buffer, size,
			logger->logger)) {
		log_error_mutex(logger, "Error al crear el archivo: %s", path);
		free(buffer);
		enviarMsjASafaArchivoCreado(pid, 0, path);
		return EXIT_FAILURE;
	}

	//SE ENVIA CONFIRMACION A SAFA
	enviarMsjASafaArchivoCreado(pid, 1, path);
	free(buffer);
	return EXIT_SUCCESS;
}

void enviarMsjASafaArchivoCreado(int pid, int itsCreated, char * path) {

	//Está creado(itsCreated)--> 1: Exito, 0: Fallo.
	//Se envía a S-afa un mensaje diciendo que el proceso ya esta cargado en memoria
	char *buffer;

	copyIntToBuffer(&buffer, pid);
	copyIntToBuffer(&buffer, itsCreated);
	copyStringToBuffer(&buffer, path);

	int size = sizeof(int) * 2 + (strlen(path) * sizeof(char));

	if (enviar(t_socketSafa->socket, DAM_SAFA_CONFIRMACION_CREAR_ARCHIVO,
			buffer, size, logger->logger)) {
		log_error_mutex(logger, "Error al enviar msj de confirmacion al SAFA.");
		free(buffer);
	}
	log_info_mutex(logger, "Mensaje de confirmacion a S-afa enviado");
	free(buffer);
}

//--------------------------BORRAR ARCHIVO ------------------------------------------

int borrarArchivo(t_package paquete, int socketEnUso) {

	//Datos recibidos del cpu
	char *buffer = paquete.data;
	int pid = copyIntFromBuffer(&buffer);
	char * path = copyStringFromBuffer(&buffer);

	int size = sizeof(int) + (strlen(path) * sizeof(char));
	//SE ENVIA EL PATH DEL ARCHIVO AL MDJ PARA BORRAR EL MISMO
	if (enviar(t_socketMdj->socket, DAM_MDJ_BORRAR_ARCHIVO, buffer, size,
			logger->logger)) {
		log_error_mutex(logger,
				"Error al enviar el path al MDJ del archivo a borrar");
		free(buffer);
		enviarMsjASafaArchivoBorrado(pid, 0, path);
		return EXIT_FAILURE;
	}

	free(buffer);
	//SE ENVIA LA CONFIRMACION A SAFA
	enviarMsjASafaArchivoBorrado(pid, 1, path);
	return EXIT_SUCCESS;
}

void enviarMsjASafaArchivoBorrado(int pid, int itsDeleted, char * path) {

	//Está creado(itsCreated)--> 1: Exito, 0: Fallo.
	//Se envía a S-afa un mensaje diciendo que el proceso ya esta cargado en memoria
	char *buffer;

	copyIntToBuffer(&buffer, pid);
	copyIntToBuffer(&buffer, itsDeleted);
	copyStringToBuffer(&buffer, path);

	int size = sizeof(int) * 2 + (strlen(path) * sizeof(char));

	if (enviar(t_socketSafa->socket, DAM_SAFA_CONFIRMACION_BORRAR_ARCHIVO,
			buffer, size, logger->logger)) {
		log_error_mutex(logger, "Error al enviar msj de confirmacion al SAFA.");
		free(buffer);
	}
	log_info_mutex(logger, "Mensaje de confirmacion a S-afa enviado");
	free(buffer);
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
	} else {
		log_info_mutex(logger, "Se realizó la conexion del S-Afa");
	}

	res = conectarseConMdj();
	if (res > 0) {
		log_error_mutex(logger, "Error al crear el hilo del MDJ");
		exit_gracefully(1);
	} else {
		log_info_mutex(logger, "Se realizó la conexion del MDJ");
	}

	res = conectarseConFm9();
	if (res > 0) {
		log_error_mutex(logger, "Error al crear el hilo del FM9");
		exit_gracefully(1);
	} else {
		log_info_mutex(logger, "Se realizó la conexion del FM9");
	}

	res = conectarseConCPU();
	if (res > 0) {
		log_error_mutex(logger, "Error al crear el hilo del CPU");
		exit_gracefully(1);
	} else {
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

	//Se envía el transfer size
	int size;
	char * buffer;
	copyIntToBuffer(&buffer, configDMA->transferSize);
	size = sizeof(int);

	if (enviar(t_socketMdj->socket, DAM_MDJ_TRANSFER_SIZE, buffer, size,
			logger->logger)) {
		log_error_mutex(logger, "No se pudo enviar el transfer size al MDJ");
		free(buffer);
		return EXIT_FAILURE;
	}

	free(buffer);
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

void conectarYenviarHandshake(int puerto, char *ip, int * socket,
		int handshakeProceso, t_socket* TSocket) {
	if (!cargarSocket(puerto, ip, socket, logger->logger)) {
		TSocket = inicializarTSocket(*socket, logger->logger);
		enviarHandshake(TSocket->socket, DAM_HSK, handshakeProceso,
				logger->logger);
	} else {
		log_error_mutex(logger, "Error al conectarse al %s",
				enumToProcess(handshakeProceso));
		exit_gracefully(ERROR_SOCKET);
	}
}

void conectarYRecibirHandshake(int puertoEscucha) {

	int socketEscucha;
	if (escuchar(puertoEscucha, &socketEscucha, logger->logger)) {
		//liberar recursos/
		exit_gracefully(1);
	}
	log_trace_mutex(logger, "El socket de escucha del DMA es: %d",
			socketEscucha);
	log_info_mutex(logger, "El socket de escucha de DMA es: %d", socketEscucha);
	addNewSocketToMaster(socketEscucha);

	printf("Se conecto el CPU");
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

