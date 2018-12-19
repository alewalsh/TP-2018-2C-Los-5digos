/*
 * commons.h
 *
 *  Created on: 19 nov. 2018
 *      Author: utnso
 */

#ifndef SRC_COMMONS_H_
#define SRC_COMMONS_H_

#include <grantp/structCommons.h>
#include <grantp/configuracion.h>
#include <grantp/split.h>
#include <commons/bitarray.h>

extern int cantLineas;
extern int cantPaginas;
extern int pidBuscado;
extern int lineasXPagina;
extern int nroSegmentoBuscado;
extern char * pathBuscado;
extern pthread_mutex_t mutexSegmentoBuscado;

extern configFM9 * config;
extern t_bitarray * estadoLineas;
extern t_bitarray * estadoMarcos;
extern t_dictionary * tablaProcesos;
extern char * storage;
extern t_log_mutex * logger;
extern pthread_mutex_t mutexMaster;
extern pthread_mutex_t mutexReadset;
extern pthread_mutex_t mutexPIDBuscado;
extern pthread_mutex_t mutexMaxfd;
extern pthread_mutex_t mutexExit;
extern pthread_mutex_t mutexPathBuscado;
extern pthread_mutex_t mutexSolicitudes;

extern t_list * tablaPaginasInvertida;

void mostrarEstadoStorage();
char * prepararLineaMemoria(char * buffer);
char * intToString(int numero);
int direccion(int base, int desplazamiento);
void inicializarBitmap(t_bitarray **bitArray);
void liberarLineas(int base, int limite);
int tengoMemoriaDisponible(int cantACargar);
int posicionesLibres(t_bitarray ** bitArray);
void guardarLinea(int posicionMemoria, char * linea);
bool filtrarPorPid(t_pagina * pagina);
bool filtrarPorPidYPath(t_pagina * pagina);
void realizarFlush(char * linea, int nroLinea, int transferSize, int socket);
void enviarLineaComoPaquetes(char * lineaAEnviar, int tamanioLinea, int tamanioPaquete, int cantidadPaquetes, int nroLinea, int socket);
char * obtenerLinea(int posicionMemoria);
void actualizarTPI(t_pagina * pagina);
void ocuparMarco(int pagina);

void exit_gracefully(int error);
void liberarRecursos();

bool existeProceso(int pid);
void crearProceso(int pid);

int reservarPaginasNecesarias(int paginasAReservar, int pid, char * path, int lineasAOcupar, int nroSegmento);

void logPosicionesLibres(t_bitarray * bitarray, int modo);
int obtenerLineasProceso(int pid, char * path);
void liberarMarco(int nroMarco);
bool hayXMarcosLibres(int cantidad);

t_segmento * reservarSegmento(int lineasEsperadas, t_dictionary * tablaSegmentos, char * archivo, int paginasAReservar);
void actualizarPosicionesLibres(int finalBitArray, int lineasEsperadas, t_bitarray ** bitArray);

void enviarInstruccion(int posicionMemoria, int socketSolicitud);
void actualizarTablaDeSegmentos(int pid, t_segmento * segmento);
bool filtrarPorSegmento(t_pagina * pagina);

int calcularCantidadPaquetes(int sizeOfFile, int tamPaqueteRecibir);
#endif /* SRC_COMMONS_H_ */
