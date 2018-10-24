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

        case CPU_FM9_CONNECT:
        	cpusConectadas ++;
			printf("Se ha conectado el CPU. CPU conectadas: %d", cpusConectadas);
            break;

            //Se carga un escriptorio en memoria
            //Params: path del archivo y pid del DTB
        case CPU_DAM_BUSQUEDA_ESCRIPTORIO:
        	//TODO COMUNICARME CON EL FM9 Y TRANSFERIR LOS DATOS DEL ARCHIVO AL MDJ
        	if(leerEscriptorio(pkg,socketFD)){
				log_error_mutex(logger, "Hubo un error en la inicializacion del escriptorio");
				break;
			}
			break;

        	//TODO AVISARLE AL SAFA QUE EL ARCHIVO YA ESTA CARGADO EN MEMORIA
        	//ENVIARLE LOS DATOS DE LA MEMORIA, EN EL DTB, PARA QUE PUEDA CONSULTARLOS

        	//EN CASO DE ERROR AVISARLE AL SAFA QUE HUBO UN ERROR
        	break;

        //Procedimiento para abrir un archivo (Escritura en el FM9)
        //Params: Path del archivo a cargar
        case CPU_DAM_ABRIR_ARCHIVO:
        	//TODO: BUSCAR EL CONTENIDO DEL PATH RECIBIDO EN FM9 Y CARGARLO EN MDJ
        	printf("Abrir el archivo: ");
        	break;

        //Procedimiento para escribir en MDJ
        //Params:
        //		path --> donde guardar los datos
        //		posicion en memoria -> donde buscar los datos
        case CPU_DAM_FLUSH:
        	//TODO: OBTENER LOS DATOS DEL FM9 Y GUARDARLO EN EL PATH RECIBIDO
        	printf("CPU -> Flush: ");
        	break;

        case CPU_DAM_CREAR:
        	printf("CPU -> Crear: ");
        	break;

        case CPU_DAM_BORRAR:
        	printf("CPU -> Borrar: ");
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

int leerEscriptorio(t_package paquete, int socketEnUso){

	//Datos para el safa
	int baseMemoriaDeEscriptorio = -1;

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

	//Se recibe el archivo desde filesystem
	t_package package;

	if(recibir(t_socketMdj->socket, &package, logger->logger)){
		log_error_mutex(logger, "No se pudo recibir el mensaje del CPU");
	}else{

		int sizeOfFile = (int) package.data;
		//Recibo el tamaño del archivo a cargar
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
		if(enviar(t_socketFm9->socket,DAM_FM9_CARGAR_ESCRIPTORIO,buffer,size,logger->logger)){
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
			//O SI RECIBO UN PAQUETE CON TODO EL ARCHIVO EN EL
			t_package pkgTransferSize;
			//bufferTransferSize.size = 16; TODO: ver si setea un maximo

			if(recibir(t_socketMdj->socket, &pkgTransferSize, logger->logger)){
				log_error_mutex(logger, "Error al recibir el paquete %d",i);
				//SE ENVIA UN FALLO AL SAFA PARA EL PROCESO [pid
				enviarConfirmacionASafa(pid,0,baseMemoriaDeEscriptorio);
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

		if(baseMemoriaDeEscriptorio >= 0){
			//Se envia la confirmacion a SAFA del proceso y la posicion en memoria
			enviarConfirmacionASafa(pid,1,baseMemoriaDeEscriptorio);
		}else{
			//Comunicarle un error al safa
			enviarConfirmacionASafa(pid,0,baseMemoriaDeEscriptorio);
		}

	}

	free(keyCompress);

	return EXIT_SUCCESS;
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

void enviarConfirmacionASafa(int pid,int itsLoaded, int base){

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
