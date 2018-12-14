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
//		FUNCIONES PARA Manejo de solicitudes del DMA
//------------------------------------------------------------------------------------------------------------------

/*
 * -----------------------------LEER ESCRIPTORIO-------------------------------------
 */
bool leerEscriptorio(t_package paquete, int socketEnUso) {

	//Datos recibidos del cpu
	char *buffer = paquete.data;
	int pid = copyIntFromBuffer(&buffer);
	char * path = copyStringFromBuffer(&buffer);

//	free(buffer);
	log_info_mutex(logger,"Ehhh, voy a buscar %s para %d", path, pid);

	//Se envia el path del archivo al filesystem
	int size = sizeof(int) + ((strlen(path) + 1) * sizeof(char));
	char * bufferEnvio = (char *) malloc(size);
	char * p = bufferEnvio;
	copyStringToBuffer(&p, path);

	//CONFIRMO EXISTENCIA DEL ARCHIVO
	//mando un msj para saber si el archivo existe
	if (enviar(t_socketMdj->socket, DAM_MDJ_CONFIRMAR_EXISTENCIA_ARCHIVO, bufferEnvio, size, logger->logger)) {
		log_error_mutex(logger,
				"No se pudo enviar la busqueda del escriptorio al MDJ");
		free(bufferEnvio);
		return false;
	}
	//recibo la confirmacion
	int sizeOfFile = confirmarExistenciaFile();
	int result;

	//Declaro aca el cantIO pq si no, no funciona el enviar
	cantIODelProceso = 0;
	//Declaro aca la cantidad de lineas que va a tener el archivo
	cantLineasDelArchivo = 0;
	if(sizeOfFile >0){
		//Se reciben los paquetes del mdj y se envian al fm9
		result = enviarPkgDeMdjAFm9(pid, path, sizeOfFile );

	}else if(sizeOfFile == 0){
		//El archivo existe pero no tiene nada -> Se envia error al safa
		result = EXIT_FAILURE;
		enviarConfirmSafaScriptInit(pid, result, 0, 0);
		free(bufferEnvio);
		return false;
	}else{
		//no existe el archivo -> Se envia error al safa
		result = EXIT_FAILURE;
		enviarConfirmSafaScriptInit(pid, result, 0, 0);
		free(bufferEnvio);
		return false;
	}

	enviarConfirmSafaScriptInit(pid, result, cantIODelProceso, cantLineasDelArchivo);

	free(bufferEnvio);
	free(path);
	return true;
}

//Se recibe uno o varios paquetes del MDJ y se envian al FM9
//Return: Base en memoria de los paquetes guardados.
int enviarPkgDeMdjAFm9(int pid, char * path, int size) {

	//TODO: Hacer validacion, si es size = 0, no corresponde enviar ninguna paquete.

	//Calculo cuantos paquetes voy a recibir del fm9 segun mi transfer size
	int cantPkg = calcularCantidadPaquetes(size);

	//Ahora se reciben los paquetes y se concatena tudo el archivo
	//(Si el archivo es mayor que mi Transfersize recibo n paquetes del tamaño de mi transfersize)

	int sizeOfPkg = ((strlen(path)+1) *sizeof(char)) + sizeof(int)*3;
	char * pkg = (char *)malloc(sizeOfPkg);
	char * p = pkg;
	int inicio = 0;
	copyStringToBuffer(&p,path);//path
	copyIntToBuffer(&p,inicio);//inicio
	copyIntToBuffer(&p,size);//size

	if(enviar(t_socketMdj->socket,DAM_MDJ_CARGAR_ESCRIPTORIO, pkg, sizeOfPkg, logger->logger)){
		//error
		log_error_mutex(logger, "No se pudo enviar la busqueda del escriptorio al MDJ");
		free(pkg);
		return false;
	}
	free(pkg);

	char * bufferConcatenado = string_new();
	//Recibo x cantPkg del mdj y se concatenan en un bufferConcatenado
	for (int i = 0; i < cantPkg; i++) {
		t_package pkgTransferSize;

		if (recibir(t_socketMdj->socket, &pkgTransferSize, logger->logger)) {
			log_error_mutex(logger, "Error al recibir el paquete %d", i);
		} else {
			//Concatenar buffer
			char * buffer = pkgTransferSize.data;
			char * stringRecibido = copyStringFromBuffer(&buffer);
			string_append(&bufferConcatenado,stringRecibido);
			free(stringRecibido);
		}
	}

	//una vez recibido tudo el archivo y de haberlo concatenado en un char *
	//realizo un split con cada /n y cuento la cantidad de lineas que contiene el mensaje
	int cantLineas = 0;
	char ** arrayLineas = string_split(bufferConcatenado,"\n");
	int j = 0;
	while(arrayLineas[j])
	{
		//ALGORITMO PROPIO
		char * line = arrayLineas[j];
		if (strcmp(line," ") != 0)
		{
			t_cpu_operacion operacionDeLaLinea = parse(line, false);
			bool flag = esOperacionDeIO(operacionDeLaLinea);
			if(flag){
				cantIODelProceso ++;
			}
		}

		cantLineas++;
		j++;
	}

	cantLineasDelArchivo = cantLineas;
	free(bufferConcatenado);

	//Se envia un msj al fm9 con los siguientes parametros
	int sizeOfBuffer= sizeof(int) * 3 + ((strlen(path) + 1) * sizeof(char));
	char *buffer = (char *)malloc(sizeOfBuffer);
	char * ptr2 = buffer;
	copyIntToBuffer(&ptr2, pid); //ProcesID
	copyStringToBuffer(&ptr2, path); //Path del archivo
	copyIntToBuffer(&ptr2, cantLineas); //Cantidad De lineas

	if (enviar(t_socketFm9->socket, DAM_FM9_CARGAR_ESCRIPTORIO, buffer, sizeOfBuffer, logger->logger)) {
		log_error_mutex(logger, "Error al enviar info del escriptorio a FM9");
		free(buffer);
		return EXIT_FAILURE;
	}
	free(buffer);

	//SE ENVIA LINEA POR LINEA AL FM9
	// Agrego el i < cantLineas en vez de arrayLineas[i] porque de esta manera, evito enviar la linea vacia del final.
	for (int k = 0; k < cantLineas; k++) {
		//Linea -> buffer
		char * buffer = arrayLineas[k];
		//Tomo el tamaño total de la linea
		int tamanioLinea = string_length(buffer);

		//Si la linea es mayor a mi transfer size debo enviarlo en varios paquetes
		if (tamanioLinea > configDMA->transferSize) {
			//Calculo la cantidad de paquetes
			int cantPaquetes = calcularCantidadPaquetes(tamanioLinea);

			//por cada paquete...
			for (int l = 0; l < cantPaquetes; l++) {
				char * sub; //substring a enviar
				int inicio = configDMA->transferSize * k, //posicion inicial del substring
				fin = (configDMA->transferSize * (k + 1))-1; //posicion final del substring

				//Si es el ultimo paquete a enviar el fin es el tamanio de linea
				if (k + 1 == cantPaquetes) {
					fin = tamanioLinea;
				}

				sub = string_substring(buffer,inicio,fin);

				int size = sizeof(int) * 3 + (strlen(sub)+1) * sizeof(char);
				char * bufferAEnviar = (char *) malloc(size);
				char * p = bufferAEnviar;
				copyIntToBuffer(&p, k + 1); //NRO LINEA
				copyIntToBuffer(&p, strlen(sub) * sizeof(char)); //SIZE
				copyStringToBuffer(&p, sub); //BUFFER

				//enviar
				if (enviar(t_socketFm9->socket, DAM_FM9_ENVIO_PKG, bufferAEnviar, size, logger->logger)) {
					log_error_mutex(logger, "Error al enviar info del escriptorio a FM9");
					free(bufferAEnviar);
					return EXIT_FAILURE;
				}

				free(bufferAEnviar);
			}
		} else {
			//Si está dentro del tamaño permitido se envía la linea
			int size = (sizeof(int) * 3) + (tamanioLinea+1) * sizeof(char);
			char * bufferAEnviar = (char *) malloc(size);
			char * p = bufferAEnviar;
			copyIntToBuffer(&p, (k + 1));
			copyIntToBuffer(&p, tamanioLinea * sizeof(char));
			copyStringToBuffer(&p, buffer);

			if (enviar(t_socketFm9->socket, DAM_FM9_ENVIO_PKG,
					bufferAEnviar, size,
					logger->logger)) {
				log_error_mutex(logger, "Error al enviar info del escriptorio a FM9");
				free(buffer);
				return EXIT_FAILURE;
			}
			free(bufferAEnviar);
		}
		//free(arrayLineas[k]);
	}
	free(arrayLineas);
	log_info_mutex(logger, "Se enviaron todos los datos a memoria del proceso: %d", pid);

	//Se recibe confirmacion de datos guardados en memoria del FM9
	int result = recibirConfirmacionMemoria();

	return result;
}

/*
 * -------------------------------ABRIR ARCHIVO ------------------------------------
 */
bool abrirArchivo(t_package paquete, int socketEnUso) {

	//Datos recibidos del cpu
	char *buffer = paquete.data;
	int pid = copyIntFromBuffer(&buffer);
	char * path = copyStringFromBuffer(&buffer);

//	free(buffer);
	printf("Ehhh, voy a buscar %s para %d", path, pid);

	//Se envia el path del archivo al filesystem
	int size = ((strlen(path) + 1)*sizeof(char)) + sizeof(int);
	char * bufferEnvio = malloc(size);
	char * p = bufferEnvio;
	copyStringToBuffer(&p, path);

	//CONFIRMO EXISTENCIA DEL ARCHIVO
	//mando un msj para saber si el archivo existe
	if (enviar(t_socketMdj->socket, DAM_MDJ_CONFIRMAR_EXISTENCIA_ARCHIVO, bufferEnvio, size, logger->logger)) {
		log_error_mutex(logger, "No se pudo enviar la busqueda del escriptorio al MDJ");
		free(bufferEnvio);
		return false;
	}
	//recibo la confirmacion
	int sizeOfFile = confirmarExistenciaFile();
	int result;

	if(sizeOfFile >=0){
		//Se reciben los paquetes del mdj y se envian al fm9
		result = enviarPkgDeMdjAFm9(pid, path, sizeOfFile);
	}else{
		//no existe el archivo -> Se envia error al safa
		result = EXIT_FAILURE;
		enviarConfirmSafaAbrirArchivo(pid, result, path);
		free(bufferEnvio);
		return EXIT_FAILURE;
	}

	enviarConfirmSafaAbrirArchivo(pid, result, path);
	free(bufferEnvio);

	return EXIT_SUCCESS;
}

/*
 * --------------------------HACER FLUSH DE DATOS EN MEMORIA---------------------
 */
bool hacerFlush(t_package paquete, int socketEnUso) {

	//Datos recibidos del cpu
	char *buffer = paquete.data;
	int pid = copyIntFromBuffer(&buffer);
	char * path = copyStringFromBuffer(&buffer);

	//Se envia el path del archivo al Fm9
	int size = sizeof(int) * 3 + (strlen(path) + 1) * sizeof(char);
	char *bufferToFm9 = (char*)malloc(size);
	char * p = bufferToFm9;
	copyIntToBuffer(&p, pid);
	copyStringToBuffer(&p, path);
	copyIntToBuffer(&p, configDMA->transferSize);

	//Se envia las posiciones a FM9 y retorna los datos para guardar en MDJ
	if (enviar(t_socketFm9->socket, DAM_FM9_FLUSH, bufferToFm9, size,
			logger->logger)) {
		log_error_mutex(logger, "Error al enviar solicitud de datos a FM9");
		free(bufferToFm9);
		return false;
	}
	free(bufferToFm9);

	//Se reciben los paquetes del mdj y se envian al fm9
	int result = enviarPkgDeFm9AMdj(path);
	enviarConfirmacionSafa(pid, result, 0, 0, ARCHIVO_GUARDADO);
	return result;
}

int enviarPkgDeFm9AMdj(char * path) {
	//Se recibe el archivo desde FUNES MEMORY
	t_package pkgCantLineas;
	int cantLineasARecibir;
	//Recibo un primer mensaje para saber cuantas lineas voy a recibir del FM9
	if (recibir(t_socketFm9->socket, &pkgCantLineas, logger->logger))
	{
		log_error_mutex(logger, "No se pudo recibir el mensaje del MDJ");
		return EXIT_FAILURE;
	}
	else
	{
		char * buffer = pkgCantLineas.data;
		cantLineasARecibir = copyIntFromBuffer(&buffer);
	}

	int sizeBufferTotal = 0;
	char * bufferTotal; //buffer que se va a cargar con tudo el archivo
	//realizo una iteracion por cada linea
	for (int linea = 0; linea < cantLineasARecibir; linea++) {
		t_package package;
		int cantidadPaquetes;
//		int nroLinea;
		//int tamanioLinea;

		if (recibir(t_socketFm9->socket, &package, logger->logger))
		{
			log_error_mutex(logger, "No se pudo recibir el mensaje del Fm9");
		}
		else
		{
			if (package.code == FM9_DAM_FLUSH)
			{
				//Recibo la cantidad de paquetes que voy a recibir para esta linea
				//tamanioLinea = copyIntFromBuffer(&package.data);
				char * bufferRecibo = package.data;
//				nroLinea = copyIntFromBuffer(&bufferRecibo); //NRO DE LINEA
				cantidadPaquetes = copyIntFromBuffer(&bufferRecibo); //CANTIDAD DE PAQUETES EN LA LINEA
			}
			else
			{
				log_error_mutex(logger, "Error al recibir el tamanio de linea del fm9");
				return EXIT_FAILURE;
			}
			log_info_mutex(logger, "Se recibiran %d paquetes", cantidadPaquetes);
		}


		char * bufferLineaConcatenada = string_new();
		//Ahora se reciben los paquetes y se envia a FILESYSTEM
		for (int i = 0; i < cantidadPaquetes; i++)
		{
			t_package pkgTransferSize;
			if (recibir(t_socketFm9->socket, &pkgTransferSize, logger->logger))
			{
				log_error_mutex(logger, "Error al recibir el paquete %d", i);
				return EXIT_FAILURE;
			}
			else
			{
				//Concatenar buffer
				char * buffer = pkgTransferSize.data;
				char * stringRecibido = copyStringFromBuffer(&buffer);
				string_append(&bufferLineaConcatenada,stringRecibido);
				free(stringRecibido);
			}
		}

		string_append(bufferLineaConcatenada,"\n");

		sizeBufferTotal += strlen(bufferLineaConcatenada);
		bufferTotal = realloc(bufferTotal, sizeBufferTotal);
		char * ptr2 = bufferTotal;
		copyStringToBuffer(&ptr2, bufferLineaConcatenada);
		free(bufferLineaConcatenada);
	}

	log_info_mutex(logger, "Se enviaron todos los datos  del proceso %s,al FileSystem.", path);

	//ENVIO A MDJ
	//ENVIAR DATOS A MDJ: le envio EL PATH, EL INICIO Y EL SIZE A GUARDAR PARA QUE CALCULE LA CANTIDAD DE PAQUETES A ENVIAR
	int sizeOfBuffer = sizeof(int) * 3 + (strlen(path)+1)*sizeof(char);
	char * pkgToMdj = (char *) malloc(sizeOfBuffer);
	char * ptr = pkgToMdj;
	int inicio = 0;
	copyStringToBuffer(&ptr, path);
	copyIntToBuffer(&ptr, inicio);
	copyIntToBuffer(&ptr,strlen(bufferTotal));
	if (enviar(t_socketMdj->socket, DAM_MDJ_HACER_FLUSH, pkgToMdj,
			sizeOfBuffer, logger->logger)) {
		log_error_mutex(logger,
				"Error al enviar cantidad de paquetes a MDJ");
		free(pkgToMdj);
		free(bufferTotal);
		return EXIT_FAILURE;
	}
	free(pkgToMdj);


	int cantPaquetesAEnviar = calcularCantidadPaquetes(strlen(bufferTotal));
	for(int i= 0; i<cantPaquetesAEnviar; i++){
		//tomo el paquete de un tamaño del transfer size
		char * bufferOfTransferSize = string_substring(bufferTotal,i*configDMA->transferSize,((i+1)*configDMA->transferSize-1));

		int size = (strlen(bufferOfTransferSize)+1)*sizeof(char) + sizeof(int);
		char * bufferToMdj = malloc(size);
		char * p = bufferToMdj;
		copyStringToBuffer(&p,bufferOfTransferSize);

		if (enviar(t_socketMdj->socket, DAM_MDJ_GUARDAR_DATOS, bufferToMdj,
				size, logger->logger)) {
			log_error_mutex(logger,
					"Error al enviar el archivo a guardar al MDJ");
			free(bufferToMdj);
			free(bufferTotal);
			return EXIT_FAILURE;
		}
		free(bufferToMdj);
	}
	free(bufferTotal);

	return EXIT_SUCCESS;

}

//-------------------------CREAR ARCHIVO ---------------------------------------------
/*
 * Funcion para solicitar al MDJ la creacion de un archivo
 * params: pid,path,cantidad de lineas
 * return: 1 -> Fail
 * 		   0 -> Success
 */
 bool crearArchivo(t_package paquete, int socketEnUso) {

	//Datos recibidos del cpu
	char * buffer = paquete.data;

	int pid = copyIntFromBuffer(&buffer); //PID
	char * path = copyStringFromBuffer(&buffer); //PATH del archivo a crear
	int cantLineas = copyIntFromBuffer(&buffer); //Cantidad de lineas del archivo a crear

	int size = sizeof(int)*2 + ((strlen(path)+1) * sizeof(char));
	char *bufferEnvio = (char *) malloc(size);
	char * ptr = bufferEnvio;

	copyStringToBuffer(&ptr, path);
	copyIntToBuffer(&ptr, cantLineas);


	//SE ENVIA EL PAQEUTE CON LOS DATOS A MDJ PARA CREAR ARCHIVO
	if (enviar(t_socketMdj->socket, DAM_MDJ_CREAR_ARCHIVO, bufferEnvio, size, logger->logger)) {
		log_error_mutex(logger, "Error al crear el archivo: %s", path);
		free(bufferEnvio);
		enviarConfirmacionSafa(pid, EXIT_FAILURE, 0, 0, ARCHIVO_CREADO);
		return false;
	}

	t_package response;
	if(recibir(t_socketMdj->socket, &response, logger->logger)){
		log_error_mutex(logger, "Error al crear el archivo: %s", path);
		enviarConfirmacionSafa(pid,EXIT_FAILURE,0, 0, ARCHIVO_CREADO);
		return false;
	}

	int result;
	if(response.code == DAM_MDJ_OK){
		result = EXIT_SUCCESS;
	}else{
		result = EXIT_FAILURE;
	}

	//SE ENVIA CONFIRMACION A SAFA
	enviarConfirmacionSafa(pid, result, 0, 0, ARCHIVO_CREADO);
	free(bufferEnvio);
	return true;
}

//--------------------------BORRAR ARCHIVO ------------------------------------------

bool borrarArchivo(t_package paquete, int socketEnUso) {

	//Datos recibidos del cpu
	int pid = copyIntFromBuffer(&paquete.data);
	char * path = copyStringFromBuffer(&paquete.data);

	int size = sizeof(int)*2 + ((strlen(path)+1) * sizeof(char));
	char *buffer = (char *) malloc(size);
	char * p = buffer;
	copyIntToBuffer(&p,pid);
	copyStringToBuffer(&p,path);


	//SE ENVIA EL PATH DEL ARCHIVO AL MDJ PARA BORRAR EL MISMO
	if (enviar(t_socketMdj->socket, DAM_MDJ_BORRAR_ARCHIVO, buffer, size,
			logger->logger)) {
		log_error_mutex(logger,
				"Error al enviar el path al MDJ del archivo a borrar");
		free(buffer);
		enviarConfirmacionSafa(pid, EXIT_FAILURE, 0, 0, ARCHIVO_BORRADO);
		return false;
	}

	free(buffer);
	t_package response;
	if(recibir(t_socketMdj->socket, &response, logger->logger)){
		log_error_mutex(logger, "Error al crear el archivo: %s", path);
		enviarConfirmacionSafa(pid,EXIT_FAILURE, 0, 0, ARCHIVO_CREADO);
		return false;
	}

	int result;
	if(response.code == DAM_MDJ_OK){
		result = EXIT_SUCCESS;
	}else{
		result = EXIT_FAILURE;
	}

	//SE ENVIA LA CONFIRMACION A SAFA
	enviarConfirmacionSafa(pid, result, 0, 0, ARCHIVO_BORRADO);
	return true;
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
		log_info_mutex(logger, "Se realizó el escuchar del CPU");
	}

	log_info_mutex(logger, "Se inicializaron correctamente todas las conexiones");

}

/*FUNCION DEL HILO MDJ
 * Crea UN hilo que queda conectado y a la escucha del S-Afa
 */
int conectarseConSafa() {

	socketSafa = malloc(sizeof(int));
	t_socketSafa = conectarYenviarHandshake(configDMA->puertoSAFA, configDMA->ipSAFA,
			socketSafa, SAFA_HSK, t_socketSafa);
	return 0;
}

/*FUNCION DEL HILO MDJ
 * Crea UN hilo que queda conectado al MDJ
 */
int conectarseConMdj() {
	socketMdj = malloc(sizeof(int));
	t_socketMdj = conectarYenviarHandshake(configDMA->puertoMDJ, configDMA->ipMDJ, socketMdj,
			MDJ_HSK, t_socketMdj);

	//Se envía el transfer size
	int size = sizeof(int);
	char *buffer = (char *) malloc(size);
	char *p = buffer;
	copyIntToBuffer(&p, configDMA->transferSize);

	if (enviar(t_socketMdj->socket, DAM_MDJ_TRANSFER_SIZE, buffer, size,logger->logger)) {
		log_error_mutex(logger, "No se pudo enviar el transfer size al MDJ");
		return EXIT_FAILURE;
	}

	return 0;
}

/*FUNCION DEL HILO FM9
 * Crea UN hilo que queda conectado al FM9
 */
int conectarseConFm9() {

	socketFm9 = malloc(sizeof(int));
	t_socketFm9 = conectarYenviarHandshake(configDMA->puertoFM9, configDMA->ipFM9, socketFm9,
			FM9_HSK, t_socketFm9);
	return 0;
}

int conectarseConCPU() {
	conectarYRecibirHandshake(configDMA->puertoDAM);
	return 0;
}

t_socket * conectarYenviarHandshake(int puerto, char *ip, int * socket,
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
	return TSocket;
}

void conectarYRecibirHandshake(int puertoEscucha) {

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


//------------------------------------------------------------------------------------------------------------------
//		FUNCIONES COMUNES
//------------------------------------------------------------------------------------------------------------------


void enviarConfirmacionSafa(int pid, int result, int cantidadIODelProceso, int cantLineasArchivo, int code){

	int msjCode;
	int size = sizeof(int) * 4;
	char *buffer = (char *) malloc(size);
	char *p = buffer;
	copyIntToBuffer(&p, pid);
	copyIntToBuffer(&p, result);
	copyIntToBuffer(&p,cantidadIODelProceso);
	copyIntToBuffer(&p, cantLineasArchivo);

	switch(code){
	case ARCHIVO_CREADO:
		msjCode = DAM_SAFA_CONFIRMACION_CREAR_ARCHIVO;
		break;

	case ARCHIVO_BORRADO:
		msjCode = DAM_SAFA_CONFIRMACION_BORRAR_ARCHIVO;
		break;

	case ARCHIVO_GUARDADO:
		msjCode = DAM_SAFA_CONFIRMACION_DATOS_GUARDADOS;
		break;
	}

	if (enviar(t_socketSafa->socket, msjCode, buffer, size, logger->logger)) {
		log_error_mutex(logger, "Error al enviar msj de confirmacion al SAFA.");
		free(buffer);
	}

	log_info_mutex(logger, "Mensaje de confirmacion a S-afa enviado");
	free(buffer);
}

void enviarConfirmSafaScriptInit(int pid, int result, int cantidadIODelProceso, int cantLineasArchivo){

	int msjCode;
	int size = sizeof(int) * 4;
	char *buffer = (char *) malloc(size);
	char *p = buffer;
	copyIntToBuffer(&p, pid);
	copyIntToBuffer(&p, result);
	copyIntToBuffer(&p,cantidadIODelProceso);
	copyIntToBuffer(&p, cantLineasArchivo);

	msjCode = DAM_SAFA_CONFIRMACION_SCRIPT_INICIALIZADO;


	if (enviar(t_socketSafa->socket, msjCode, buffer, size, logger->logger)) {
		log_error_mutex(logger, "Error al enviar msj de confirmacion al SAFA.");
		free(buffer);
	}

	log_info_mutex(logger, "Mensaje de confirmacion a S-afa enviado");
	free(buffer);
}

void enviarConfirmSafaAbrirArchivo(int pid, int result, char * path){

	int msjCode;
	int size = sizeof(int) * 3 + (strlen(path)+1)*sizeof(char);
	char *buffer = (char *) malloc(size);
	char *p = buffer;
	copyIntToBuffer(&p, pid);
	copyIntToBuffer(&p, result);
	copyStringToBuffer(&p, path);

	msjCode = DAM_SAFA_CONFIRMACION_PID_CARGADO;


	if (enviar(t_socketSafa->socket, msjCode, buffer, size, logger->logger)) {
		log_error_mutex(logger, "Error al enviar msj de confirmacion al SAFA.");
		free(buffer);
	}

	log_info_mutex(logger, "Mensaje de confirmacion a S-afa enviado");
	free(buffer);
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

int calcularCantidadPaquetes(int sizeOfFile) {
	//Lo divido por mi transfer Size y me quedo con la parte entera
	int cantPart = sizeOfFile / configDMA->transferSize;
	if(sizeOfFile % configDMA->transferSize != 0){
		cantPart++;
	}
//	log_info_mutex(logger, "Se recibiran %d paquetes", cantPart);

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


int confirmarExistenciaFile(){
	//Se recibe el archivo desde filesystem
	t_package package;
	int result = EXIT_FAILURE;

	//Recibo un primer mensaje para saber cuanto pesa el archivo
	if (recibir(t_socketMdj->socket, &package, logger->logger)) {
		log_error_mutex(logger, "No se pudo recibir el mensaje del MDJ");
		return EXIT_FAILURE;
	}

	if (package.code == DAM_MDJ_FAIL) {
		return EXIT_FAILURE;
	}

	//Recibo el tamaño del archivo a cargar
	char * buffer = package.data;
	result = copyIntFromBuffer(&buffer);

	return result;
}

int recibirConfirmacionMemoria() {

	t_package package;

	if (recibir(t_socketFm9->socket, &package, logger->logger)) {
		log_error_mutex(logger, "Error al recibir los datos de memoria asociados al proceso");
		return EXIT_FAILURE;
	}
	if (package.code == FM9_DAM_ESCRIPTORIO_CARGADO) {
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}

int esOperacionDeIO(t_cpu_operacion operacion){
	switch(operacion.keyword){
		case ABRIR: case FLUSH: case CREAR: case BORRAR:
			return true;
		default:
			return false;
	}
}

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
