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
	//Recibir DTB y verificar valor de flag de inicializacion
    pthread_create(&threadPrincipal, &tattr, (void *) recibirDTB, NULL);
	exit_gracefully(EXIT_SUCCESS);
}

void recibirDTB()
{
	while(1)
	{
		// Aca habria que quedarse esperando por un DTB enviado por el SAFA.
		t_package paquete;
		if (recibir(t_socketSAFA->socket,&paquete,loggerCPU->logger)) {
            log_error_mutex(loggerCPU, "No se pudo recibir el mensaje.");
            //handlerDisconnect(i);
        }
		else
		{
			manejarSolicitud(paquete, t_socketSAFA->socket);
		}
	}
}

void manejarSolicitud(t_package pkg, int socketFD) {

    switch (pkg.code) {
        case SAFA_CPU_NUEVO_DUMMY:
        	if(nuevoDummy(pkg)){
                log_error_mutex(loggerCPU, "Hubo un error en la inicializacion del dummy");
                break;
        	}
        	//sem_post(&sem_nuevoDummy);
            break;
        case SAFA_CPU_EJECUTAR:
        	if(comenzarEjecucion(pkg))
        	{
        		log_error_mutex(loggerCPU, "Hubo un error en la ejecucion del Escriptorio");
        		break;
        	}
        	//sem_post(&sem_comienzaEjecucion);
        	break;
        case SAFA_CPU_QUANTUM:
        	if(setQuantum(pkg))
        	{
        		log_error_mutex(loggerCPU, "Hubo un error en el seteo del nuevo quantum.");
        		break;
        	}
        	//sem_post(&sem_nuevoQuantum);
        	break;
//        case COORD_PLAN_BLOCK:
//            //log_info_mutex(logger, "El coordinador me pide que bloquee un recurso");
//            if (blockKey(socketFD, pkg, logger)) {
//                log_error_mutex(logger, "No se pudo completar la operacion de bloqueo");
//            }
//            break;
//        case COORD_PLAN_STORE:
//            //log_info_mutex(logger, "El coordinador me pide que desbloque un recurso");
//            if (storeKey(socketFD, pkg, logger)) {
//                log_error_mutex(logger, "No se pudo completar la operacion de desbloqueo");
//            }
//            break;
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
int nuevoDummy(t_package paquete)
{
	// Luego de recibirlo tengo que verificar su flag de inicializacion
	t_dtb * dtb = transformarPaqueteADTB(paquete);
	// Si es 0, levanto un hilo y realizo  la operación Dummy - Iniciar G.DT
	// Solicitarle al DAM la busqueda del Escriptorio en el MDJ
	char *buffer;
	int size = sizeof(int) + strlen(dtb->dirEscriptorio);
	copyIntToBuffer(&buffer, dtb->idGDT);
	copyStringToBuffer(&buffer, dtb->dirEscriptorio);
	if(enviar(t_socketDAM->socket,CPU_DAM_BUSQUEDA_ESCRIPTORIO,buffer, size, loggerCPU->logger))
	{
		log_error_mutex(loggerCPU, "No se pudo enviar la busqueda del escriptorio al DAM.");
		free(buffer);
		return EXIT_FAILURE;
	}
	free(buffer);

	// Desalojar al DTB Dummy, avisando a SAFA que lo bloquee
	if(enviar(t_socketSAFA->socket,CPU_SAFA_BLOQUEAR_DUMMMY,paquete.data, paquete.size, loggerCPU->logger))
	{
		log_error_mutex(loggerCPU, "No se pudo enviar el bloqueo del Dummy al S-AFA.");
		free(buffer);
		return EXIT_FAILURE;
	}
	free(buffer);
	free(dtb);
	return EXIT_SUCCESS;
}

t_list * parseoInstrucciones(char * path, int cantidadLineas)
{
	FILE * fp = fopen(path, "r");
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	if (fp == NULL){
		log_error_mutex(loggerCPU, "Error al abrir el archivo: ");
		exit_gracefully(EXIT_FAILURE);
	}
	t_list * listaInstrucciones = list_create();
	int i = 1;
	while ((read = getline(&line, &len, fp)) != -1)
	{
		bool ultimaLinea = (i == cantidadLineas);
		t_cpu_operacion parsed = parse(line, ultimaLinea);
		if(parsed.valido)
		{
			if(!parsed.esComentario)
				list_add(listaInstrucciones, &parsed);

			destruir_operacion(parsed);
			// PRUEBA TEMPORAL PARA VERIFICAR QUE NO DESTRUYE LA REFERENCIA QUE SE AGREGÓ EN LISTA INSTRUCCIONES
			t_cpu_operacion * operacion = list_get(listaInstrucciones, 0);
			printf("Accion: %d", operacion->keyword);
			printf("Argumento 1: %s", operacion->argumentos.ABRIR.path);
		}
		else
		{
			log_error_mutex(loggerCPU, "La linea <%d> no es valida\n", i);
			exit_gracefully(EXIT_FAILURE);
		}
		i++;
	}

	fclose(fp);
	if (line)
		free(line);

	return listaInstrucciones;

}
int comenzarEjecucion(t_package paquete)
{
	// Luego de recibirlo tengo que verificar su flag de inicializacion
	t_dtb * dtb = transformarPaqueteADTB(paquete);
	if (dtb->flagInicializado != 1)
	{
		log_error_mutex(loggerCPU, "Error: el DTB no ha sido inicializado.");
		// TODO: revisar si debería morir el CPU por esto o si simplemente deberia mandarlo al otro método
		exit_gracefully(EXIT_FAILURE);
	}
	// Si es 1, levanto un hilo y comienzo la ejecución de sentencias
	// if DTB->flagInicializado == 1
	t_list * listaInstrucciones = parseoInstrucciones(dtb->dirEscriptorio, dtb->cantidadLineas);
	// Realizar las ejecuciones correspondientes definidas por el quantum de SAFA
	// Por cada unidad de tiempo de quantum, se ejecutara una linea del Escriptorio indicado en el DTB
    pthread_mutex_lock(&mutexQuantum);
	while(dtb->programCounter < quantum)
	{
		// Comunicarse con el FM9 en caso de ser necesario.
		// Si el FM9 indica un acceso invalido o error, se aborta el DTB informando a SAFA para que
		// lo pase a la cola de Exit.
		t_cpu_operacion * operacion = list_get(listaInstrucciones, dtb->programCounter);
		int respuesta = ejecutarOperacion(operacion, &dtb);
		switch(respuesta)
		{
			case EXIT_FAILURE:
				log_error_mutex(loggerCPU, "Ha ocurrido un error durante la ejecucion de una operacion.");
				exit_gracefully(EXIT_FAILURE);
				break;
			case CONCENTRAR_EJECUTADO:
				continue;
				break;
			default:
				break;
		}
//		pthread_mutex_lock(&mutexAbrir);
		if (respuesta == DTB_DESALOJADO)
		{
			break;
		}
//	    pthread_mutex_unlock(&mutexDesalojo);
		dtb->programCounter++;
	}
    pthread_mutex_unlock(&mutexQuantum);
    free(dtb);
	return EXIT_SUCCESS;
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
				break;
			}
			break;
		case SIGNAL:
			if(manejarRecursosSAFA(operacion->argumentos.SIGNAL.recurso, (*dtb)->idGDT, SIGNAL))
			{
				log_error_mutex(loggerCPU, "No se pudo realizar el SIGNAL del recurso %s.", operacion->argumentos.SIGNAL.recurso);
				break;
			}
			break;
		case ABRIR: case FLUSH: case CREAR: case BORRAR:
			respuesta = enviarAModulo(operacion, dtb, operacion->keyword, DAM);
			if(respuesta)
			{
				log_error_mutex(loggerCPU, "No se pudo enviar al DAM la operacion indicada.");
				break;
			}
			return respuesta;
			break;
		case ASIGNAR: case CLOSE:
			respuesta = enviarAModulo(operacion, dtb, operacion->keyword, FM9);
			if(respuesta)
			{
				log_error_mutex(loggerCPU, "No se pudo enviar al FM9 la operacion indicada.");
				break;
			}
			return respuesta;
		default:
			log_error_mutex(loggerCPU, "Error: operacion desconocida.");
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int eventoSAFA(t_dtb ** dtb, int accion, int code)
{
//	int code = CPU_SAFA_BLOQUEAR_DTB;
	char *buffer;
	int size = sizeof(int);
	copyIntToBuffer(&buffer, (*dtb)->idGDT);
	if(enviar(t_socketSAFA->socket, code, buffer, size, loggerCPU->logger))
	{
		log_error_mutex(loggerCPU, "Hubo un error en el envío del paquete al SAFA");
		return EXIT_FAILURE;
	}
//	if (accion == ABRIR)
//	{
//		pthread_mutex_lock(&mutexDesalojo);
//		desalojado = 1;
	//	pthread_mutex_unlock(&mutexAbrir);
//	}
	free(buffer);
	return EXIT_SUCCESS;
}

int enviarAModulo(t_cpu_operacion * operacion, t_dtb ** dtb, int accion, int modulo)
{
	int code = 0, size = 0, socket = 0;
	char * buffer;
	// Segun la accion que se quiera realizar, se establece el codigo, el size y se llena el buffer.
	switch(accion)
	{
		case ABRIR:
			code = CPU_DAM_ABRIR_ARCHIVO;
			size = strlen(operacion->argumentos.ABRIR.path);
			copyStringToBuffer(&buffer, operacion->argumentos.ABRIR.path);
			break;
		case FLUSH:
			code = CPU_DAM_FLUSH;
			size = strlen(operacion->argumentos.FLUSH.path);
			copyStringToBuffer(&buffer, operacion->argumentos.FLUSH.path);
			break;
		case CREAR:
			code = CPU_DAM_CREAR;
			size = strlen(operacion->argumentos.CREAR.path) + sizeof(int);
			copyStringToBuffer(&buffer, operacion->argumentos.CREAR.path);
			copyIntToBuffer(&buffer, operacion->argumentos.CREAR.linea);
			break;
		case BORRAR:
			code = CPU_DAM_BORRAR;
			size = strlen(operacion->argumentos.BORRAR.path);
			copyStringToBuffer(&buffer, operacion->argumentos.BORRAR.path);
			break;
		case ASIGNAR:
			code = CPU_FM9_ASIGNAR;
			size = strlen(operacion->argumentos.ASIGNAR.path) + sizeof(int) + strlen(operacion->argumentos.ASIGNAR.datos);
			copyStringToBuffer(&buffer, operacion->argumentos.ASIGNAR.path);
			copyIntToBuffer(&buffer, operacion->argumentos.ASIGNAR.linea);
			copyStringToBuffer(&buffer, operacion->argumentos.ASIGNAR.datos);
			break;
		case CLOSE:
			code = CPU_FM9_CERRAR_ARCHIVO;
			size = strlen(operacion->argumentos.CLOSE.path);
			copyStringToBuffer(&buffer, operacion->argumentos.CLOSE.path);
			break;
		default:
			return EXIT_FAILURE;
	}
	if (accion == ABRIR && strstr((*dtb)->tablaDirecciones, operacion->argumentos.ABRIR.path) != NULL)
	{
		log_info_mutex(loggerCPU, "El archivo %s ya está abierto por el proceso %d.", operacion->argumentos.ABRIR.path, (*dtb)->idGDT);
		return EXIT_SUCCESS;
	}
	if (modulo == DAM)
	{
		socket = t_socketDAM->socket;
	}
	if (modulo == FM9)
	{
		socket = t_socketFM9->socket;
	}
	if (code < 1 || strlen(buffer) < 1 || size < 1 || socket < 1)
	{
		log_error_mutex(loggerCPU, "Hubo un error al intentar crear el paquete para el envio al DAM.");
		return EXIT_FAILURE;
	}

	// Aca se realiza el envío de la operacion que se está ejecutando actualmente
	if(enviar(socket, code, buffer, size, loggerCPU->logger))
	{
		log_error_mutex(loggerCPU, "Hubo un error en el envío del paquete.");
		return EXIT_FAILURE;
	}

	// Esta validacion es para que sólo se bloquee el GDT cuando la accion implica una llamada al DAM.
	if (modulo == DAM)
	{
		if (eventoSAFA(dtb, accion, CPU_SAFA_BLOQUEAR_DTB))
		{
			log_error_mutex(loggerCPU, "Hubo un error en el envio de bloqueo del G.DT.");
			return EXIT_FAILURE;
		}
		return DTB_DESALOJADO;
	}
	// Esta validacion es para esperar una respuesta del FM9 y verificar que no haya errores ni accesos inválidos.
	if (modulo == FM9)
	{
		t_package package;
		if(recibir(socket, &package, loggerCPU->logger))
		{
			log_error_mutex(loggerCPU, "Hubo un error al recibir la respuesta del FM9.");
			return EXIT_FAILURE;
		}
		if(package.code == FM9_CPU_ACCESO_INVALIDO || package.code == FM9_CPU_ERROR)
		{
			if (eventoSAFA(dtb, accion, CPU_SAFA_ABORTAR_DTB))
			{
				log_error_mutex(loggerCPU, "Hubo un error en el envio de finalización del G.DT.");
				return EXIT_FAILURE;
			}
			return DTB_DESALOJADO;
		}
	}
	free(buffer);
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
	int size = strlen(recurso) + sizeof(int);
	char * buffer;
	copyStringToBuffer(&buffer, recurso);
	copyIntToBuffer(&buffer,idGDT);
	size = strlen(buffer);
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
	int quantum = copyIntFromBuffer(&paquete.data);
	if (quantum > 0)
	{
		log_info_mutex(loggerCPU, "El valor del quantum es %d", quantum);
		return EXIT_SUCCESS;
	}
    pthread_mutex_unlock(&mutexQuantum);
	return EXIT_FAILURE;
}

t_dtb * transformarPaqueteADTB(t_package paquete)
{
	// Se realiza lo que sería una deserializacion de la info dentro de paquete->data
	t_dtb * dtb = malloc(sizeof(t_dtb));
	char *buffer = paquete.data;
	dtb->idGDT = copyIntFromBuffer(&buffer);
	dtb->dirEscriptorio = copyStringFromBuffer(&buffer);
	dtb->programCounter = copyIntFromBuffer(&buffer);
	dtb->flagInicializado = copyIntFromBuffer(&buffer);
	dtb->tablaDirecciones = copyStringFromBuffer(&buffer);
	dtb->cantidadLineas = copyIntFromBuffer(&buffer);
	return dtb;
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
//		if (error != ERROR_SOCKET_DAM)
//		{
	close(t_socketDAM->socket);
	free(t_socketDAM);
//			if (error != ERROR_SOCKET_SAFA)
//			{
	close(t_socketSAFA->socket);
	free(t_socketSAFA);
//			}
//		}
	close(t_socketFM9->socket);
	free(t_socketFM9);
	pthread_mutex_destroy(mutexQuantum);
	log_destroy_mutex(loggerCPU);
	freeConfig(config, CPU);
	exit(error);
}

void initMutexs(){
	pthread_mutex_init(&mutexQuantum, NULL);
//	pthread_mutex_init(&mutexDesalojo, NULL);
}

//void initSems() {
//    sem_init(&sem_nuevoDummy, 0, 0);
//}
