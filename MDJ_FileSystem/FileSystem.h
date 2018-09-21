#ifndef FileSystem_H_
#define FileSystem_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <grantp/configuracion.h>
#include <grantp/socket.h>
#include <pthread.h>


pthread_t threadDAM;
pthread_attr_t tattr;
t_socket* socketEscucha;

t_log* logger;
configMDJ* configuracion;

enum codigosError
{
	ERROR_SOCKET_DAM,
	ERROR_PATH_CONFIG,
	ERROR_CONFIG,
	FIN_EXITOSO
};

void inicializarMDJ(char *);
void inicializarConexion();
void esperarInstruccionDAM();
void exit_gracefully(int);
void responderDAM();

#endif /* FileSystem_H_ */
