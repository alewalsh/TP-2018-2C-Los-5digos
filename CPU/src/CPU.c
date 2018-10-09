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

int main(int argc, char ** argv) {
	inicializarCPU(argv[1]);
	//Recibir DTB y verificar valor de flag de inicializacion
	recibirDTB();
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
	//	t_dtb * DTB = transformarPaqueteADTB(paquete);
	// Si es 0, levanto un hilo y realizo  la operación Dummy - Iniciar G.DT
	// Solicitarle al DAM la busqueda del Escriptorio en el MDJ
	paquete.code = CPU_DAM_BUSQUEDA_ESCRIPTORIO;
	if(enviar(t_socketDAM->socket,paquete.code,paquete.data, paquete.size, loggerCPU->logger))
		return EXIT_FAILURE;

	// Desalojar al DTB Dummy, avisando a SAFA que lo bloquee
	paquete.code = CPU_SAFA_BLOQUEAR_DUMMMY;
	if(enviar(t_socketSAFA->socket,paquete.code,paquete.data, paquete.size, loggerCPU->logger))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

int comenzarEjecucion(t_package paquete)
{
	// Luego de recibirlo tengo que verificar su flag de inicializacion
	//	t_dtb * DTB = transformarPaqueteADTB(paquete);
	// Si es 1, levanto un hilo y comienzo la ejecución de sentencias
	// if DTB->flagInicializado == 1

	// Realizar las ejecuciones correspondientes definidas por el quantum de SAFA

	// Por cada unidad de tiempo de quantum, se ejecutara una linea del Escriptorio indicado en el DTB

	// Comunicarse con el FM9 en caso de ser necesario.

	// Si el FM9 indica un acceso invalido o error, se aborta el DTB informando a SAFA para que
	// lo pase a la cola de Exit.
	return EXIT_SUCCESS;
}

int setQuantum(t_package paquete)
{
	int quantum = copyIntFromBuffer(&paquete.data);
	if (quantum > 0)
	{
		log_info_mutex(loggerCPU, "El valor del quantum es %d", quantum);
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

t_dtb * transformarPaqueteADTB(t_package paquete)
{
	t_dtb * dtb = malloc(sizeof(t_dtb));
	//int tamanioTotal = paquete.size;
	// Aca habria que realizar lo que sería una deserializacion de la info dentro de paquete->data
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
	socketFM9 = malloc(sizeof(int));
	t_socketFM9 = conectarseAProceso(config->puertoFM9,config->ipFM9,socketFM9,FM9_HSK);
	socketDAM = malloc(sizeof(int));
	t_socketDAM = conectarseAProceso(config->puertoDAM,config->ipDAM,socketDAM,DAM_HSK);
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
			free(t_socketDAM);
			close(*socketDAM);
//			if (error != ERROR_SOCKET_SAFA)
//			{
				free(t_socketSAFA);
				close(*socketSAFA);
//			}
//		}
	log_destroy_mutex(loggerCPU);
	freeConfig(config, CPU);
	exit(error);
}

//void initSems() {
//    sem_init(&sem_nuevoDummy, 0, 0);
//}
