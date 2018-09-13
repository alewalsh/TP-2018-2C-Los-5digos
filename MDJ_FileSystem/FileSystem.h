#ifndef FileSystem_H_
#define FileSystem_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <configuracion.h>
#include <socket.h>
#include <pthread.h>

t_log* logger;
configMDJ* configuracion;

enum codigosError
{
	ERROR_SOCKET_DAM,
	ERROR_PATH_CONFIG,
	ERROR_CONFIG,
	FIN_EXITOSO
};

void inicializarCPU(char *);
void inicializarConexion();
void esperarInstruccionDAM(int);
void exit_gracefully(int);

#endif /* FileSystem_H_ */
