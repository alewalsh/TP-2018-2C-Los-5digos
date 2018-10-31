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

pthread_t threadDAM;
pthread_t threadConsola;

pthread_attr_t tattr;
t_socket* socketEscucha;

t_log* logger;
configMDJ* configuracion;

int * socketDAM;

//componentes FIFA
t_bitarray *bitarray;

enum codigosError
{
	ERROR_SOCKET_DAM,
	ERROR_PATH_CONFIG,
	ERROR_CONFIG,
	FIN_EXITOSO
};

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
void detectaIngresoConsola(char* const mensaje, char[3][40]);
void leerComandos(void);
void menuConsola(void);


//FUNCIONES DAM
int shouldExit;
pthread_mutex_t mutexExit;

void responderDAM();
//void responderDAM(t_package, int);
int validarArchivo(char *);

void consoleExit();

int getExit();
void setExit();

#endif /* FileSystem_H_ */
