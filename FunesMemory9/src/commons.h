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

extern configFM9 * config;
extern t_bitarray * estadoLineas;
extern t_bitarray * estadoMarcos;
extern t_dictionary * tablaProcesos;
extern char * storage;
extern t_log_mutex * logger;
extern pthread_mutex_t mutexMaster;
extern pthread_mutex_t mutexReadset;
extern t_list * tablaPaginasInvertida;

char * intToString(int numero);
int direccion(int base, int desplazamiento);
void inicializarBitmap(t_bitarray * bitArray);
void liberarLineas(int base, int limite);
int tengoMemoriaDisponible(int cantACargar);
int posicionesLibres(t_bitarray * bitArray);
void guardarLinea(int posicionMemoria, char * linea);
bool filtrarPorPid(t_pagina * pagina);
void realizarFlush(char * linea, int nroLinea, int transferSize, int socket);
void enviarLineaComoPaquetes(char * lineaAEnviar, int tamanioLinea, int tamanioPaquete, int cantidadPaquetes, int nroLinea, int socket);
char * obtenerLinea(int posicionMemoria);
void actualizarTPI(t_pagina * pagina);
void ocuparMarco(int pagina);

void exit_gracefully(int error);
void liberarRecursos();

bool existeProceso(int pid);
void crearProceso(int pid);

void reservarPaginasNecesarias(int paginasAReservar, int pid, char * path, int lineasAOcupar);

#endif /* SRC_COMMONS_H_ */
