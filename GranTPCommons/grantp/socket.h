/*
 * socket.h
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#include <commons/log.h>
#include <commons/string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "mutex_log.h"

#define packageHeaderSize sizeof(uint32_t) + sizeof(uint16_t);

#define BACKLOG 40;
/*
 * Codigos de operacion para mandar en el header de los mensajes.
 */
enum codigoID {

	//HANDSHAKES
	SAFA_HSK = 1001,
	CPU_HSK = 1002,
	FM9_HSK = 1003,
	DAM_HSK = 1004,
	MDJ_HSK = 1005,

	//SAFA-CPU
	SAFA_CPU_CONNECT,// = 1005,
	SAFA_CPU_OK,// = 1006,
	SAFA_CPU_FAIL,// = 1007,
//	SAFA_CPU_GET_FAIL,// = 1008,
//	SAFA_CPU_SET_FAIL,// = 1009,
//	SAFA_CPU_STORE_FAIL,// = 1010,
//	SAFA_CPU_GET_OK,// = 1011,
//	SAFA_CPU_SET_OK,// = 1012,
//	SAFA_CPU_STORE_OK,// = 1013,
//	SAFA_CPU_COMPACT_OK,// = 1014,
//	SAFA_CPU_BEGIN_COMPACT,// = 1015,
	SAFA_CPU_DISCONNECT,
	SAFA_CPU_NUEVO_DUMMY,
	SAFA_CPU_EJECUTAR,
	SAFA_CPU_QUANTUM,

    //DAM-FM9
    DAM_FM9_CONNECT,
    DAM_FM9_OK,
    DAM_FM9_FAIL,
    DAM_FM9_DISCONNECT,
	DAM_FM9_GUARDARLINEA,
	DAM_FM9_CARGAR_ESCRIPTORIO,
	DAM_FM9_ENVIO_PKG,
	DAM_FM9_RETORNARlINEA,

	//DAM-MDJ
	DAM_MDJ_CONNECT,
	DAM_MDJ_OK,
	DAM_MDJ_FAIL,
	DAM_MDJ_DISCONNECT,
	DAM_MDJ_CARGAR_ESCRIPTORIO,
	DAM_MDJ_GUARDAR_DATOS,
	DAM_MDJ_CREAR_ARCHIVO,
	DAM_MDJ_BORRAR_ARCHIVO,

	//DAM-SAFA
	DAM_SAFA_CONNECT,
	DAM_SAFA_OK,
	DAM_SAFA_FAIL,
	DAM_SAFA_DISCONNECT,
	DAM_SAFA_CONFIRMACION_PID_CARGADO,
	DAM_SAFA_CONFIRMACION_DATOS_GUARDADOS,
	DAM_SAFA_CONFIRMACION_CREAR_ARCHIVO,
	DAM_SAFA_CONFIRMACION_BORRAR_ARCHIVO,

	//CPU-FM9
	CPU_FM9_CONNECT,
	CPU_FM9_OK,
	CPU_FM9_FAIL,
	CPU_FM9_DISCONNECT,
	CPU_FM9_CERRAR_ARCHIVO,
	CPU_FM9_ASIGNAR,
	CPU_FM9_CARGAR_ESCRIPTORIO,

	//CPU-DAM
	CPU_DAM_CONNECT,
	CPU_DAM_OK,
	CPU_DAM_FAIL,
	CPU_DAM_DISCONNECT,
	CPU_DAM_BUSQUEDA_ESCRIPTORIO,
	CPU_DAM_ABRIR_ARCHIVO,
	CPU_DAM_FLUSH,
	CPU_DAM_CREAR,
	CPU_DAM_BORRAR,

	//CPU-SAFA
	CPU_SAFA_BLOQUEAR_DUMMMY,
	CPU_SAFA_WAIT_RECURSO,
	CPU_SAFA_SIGNAL_RECURSO,
	CPU_SAFA_BLOQUEAR_DTB,
	CPU_SAFA_ABORTAR_DTB,

	//FM9-CPU
	FM9_CPU_ACCESO_INVALIDO,
	FM9_CPU_ERROR,

	//FM9-DAM
	FM9_DAM_MEMORIA_INSUFICIENTE,

	//FM9
	GUARDAR_LINEA,
	CARGAR_ESCRIPTORIO,
	RETORNAR_LINEA,
//    //COORDINADOR-SAFA
//    COORD_SAFA_WELCOME,// = 1016,
//    COORD_SAFA_OK,// = 1017,
//    COORD_SAFA_FAIL,// = 1018,
//    COORD_SAFA_GET,// = 1019,
//    COORD_SAFA_STORE,// = 1020,
//    COORD_SAFA_SET,// = 1021,
//    COORD_SAFA_COMPACT,// = 1022,
//
//    //ESI-PLANIFICADOR
//    ESI_PLAN_CONNECT,// = 1023,
//    ESI_PLAN_OK,// = 1024,
//    ESI_PLAN_FAIL,// = 1025,
//    ESI_PLAN_FINISH,// = 1026,
//    ESI_PLAN_EOF,// = 1027,
//
//    //PLANIFICADOR-ESI
//    PLAN_ESI_WELCOME,// = 1028,
//    PLAN_ESI_PLAY,// = 1029,
//    PLAN_ESI_DEAD,// = 1030,
//
//    //ESI-COORDINADOR
//    ESI_COORD_CONNECT,// = 1031,
//    ESI_COORD_SENDPKG,// = 1032,
//
//    //CORDINADOR-ESI
//    COORD_ESI_WELCOME,// = 1033,
//    COORD_ESI_OK,// = 1034,
//    COORD_ESI_FAIL,// = 1035,
//
//    //COORDINADOR-PLANIFICADOR
//    COORD_PLAN_WELCOME,// = 1036,
//    COORD_PLAN_BLOCK,// = 1037,
//    COORD_PLAN_STORE,// = 1038,
//    CORRD_PLAN_STATUS_FOUND,// = 1039,
//    CORRD_PLAN_STATUS_NOT_FOUND,// = 1040,
//
//
//    //PLANIFICADOR-COORDINADOR
//    PLAN_COORD_CONNECT,// = 1041,
//    PLAN_COORD_OK,// = 1042,
//    PLAN_COORD_BLOCKED,// = 1043,
//    PLAN_CORRD_STORE_FAIL,// = 1044,
//    PLAN_CORRD_STORE_OK,// = 1045,
//    PLAN_CORRD_STATUS,// = 1046,
//
    GENERIC_ERROR,

    SOCKET_DISCONECT,// = 1048
    PING,//=1049
    PING_OK//1050

};

typedef struct {
    pthread_mutex_t mutex;
    int socket;
} t_socket;

/*
 * Estructura de los mensajes
 * code: Codigo del mensaje, esto indica que es lo que operacion quiero hacer.
 * size: tamaño del contenido a mandar en data
 * data: lo que quiero enviar en el mensaje, el payload.
 */
typedef struct {
    uint16_t code;
    uint32_t size;
    char *data;
} t_package;

/**
 *
 */
t_socket *inicializarTSocket(int socket, t_log *logger);

/**
 * @NAME: escuchar
 * @DESC: crea un socket y se encarga de hacer un liste sobre el mismo.
 * @PARAMS: {int} 	puerto Numero de puerto
 * 			{int *}	socket Puntero en el que se almacenara el socket, necesita tener la memoria asignada.
 */
int escuchar(int puerto, int *socket, t_log *logger);

/**
 * @NAME: escuchar
 * @DESC: Acepta una conexion sobre un socket que esta haciendo un listen. por lo que hay que llamar a escuchar antes que a esta funcion.
 * @PARAMS: {int} 	socket Socket que esta escuchando
 * 			{int *}	newSocket Nuevo sokcet generado al hacer el accept.
 */
int aceptar(int socket, int *newSocket, t_log *logger);

/**
 * @NAME: cargarSocket
 * @DESC: Crea un socket que se puede usar tanto para listen, si no se le pasa la ip, como para connect,
 * 		  si le paso la ip a la que me quiero conectar. La memoria de la variables puntero debe estar previamente asignada.
 * @PARAMS: {int} 	iPuerto puesto que se le asigna al socket.
 * 			{int*}	ip ip a la que me quiero conectar, o si es null el socket se puede usar para hacer listen.
 * 			{int*}	pSocket Socket creado dentro de la funcion.
 * 			{t_log*} Logger.
 */
int cargarSocket(int iPuerto, const char *ip, int *pSocket, t_log *logger);

/**
 * @NAME: enviarHandshake
 * @DESC: Envia un codigo de handshake,luego recibe la respuesta y chequea que sea la esperada.
 * 		  NOTA IMPORTANTE: no tengo que llamar a recibirHandshake,
 * @PARAMS: {int} 	socket por el que realizo la comunicación.
 * 			{uint16_t}	codigoMio Codigo del programa que llama a la función.
 * 			{uint16_t}	codigoOtro Codigo del programa  al que me quiero conectar.
 * 			{t_log*} Logger.
 */
int enviarHandshake(int socket, uint16_t codigoMio, uint16_t codigoOtro,
                    t_log *logger);

/**
 * @NAME: recibirHandshake
 * @DESC: Recibe un handshake y manda correspondiente la respuesta.
 * 		  NOTA IMPORTANTE: no tengo que llamar a enviarHandshake antes de usar esta funcion.
 * @PARAMS: {int} 	socket por el que realizo la comunicación.
 * 			{uint16_t}	codigoMio Codigo del programa que llama a la función.
 * 			{uint16_t}	codigoOtro Se guarda el codigo de la persona que mandó el handshake.
 * 			{t_log*} Logger.
 */
int recibirHandshake(int socket, uint16_t codigoMio, uint16_t *codigoOtro,
                     t_log *logger);

/**
 * @NAME: packageSize
 * @DESC: Devuelve el tamaño del tipo t_package junto con el tamaño del contenido de data.
 * @PARAMS: {uint32_t} size Tiene que ser el tamaño del contenido de data.
 */
uint32_t packageSize(uint32_t size);

/**
 * @NAME: compress
 * @DESC: Se encarga de crear un bloque de memoria los diferentes datos de un mensaje en un solo bloque de memoria.
 * 		  NOTA: Funcion de uso interna.
 * @PARAMS: {int} 	code Código a enviar en el paquete.
 * 			{char*}	data Datos a enviar.
 * 			{uint32_t} size Tamaño del contenido de 'data'.
 * 			{t_log*} Logger.
 */
char *compress(int code, char *data, uint32_t size, t_log *logger);

/**
 * @NAME: enviar
 * @DESC: Se encar de enviar un mensaje, con el codigo y los datos con las que es llamado.
 * 		  NOTA: el proceso al que le envio los datos tiene que estar esperando con un recibir.
 * @PARAMS: {int} 	socket Código a enviar en el paquete.
 * 			{uint16_t}	code Codigo de operación.
 * 			{char*} data datos que se quieren enviar. El payload.
 * 			{uint32_t} size Tamaño de lo que voy enviar, tamaño del payload.
 * 			{t_log*} Logger.
 */
int enviar(int socket, uint16_t code, char *data, uint32_t size, t_log *logger);

/**
 * @NAME: enviar_m
 * @DESC: Se encar de enviar un mensaje, con el codigo y los datos con las que es llamado.
 * 		  NOTA: el proceso al que le envio los datos tiene que estar esperando con un recibir.
 * @PARAMS: {int} 	socket Código a enviar en el paquete.
 * 			{uint16_t}	code Codigo de operación.
 * 			{char*} data datos que se quieren enviar. El payload.
 * 			{uint32_t} size Tamaño de lo que voy enviar, tamaño del payload.
 * 			{t_log_mutex*} Logger.
 */
int enviar_m(int socket, uint16_t code, char *data, uint32_t size,
             t_log_mutex *logger);

/**
 * @NAME: recibir
 * @DESC: Recibe un mensaje y lo guarda en el segundo parametro, el t_package*.
 * 		  NOTA: Se queda a la espera de que el otro proceso realize un enviar.
 * @PARAMS: {int} 	socket Socket por el que se hace la comunicación.
 * 			{t_package*} mensaje donde se guarda el mensaje recibido.
 * 			{t_log*} Logger.
 */
int recibir(int socket, t_package *mensaje, t_log *logger);

/**
 * @NAME: recibir_m
 * @DESC: Recibe un mensaje y lo guarda en el segundo parametro, el t_package*.
 * 		  NOTA: Se queda a la espera de que el otro proceso realize un enviar.
 * @PARAMS: {int} 	socket Socket por el que se hace la comunicación.
 * 			{t_package*} mensaje donde se guarda el mensaje recibido.
 * 			{t_log_mutex*} Logger.
 */
int recibir_m(int socket, t_package *mensaje, t_log_mutex *logger);

/**
 * @NAME: recvPkg
 * @DESC: Se encar de hacer el send de forma recursiva hasta que se lea el mensaje completo.
 * 		  NOTA: Funcion de uso interna, no usar.
 * @PARAMS: {int} socket Socket por el que se hace la comunicación.
 * 			{char**} buffer Puntero al char* donde se va a guardar el mensaje.
 * 			{uint32_t} size Tamaño de lo que voy recibir, tamaño del payload.
 * 			{t_log*} Logger.
 */
int recvPkg(int socket, char **buffer, uint32_t size, t_log *logger);

/**
 * @NAME: highestFD
 * @DESC: Compara 2 file descriptors y retorna el valor del mayor más 1 para ser almacenado en el select.
 * @PARAMS: {int} fd File descriptor más grande guardado en el select.
 * 			{int} nfd Nuevo file descriptor.
 */
int highestFD(int, int);

/**
 * @NAME: freeData
 * @DESC: Libera el campo data de un t_package si es que tiene asignado algo
 * @PARAMS: {t_package} paquete del que deseo liberar el campo data.
 */
void freeData(t_package pkg);

/**
 * @NAME: freePackage
 * @DESC: Libera un t_package* y su campo data si es que estos estan inicializados. Si el t_package no fue creado con
 * malloc la funcion va a fallar.
 * @PARAMS: {t_package *} Puntero a un t_package creado dinamicamente que deseo liberar..
 */
void freePackage(t_package *pkg);


/**
 * @NAME: getIP
 * @DESC: Retorna la ip de la maquina.
 * @PARAMS:
 */
char *getIP();

/**
 * @NAME: parseAdress
 * @DESC: Pasea un string del tipo "xxx.xxx.xxx.xxx:puerto" en 2 variables. Un string para la ip y un int para el puerto.
 * @PARAMS: {char *} String que se desea parsear.
 *          {char **} Puntero a char* donde se asignara un nuevo string dinamicamente creado. (El string NO tiene que estar Inicializado).
 *          {int *} puntero a entero donde se guardara el puerto. (Debe de estar inicializado.)
 */
void parseAdress(char *adress, char **ip, int *puerto);

/**
 * @NAME: acceptConnection
 * @DESC: Funcion con la que se aceptanconexiones entrantes y se recibe el handshake.
 * @PARAMS: {int} Socket que esta escuchando conexiones entrantes.
 *          {int *} Puntero a entero donde se guardara el descriptor del nuevo socket que se genera despues del accept. (DEBE ESTAR INICIALIZADO)
 *          {uint16_t } Codigo de handshake a enviar
 *          {uint16_t *} Variable donde se guardara el Codigo de handshake recibido (DEBE ESTAR INICIALIZADO)
 *          {t_log *} logger.
 */
int acceptConnection(int socketListen, int *pNewSocket, uint16_t handshake,
                     uint16_t *handshakeotro, t_log *logger);

/**
 * @NAME: codigoIDToString
 * @DESC: Imprime el nombre del codigo (ENUM) que se le pasa.
 * @PARAMS: {uint16_t} ENUM que se usa para la comunicacion entre procesos.

 */
char* codigoIDToString(uint16_t code);



#endif /* COMANDOS_H_ */
