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
		t_package * paquete = malloc(sizeof(t_package));
		recibir(t_socketSAFA->socket,paquete,loggerCPU->logger);
		// Luego de recibirlo tengo que verificar su flag de inicializacion
		t_dtb * DTB = transformarPaqueteADTB(paquete);
		// Si es 0, levanto un hilo y realizo  la operación Dummy - Iniciar G.DT
		if (DTB->flagInicializado == 0)
		{
			// Solicitarle al DAM la busqueda del Escriptorio en el MDJ

			// Desalojar al DTB Dummy, avisando a SAFA que lo bloquee

		}
		// Si es 1, levanto un hilo y comienzo la ejecución de sentencias
		else if (DTB->flagInicializado == 1)
		{
			// Realizar las ejecuciones correspondientes definidas por el quantum de SAFA

			// Por cada unidad de tiempo de quantum, se ejecutara una linea del Escriptorio indicado en el DTB

			// Comunicarse con el FM9 en caso de ser necesario.

			// Si el FM9 indica un acceso invalido o error, se aborta el DTB informando a SAFA para que
			// lo pase a la cola de Exit.

		}
		else
		{
			exit_gracefully(ERROR_DTB);
		}
		free(paquete);
	}
}

t_dtb * transformarPaqueteADTB(t_package * paquete)
{
	t_dtb * dtb = malloc(sizeof(t_dtb));
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
	conectarseAProceso(config->puertoDAM,config->ipDAM,socketDAM,DAM_HSK, t_socketDAM);
	conectarseAProceso(config->puertoSAFA,config->ipSAFA,socketSAFA,SAFA_HSK, t_socketSAFA);
	conectarseAProceso(config->puertoFM9,config->ipFM9,socketFM9,FM9_HSK, t_socketFM9);
}

void conectarseAProceso(int puerto, char *ip, int * socket, int handshakeProceso, t_socket* TSocket)
{
	cargarSocket(puerto,ip,socket,loggerCPU->logger);
	if (socket != 0)
	{
		inicializarTSocket(*socket, loggerCPU->logger);
		enviarHandshake(TSocket->socket,CPU_HSK,handshakeProceso,loggerCPU->logger);
	}
	else
	{
		log_error_mutex(loggerCPU, "Error al conectarse al %s", enumToProcess(handshakeProceso));
		exit_gracefully(ERROR_SOCKET);
	}
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
//	if (error != ERROR_PATH_CONFIG)
//	{
		free(config);
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
//	}
	log_destroy_mutex(loggerCPU);
	exit(error);
}
