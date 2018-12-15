/*
 ============================================================================
 Name        : CPU.c
 Author      : Alejandro Walsh
 Version     : 1.0.1
 Copyright   :
 Description : Módulo CPU del Gran TP (Sistemas Operativos, UTN FRBA, 2018)
 ============================================================================
 */
#include "CPU.h"

//int desalojado = 0;
pthread_attr_t tattr;

int main(int argc, char ** argv) {
	// UTILIZAR ESTA LINEA PARA PROBAR EL MÉTODO Y QUE PUEDA ABRIR UN ESCRIPTORIO DE EJEMPLO
	//t_list * lista = parseoInstrucciones(char * path, int cantidadLineas)
	inicializarCPU(argv[1]);
    pthread_t threadPrincipal;
//    sig_handler(SIGINT);
	//Recibir DTB y verificar valor de flag de inicializacion
    pthread_create(&threadPrincipal, &tattr, (void *) recibirDTB, NULL);
	pthread_join(threadPrincipal,NULL);
    exit_gracefully(EXIT_SUCCESS);
}

//void sig_handler(int signal)
//{
//	if (signal == SIGINT)
//	{
//		exit_gracefully(signal);
//	}
//}
void recibirDTB()
{
	while(1)
	{
		// Aca habria que quedarse esperando por un DTB enviado por el SAFA.
		t_package paquete;
		if (recibir(t_socketSAFA->socket,&paquete,loggerCPU->logger)) {
            log_error_mutex(loggerCPU, "No se pudo recibir el mensaje.");
            exit_gracefully(EXIT_FAILURE);
            //handlerDisconnect(i);
        }
		else
		{
			pthread_mutex_lock(&mutexSolicitudes);
			manejarSolicitud(paquete, t_socketSAFA->socket);
			pthread_mutex_unlock(&mutexSolicitudes);
		}
	}
}

void manejarSolicitud(t_package pkg, int socketFD) {

    switch (pkg.code) {
        case SAFA_CPU_EJECUTAR:
        	if(comenzarEjecucion(pkg))
        	{
        		log_error_mutex(loggerCPU, "Hubo un error en la ejecucion del Escriptorio");
        		break;
        	}
        	break;
        case SAFA_CPU_QUANTUM:
        	if(setQuantum(pkg))
        	{
        		log_error_mutex(loggerCPU, "Hubo un error en el seteo del nuevo quantum.");
        		break;
        	}
        	break;
        case SOCKET_DISCONECT:
//            handlerDisconnect(socketFD);
            close(socketFD);
//            deleteSocketFromMaster(socketFD);
            break;
        default:
            log_warning_mutex(loggerCPU, "El mensaje recibido es: %s", codigoIDToString(pkg.code));
            log_warning_mutex(loggerCPU, "Ojo, estas recibiendo un mensaje que no esperabas.");
    		exit_gracefully(ERROR_DTB);
            break;

    }

    free(pkg.data);

}
int nuevoDummy(t_dtb * dtb, t_package paquete)
{
	// Luego de recibirlo tengo que verificar su flag de inicializacion

	// Si es 0, levanto un hilo y realizo  la operación Dummy - Iniciar G.DT
	// Solicitarle al DAM la busqueda del Escriptorio en el MDJ
	int longitudEscriptorio = strlen(dtb->dirEscriptorio) + 1;
	if (longitudEscriptorio <= 0)
	{
		if(enviar(t_socketSAFA->socket,CPU_SAFA_ABORTAR_DTB_NUEVO,paquete.data, paquete.size, loggerCPU->logger))
		{
			log_error_mutex(loggerCPU, "No se pudo enviar el ABORTA del Dummy al S-AFA.");
			return EXIT_FAILURE;
		}
		return EXIT_FAILURE;
	}
	int size = 2*sizeof(int) + (longitudEscriptorio * sizeof(char));
	char *buffer = (char *) malloc(size);
	char * p = buffer;
	copyIntToBuffer(&p, dtb->idGDT);
	copyStringToBuffer(&p, dtb->dirEscriptorio);
	if(enviar(t_socketDAM->socket,CPU_DAM_BUSQUEDA_ESCRIPTORIO,buffer, size, loggerCPU->logger))
	{
		log_error_mutex(loggerCPU, "No se pudo enviar la busqueda del escriptorio al DAM.");
		free(buffer);
		return EXIT_FAILURE;
	}
	free(buffer);
	log_info_mutex(loggerCPU, "Se envió el pedido de búsqueda de escriptorio del proceso %d al DAM.", dtb->idGDT);

	// Desalojar al DTB Dummy, avisando a SAFA que lo bloquee
	if(enviar(t_socketSAFA->socket,CPU_SAFA_BLOQUEAR_DUMMMY,paquete.data, paquete.size, loggerCPU->logger))
	{
		log_error_mutex(loggerCPU, "No se pudo enviar el bloqueo del Dummy al S-AFA.");
		return EXIT_FAILURE;
	}
	log_info_mutex(loggerCPU, "Se envió el pedido de bloqueo del proceso %d al SAFA.", dtb->idGDT);
	free(dtb);
	return EXIT_SUCCESS;
}

int comenzarEjecucion(t_package paquete)
{
	// Luego de recibirlo tengo que verificar su flag de inicializacion
	t_dtb * dtb = transformarPaqueteADTB(paquete);
	if (dtb->flagInicializado == 0)
	{
		if(nuevoDummy(dtb, paquete))
		{
			log_error_mutex(loggerCPU, "Hubo un error en la inicializacion del dummy");
			return EXIT_FAILURE;
		}
	}
	else if (dtb->flagInicializado == 1)
	{
		if(realizarEjecucion(dtb))
		{
			log_error_mutex(loggerCPU, "Hubo un error en la ejecucion del DTB");
			return EXIT_FAILURE;
		}
	}
	else
	{
		log_error_mutex(loggerCPU, "Error: el DTB tiene alguna falla.");
		// TODO: revisar si debería morir el CPU por esto o si simplemente deberia mandarlo al otro método
		exit_gracefully(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}

int realizarEjecucion(t_dtb * dtb)
{
	bool finEjecucion = false;
	bool segFaultDTB = false;
	// Si es 1, levanto un hilo y comienzo la ejecución de sentencias
	// if DTB->flagInicializado == 1
	// Realizar las ejecuciones correspondientes definidas por el quantum de SAFA
	// Por cada unidad de tiempo de quantum, se ejecutara una linea del Escriptorio indicado en el DTB
	pthread_mutex_lock(&mutexQuantum);
	int periodoEjecucion = 0;
	while(periodoEjecucion < quantum)
	{
		// RETARDO DE EJECUCION:
		usleep(config->retardo * 1000);
		// Comunicarse con el FM9 en caso de ser necesario.
		// Si el FM9 indica un acceso invalido o error, se aborta el DTB informando a SAFA para que
		// lo pase a la cola de Exit.

		log_info_mutex(loggerCPU, "Se va a ejecutar la operacion %d del proceso %d.",dtb->programCounter, dtb->idGDT);
		t_cpu_operacion operacion = obtenerInstruccionMemoria(dtb->dirEscriptorio, dtb->idGDT, dtb->programCounter);
		if (operacion.valido)
		{
			int quantumRestante = (quantum - periodoEjecucion)-1;
			dtb->quantumRestante = quantumRestante;
			if (operacion.keyword != CONCENTRAR)
				dtb->programCounter++;
			int respuesta = ejecutarOperacion(&operacion, &dtb);
			switch(respuesta)
			{
				case EXIT_FAILURE:
					log_error_mutex(loggerCPU, "Ha ocurrido un error durante la ejecucion de una operacion.");
					segFaultDTB = true;
					if (finalizoEjecucionDTB(dtb, CPU_SAFA_ABORTAR_DTB))
					{
						log_error_mutex(loggerCPU, "Hubo un error en el envio del mensaje al SAFA.");
					}
					break;
				case CONCENTRAR_EJECUTADO:
					periodoEjecucion++;
					if (dtb->programCounter == dtb->cantidadLineas)
					{
						periodoEjecucion = quantum;
						finEjecucion = true;
					}
					continue;
					break;
				default:
					break;
			}
			if (respuesta == DTB_DESALOJADO)
			{
				break;
			}

			if (dtb->programCounter == dtb->cantidadLineas)
			{
				periodoEjecucion = quantum;
				finEjecucion = true;
			}
			else
			{
				periodoEjecucion++;
			}
		}
		else
		{
			log_info_mutex(loggerCPU, "Se aborta el proceso %d porque la operacion %d es inválida.", dtb->idGDT, dtb->programCounter);
			if(finalizoEjecucionDTB(dtb, CPU_SAFA_ABORTAR_DTB))
			{
				log_error_mutex(loggerCPU, "Hubo un error en la finalización de la ejecución del DTB.");
			}
			break;
		}
	}
	pthread_mutex_unlock(&mutexQuantum);
	if (!segFaultDTB)
	{
		if (finEjecucion)
		{
			periodoEjecucion--;
		}
		// TODO: Recibir la cantidad de lineas real del DTB, ahora voy a forzar que no entre para evitar problemas.
		//dtb->cantidadLineas = dtb->programCounter + 1;
		if (dtb->programCounter == dtb->cantidadLineas)
		{
			if(finalizoEjecucionDTB(dtb, CPU_SAFA_FIN_EJECUCION_DTB))
			{
				log_error_mutex(loggerCPU, "Hubo un error en la finalización de la ejecución del DTB.");
			}
		}
		else if (periodoEjecucion == quantum)
		{
			if(finalizoEjecucionDTB(dtb, CPU_SAFA_FIN_EJECUCION_X_QUANTUM_DTB))
			{
				log_error_mutex(loggerCPU, "Hubo un error en la finalización de la ejecución del DTB por quantum.");
			}
		}
	}
	free(dtb);
	return EXIT_SUCCESS;
}

t_cpu_operacion obtenerInstruccionMemoria(char * direccionEscriptorio, int idGDT, int posicion)
{
	int size = strlen(direccionEscriptorio)+1 + 3*sizeof(int);
	char * buffer = malloc(size);
	char * p = buffer;
	copyStringToBuffer(&p, direccionEscriptorio);
	copyIntToBuffer(&p, idGDT);
	copyIntToBuffer(&p, posicion);
	if (enviar(t_socketFM9->socket,CPU_FM9_DAME_INSTRUCCION,buffer,size,loggerCPU->logger))
	{
		log_error_mutex(loggerCPU, "Hubo un error en el envio de la accion dame instruccion al FM9.");
	}
	t_package paquete;
	if (recibir(t_socketFM9->socket, &paquete, loggerCPU->logger))
	{
		log_error_mutex(loggerCPU, "Hubo un error la recepción de la instruccion desde el FM9.");
	}
	char * bufferRecepcion = paquete.data;
	char * instruccion = copyStringFromBuffer(&bufferRecepcion);
	t_cpu_operacion operacion = parse(instruccion, false);
	return operacion;
}

int finalizoEjecucionDTB(t_dtb * dtb, int code)
{
	if (eventoSAFA(&dtb, code))
	{
		log_error_mutex(loggerCPU, "Hubo un error en el envio del mensaje al SAFA.");
		return EXIT_FAILURE;
	}
	if (code == CPU_SAFA_FIN_EJECUCION_DTB)
	{
		if (finEjecucionFM9(dtb->idGDT))
		{
			log_error_mutex(loggerCPU, "Hubo un error en el envio de la finalizacion del proceso al FM9.");
			return EXIT_FAILURE;
		}
		log_info_mutex(loggerCPU, "Se ha finalizado la ejecucion del proceso %d.", dtb->idGDT);
	}
	else
	{
		if (code == CPU_SAFA_FIN_EJECUCION_X_QUANTUM_DTB)
			log_info_mutex(loggerCPU, "Se ha desalojado al proceso %d por fin de quantum.", dtb->idGDT);
		if (code == CPU_SAFA_BLOQUEAR_DTB)
			log_info_mutex(loggerCPU, "Se ha desalojado al proceso %d por realizar una operación de E/S.", dtb->idGDT);
	}
	return EXIT_SUCCESS;
}

int finEjecucionFM9(int idGDT)
{
	int code = CPU_FM9_FIN_GDT;
	int size = sizeof(int);
	char * buffer = malloc(size);
	char * p = buffer;
	copyIntToBuffer(&p, idGDT);
	if (enviar(t_socketFM9->socket, code, buffer, size, loggerCPU->logger)){
		return EXIT_FAILURE;
	}
	t_package pkg;
	if (recibir(t_socketFM9->socket, &pkg, loggerCPU->logger))
	{
		return EXIT_FAILURE;
	}
	if (pkg.code == FM9_CPU_GDT_FINALIZADO)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}

int ejecutarOperacion(t_cpu_operacion * operacion, t_dtb ** dtb)
{
	int respuesta = 1;
	// Aca habría que diferenciar la ejecucion dependiendo de la accion que venga
	switch(operacion->keyword)
	{
		case CONCENTRAR:
			(*dtb)->programCounter++;
			return CONCENTRAR_EJECUTADO;
		case WAIT:
			if(manejarRecursosSAFA(operacion->argumentos.WAIT.recurso, (*dtb)->idGDT, WAIT))
			{
				log_error_mutex(loggerCPU, "No se pudo realizar el WAIT del recurso %s.", operacion->argumentos.WAIT.recurso);
			}
			break;
		case SIGNAL:
			if(manejarRecursosSAFA(operacion->argumentos.SIGNAL.recurso, (*dtb)->idGDT, SIGNAL))
			{
				log_error_mutex(loggerCPU, "No se pudo realizar el SIGNAL del recurso %s.", operacion->argumentos.SIGNAL.recurso);
			}
			break;
		case ABRIR: case FLUSH: case CREAR: case BORRAR:
			respuesta = enviarAModulo(operacion, dtb, operacion->keyword, DAM);
			if(respuesta == EXIT_FAILURE)
			{
				log_error_mutex(loggerCPU, "No se pudo enviar al DAM la operacion indicada.");
			}
			return respuesta;
			break;
		case ASIGNAR: case CLOSE:
			respuesta = enviarAModulo(operacion, dtb, operacion->keyword, FM9);
			if(respuesta == EXIT_FAILURE)
			{
				log_error_mutex(loggerCPU, "No se pudo enviar al FM9 la operacion indicada.");
			}
			return respuesta;
		default:
			log_error_mutex(loggerCPU, "Error: operacion desconocida.");
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int eventoSAFA(t_dtb ** dtb, int code)
{
	t_package paquete = transformarDTBAPaquete((*dtb));
	if(enviar(t_socketSAFA->socket, code, paquete.data, paquete.size, loggerCPU->logger))
	{
		log_error_mutex(loggerCPU, "Hubo un error en el envío del paquete al SAFA");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

t_package crearPaqueteSegunAccion(int accion, t_cpu_operacion * operacion, t_dtb ** dtb)
{
	t_package paquete;
	char * buffer = malloc(sizeof(char));
	char * p;
	switch(accion)
	{
		case ABRIR:
			paquete.code = CPU_DAM_ABRIR_ARCHIVO;
			paquete.size = (strlen(operacion->argumentos.ABRIR.path) + 1 * sizeof(char)) + 2*sizeof(int);
			buffer = realloc(buffer, paquete.size);
			p = buffer;
			copyIntToBuffer(&p, (*dtb)->idGDT);
			copyStringToBuffer(&p, operacion->argumentos.ABRIR.path);
			break;
		case FLUSH:
			paquete.code = CPU_DAM_FLUSH;
			paquete.size = (strlen(operacion->argumentos.FLUSH.path) + 1  * sizeof(char)) + 2*sizeof(int);
			buffer = realloc(buffer, paquete.size);
			p = buffer;
			copyIntToBuffer(&p, (*dtb)->idGDT);
			copyStringToBuffer(&p, operacion->argumentos.FLUSH.path);
			break;
		case CREAR:
			paquete.code = CPU_DAM_CREAR;
			paquete.size = (strlen(operacion->argumentos.CREAR.path)  + 1 * sizeof(char)) + 3*sizeof(int);
			buffer = realloc(buffer, paquete.size);
			p = buffer;
			copyIntToBuffer(&p,(*dtb)->idGDT);
			copyStringToBuffer(&p, operacion->argumentos.CREAR.path);
			copyIntToBuffer(&p, operacion->argumentos.CREAR.linea);
			break;
		case BORRAR:
			paquete.code = CPU_DAM_BORRAR;
			paquete.size = (strlen(operacion->argumentos.BORRAR.path) + 1)* sizeof(char) + 2*sizeof(int);
			buffer = realloc(buffer, paquete.size);
			p = buffer;
			copyIntToBuffer(&p,(*dtb)->idGDT);
			copyStringToBuffer(&p, operacion->argumentos.BORRAR.path);
			break;
		case ASIGNAR:
			paquete.code = CPU_FM9_ASIGNAR;
			paquete.size = (strlen(operacion->argumentos.ASIGNAR.path) + 1 * sizeof(char)) + 4 * sizeof(int) + (strlen(operacion->argumentos.ASIGNAR.datos) + 1 * sizeof(char));
			buffer = realloc(buffer, paquete.size);
			p = buffer;
			copyIntToBuffer(&p, (*dtb)->idGDT);
			copyStringToBuffer(&p, operacion->argumentos.ASIGNAR.path);
			copyIntToBuffer(&p, operacion->argumentos.ASIGNAR.linea);
			copyStringToBuffer(&p, operacion->argumentos.ASIGNAR.datos);
			break;
		case CLOSE:
			paquete.code = CPU_FM9_CERRAR_ARCHIVO;
			paquete.size = (strlen(operacion->argumentos.CLOSE.path) + 1 * sizeof(char)) + 2*sizeof(int);
			buffer = realloc(buffer, paquete.size);
			p = buffer;
			copyIntToBuffer(&p, (*dtb)->idGDT);
			copyStringToBuffer(&p, operacion->argumentos.CLOSE.path);
			break;
		default:
			break;
	}
	paquete.data = buffer;
	return paquete;
}

int enviarAModulo(t_cpu_operacion * operacion, t_dtb ** dtb, int accion, int modulo)
{
	int socket = 0;
	// Segun la accion que se quiera realizar, se establece el codigo, el size y se llena el buffer.
	t_package paquete = crearPaqueteSegunAccion(accion, operacion, dtb);
	if (accion == ABRIR)
	{
		// TODO: Chequear la conversion de DTB a Package y viceversa para replicar correctamente la tabla de direcciones.
		pthread_mutex_lock(&mutexPath);
		pathBuscado = operacion->argumentos.ABRIR.path;
		if (!list_is_empty((*dtb)->tablaDirecciones) && list_any_satisfy((*dtb)->tablaDirecciones, (void *) encontrarPath))
		{
			pthread_mutex_unlock(&mutexPath);
			log_info_mutex(loggerCPU, "El archivo %s ya está abierto por el proceso %d.", operacion->argumentos.ABRIR.path, (*dtb)->idGDT);
			return EXIT_SUCCESS;
		}
		pthread_mutex_unlock(&mutexPath);
	}
	int respuesta = 0;
	// Esta validacion es para que sólo se bloquee el GDT cuando la accion implica una llamada al DAM.
	if (modulo == DAM)
	{
		socket = t_socketDAM->socket;
		respuesta = ejecucionDAM(dtb);
	}
	// Esta validacion es para esperar una respuesta del FM9 y verificar que no haya errores ni accesos inválidos.
	if (modulo == FM9)
	{
		socket = t_socketFM9->socket;
	}
	if (paquete.code < 1 || strlen(paquete.data) < 1 || paquete.size < 1 || socket < 1)
	{
		log_error_mutex(loggerCPU, "Hubo un error al intentar crear el paquete para el envio.");
		return EXIT_FAILURE;
	}

	// Aca se realiza el envío de la operacion que se está ejecutando actualmente
	if(enviar(socket, paquete.code, paquete.data, paquete.size, loggerCPU->logger))
	{
		log_error_mutex(loggerCPU, "Hubo un error en el envío del paquete.");
		return EXIT_FAILURE;
	}

	if (modulo == DAM)
	{
		return respuesta;
	}
	if (modulo == FM9)
	{
		return ejecucionFM9(dtb, socket);
	}
	return EXIT_SUCCESS;
}

bool encontrarPath(char * direccion)
{
	if (strcmp(direccion, pathBuscado) == 0)
		return true;
	return false;
}

int ejecucionDAM(t_dtb ** dtb)
{
	if (finalizoEjecucionDTB((*dtb), CPU_SAFA_BLOQUEAR_DTB))
	{
		log_error_mutex(loggerCPU, "Hubo un error en el envio de bloqueo del G.DT.");
		return EXIT_FAILURE;
	}
	return DTB_DESALOJADO;
}

int ejecucionFM9(t_dtb ** dtb, int socket)
{
	t_package package;
	if(recibir(socket, &package, loggerCPU->logger))
	{
		log_error_mutex(loggerCPU, "Hubo un error al recibir la respuesta del FM9.");
		return EXIT_FAILURE;
	}
	if(package.code == (FM9_CPU_ACCESO_INVALIDO || FM9_CPU_PROCESO_INEXISTENTE || FM9_CPU_FALLO_SEGMENTO_MEMORIA))
	{
		log_warning_mutex(loggerCPU, "Hubo un error en el FunesMemory9.");
		if (eventoSAFA(dtb, CPU_SAFA_ABORTAR_DTB))
		{
			log_error_mutex(loggerCPU, "Hubo un error en el envio de finalización del G.DT.");
			return EXIT_FAILURE;
		}
		return DTB_DESALOJADO;
	}
	if (package.code == FM9_CPU_LINEA_GUARDADA)
	{
		log_info_mutex(loggerCPU, "La linea correspondiente se ha guardado correctamente en memoria.");
	}
	if (package.code == FM9_CPU_ARCHIVO_CERRADO)
	{
		log_info_mutex(loggerCPU, "El archivo correspondiente ha sido cerrado correctamente.");
	}
	return EXIT_SUCCESS;
}

int manejarRecursosSAFA(char * recurso, int idGDT, int accion)
{
	int code;
	if (accion == WAIT)
	{
		code = CPU_SAFA_WAIT_RECURSO;
	}
	else if (accion == SIGNAL)
	{
		code = CPU_SAFA_SIGNAL_RECURSO;
	}
	else
	{
		return EXIT_FAILURE;
	}
	// Enviar la informacion del recurso y del idGDT que lo bloquea (o desbloquea) al SAFA
	int size = (strlen(recurso) + 1) * sizeof(char) + 2*sizeof(int);
	char * buffer = malloc(size);
	char * p = buffer;
	copyStringToBuffer(&p, recurso);
	copyIntToBuffer(&p,idGDT);
	if(enviar(t_socketSAFA->socket,code,buffer, size, loggerCPU->logger))
	{
		log_error_mutex(loggerCPU, "No se pudo enviar el bloqueo o desbloqueo del recurso al SAFA..");
		free(buffer);
		return EXIT_FAILURE;
	}
	free(buffer);
	return EXIT_SUCCESS;
}

int setQuantum(t_package paquete)
{
    pthread_mutex_lock(&mutexQuantum);
    char * buffer = paquete.data;
	quantum = copyIntFromBuffer(&buffer);
	if (quantum > 0)
	{
		log_info_mutex(loggerCPU, "El valor del quantum es %d", quantum);
	    pthread_mutex_unlock(&mutexQuantum);
		return EXIT_SUCCESS;
	}
    pthread_mutex_unlock(&mutexQuantum);
	return EXIT_FAILURE;
}

void inicializarCPU(char * pathConfig)
{
	loggerCPU = log_create_mutex("CPU.log", "CPU", true, LOG_LEVEL_INFO);
	if (pathConfig != NULL)
	{
		config = cargarConfiguracion(pathConfig, CPU, loggerCPU->logger);
	}
	else
	{
		log_error_mutex(loggerCPU, "No hay un path correcto a un archivo de configuracion");
		exit_gracefully(ERROR_PATH_CONFIG);
	}
	if (config != NULL)
	{
		inicializarConexiones();
	}
	else
	{
		log_error_mutex(loggerCPU, "Error en el archivo de configuracion");
		exit_gracefully(ERROR_CONFIG);
	}
}

void inicializarConexiones()
{
	socketSAFA = malloc(sizeof(int));
	t_socketSAFA = conectarseAProceso(config->puertoSAFA,config->ipSAFA,socketSAFA,SAFA_HSK);
	free(socketSAFA);
	socketFM9 = malloc(sizeof(int));
	t_socketFM9 = conectarseAProceso(config->puertoFM9,config->ipFM9,socketFM9,FM9_HSK);
	free(socketFM9);
	socketDAM = malloc(sizeof(int));
	t_socketDAM = conectarseAProceso(config->puertoDAM,config->ipDAM,socketDAM,DAM_HSK);
	free(socketDAM);
}

// Se cambió el método y ahora devuelve el t_socket debido a que pasandolo como referencia,
// a veces no persistian los valores del socket.
t_socket * conectarseAProceso(int puerto, char *ip, int * socket, int handshakeProceso)
{
	t_socket * TSocket;
	if(!cargarSocket(puerto,ip,socket,loggerCPU->logger))
	{
		TSocket = inicializarTSocket(*socket, loggerCPU->logger);
		enviarHandshake(TSocket->socket,CPU_HSK,handshakeProceso,loggerCPU->logger);
	}
	else
	{
		log_error_mutex(loggerCPU, "Error al conectarse al %s", enumToProcess(handshakeProceso));
		exit_gracefully(ERROR_SOCKET);
	}
	return TSocket;
}

char * enumToProcess(int proceso)
{
	char * nombreProceso = "";
	switch(proceso)
	{
		case FM9_HSK:
			nombreProceso = "FM9";
			break;
		case DAM_HSK:
			nombreProceso = "DAM";
			break;
		case SAFA_HSK:
			nombreProceso = "SAFA";
			break;
		default:
			log_error_mutex(loggerCPU, "Enum proceso error.");
			break;
	}
	if (strcmp(nombreProceso,"") == 0)
		exit_gracefully(-1);
	return nombreProceso;
}

void exit_gracefully(int error)
{
	liberarMemoriaTSocket(t_socketDAM);
	liberarMemoriaTSocket(t_socketSAFA);
	liberarMemoriaTSocket(t_socketFM9);
	pthread_mutex_destroy(&mutexQuantum);
	pthread_mutex_destroy(&mutexPath);
	pthread_mutex_destroy(&mutexSolicitudes);
	log_destroy_mutex(loggerCPU);
	freeConfig(config, CPU);
	exit(error);
}

void liberarMemoriaTSocket(t_socket * TSocket)
{
	if(TSocket != NULL && TSocket->socket != 0)
	{
		close(TSocket->socket);
		free(TSocket);
	}
}

void initMutexs(){
	pthread_mutex_init(&mutexQuantum, NULL);
	pthread_mutex_init(&mutexPath, NULL);
	pthread_mutex_init(&mutexSolicitudes, NULL);
}
