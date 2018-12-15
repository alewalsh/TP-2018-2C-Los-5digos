#ifndef FileSystem_H_
#define FileSystem_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <grantp/configuracion.h>
#include <grantp/compression.h>
#include <grantp/socket.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/stat.h>
#include <sys/types.h>

//libreria para consola
#include <readline/readline.h>
#include <readline/history.h>

//libreria para comando ls
#include <dirent.h>
#include <openssl/md5.h> // Para calcular el MD5

pthread_t threadDAM;
pthread_t threadConsola;

pthread_attr_t tattr;

t_log_mutex* loggerMDJ;
t_log_mutex* loggerAtencionDAM;
configMDJ* configuracion;

int socketDAM;
t_socket* socketEscucha;
int trasnfer_size;

//componentes FIFA
t_bitarray *bitarray;
pthread_mutex_t semaforoBitarray;

enum codigosError
{
	ERROR_SOCKET_DAM,
	ERROR_PATH_CONFIG,
	ERROR_CONFIG,
	FIN_EXITOSO
};

typedef struct metadataArchivo{
	int tamanio;
	char * bloques;
}metadataArchivo;

int fileSystemAvtivo;

void inicializarMDJ(char *);
void inicializarConexion();
void esperarInstruccionDAM();
void exit_gracefully(int);
void crearFifa();
void crearRutaDirectorio(char *);
void actualizarBitmapHDD();
int cuantosBitsLibres();

//FUNCIONES CONSOLA

void inicializarCosnola();
void eleESE(char *);
int lenUltimaCarpeta(char *);

//FUNCIONES DAM
int shouldExit;
pthread_mutex_t mutexExit;

void responderDAM();
//void responderDAM(t_package);
int validarArchivo(char *);
int escribirStringEnArchivo(char *, char *);
void escribirMetadata(char*, struct metadataArchivo *);
metadataArchivo * leerMetadata(char *path);
char* obtenerDatos(char*,int ,int);
void borrarArchivo(char *);
void enviarStringDAMporTRansferSize(char *);
char * rebirStringDAMporTRansferSize(int);
void crearPathArchivo(char *);


void consoleExit();

int getExit();
void setExit();

void manejarDAM(t_package pkg);

#endif /* FileSystem_H_ */
