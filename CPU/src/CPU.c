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
	exit_gracefully(FIN_EXITOSO);
}

void inicializarCPU(char * pathConfig)
{
	logger = log_create("CPU.log", "CPU", true, LOG_LEVEL_INFO);
	if (pathConfig != NULL)
	{
		config = cargarConfiguracion(pathConfig, CPU, logger);
	}
	else
	{
		log_error(logger, "No hay un path correcto a un archivo de configuracion");
		exit_gracefully(ERROR_PATH_CONFIG);
	}
	if (config != NULL)
	{
		inicializarConexiones();
	}
	else
	{
		log_error(logger, "Error en el archivo de configuracion");
		exit_gracefully(ERROR_CONFIG);
	}
}

void inicializarConexiones()
{
	cargarSocket(config->puertoDAM,config->ipDAM,socketDAM,logger);
	if (socketDAM != 0)
	{
		inicializarTSocket(*socketDAM, logger);
		enviarHandshake(t_socketDAM->socket,CPU_HSK,DAM_HSK,logger);
	}
	else
	{
		log_error(logger, "Error al conectarse al DAM.");
		exit_gracefully(ERROR_SOCKET_DAM);
	}

	cargarSocket(config->puertoSAFA,config->ipSAFA,socketSAFA,logger);
	if (socketSAFA != 0)
	{
		inicializarTSocket(*socketSAFA, logger);
		enviarHandshake(t_socketSAFA->socket,CPU_HSK,SAFA_HSK,logger);
	}
	else
	{
		log_error(logger, "Error al conectarse al SAFA.");
		exit_gracefully(ERROR_SOCKET_SAFA);
	}
}

void exit_gracefully(int error)
{
	if (error != ERROR_PATH_CONFIG)
	{
		free(config);
		if (error != ERROR_SOCKET_DAM)
		{
			free(t_socketDAM);
			close(*socketDAM);
			if (error != ERROR_SOCKET_SAFA)
			{
				free(t_socketSAFA);
				close(*socketSAFA);
			}
		}
	}
	log_destroy(logger);
	exit(error);
}
