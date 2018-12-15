/*
 * segPaginada.h
 *
 *  Created on: 19 nov. 2018
 *      Author: utnso
 */

#ifndef SRC_SEGPAGINADA_H_
#define SRC_SEGPAGINADA_H_

#include "commons.h"

extern int contLineasUsadas;
int paginaBuscada;
extern pthread_mutex_t mutexPaginaBuscada;

int devolverInstruccionSegPag(t_package pkg, t_infoDevolverInstruccion* datosPaquete, int socketSolicitud);
int finGDTSegPag(t_package pkg, int idGDT, int socketSolicitud);
int ejecutarCargarEsquemaSegPag(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud);
//int reservarSegmentoSegmentacionPaginada(t_gdt * gdt, int pid);
int flushSegmentacionPaginada(int socketSolicitud, t_datosFlush * data, int accion);
void imprimirInfoAdministrativaSegPag(int pid);
int ejecutarGuardarEsquemaSegPag(t_package pkg, t_infoGuardadoLinea* datosPaquete, int socket);
int cerrarArchivoSegPag(t_package pkg, t_infoCerrarArchivo* datosPaquete, int socketSolicitud);

bool filtrarPorNroPagina(t_pagina * pagina);
int reservarPaginasParaSegmento(t_segmento * segmento, t_infoCargaEscriptorio* datosPaquete, int paginasAReservar);

int obtenerPosicionRelativaPagina(int lineaActual);
#endif /* SRC_SEGPAGINADA_H_ */
