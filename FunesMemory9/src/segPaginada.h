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

int ejecutarCargarEsquemaSegPag(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud);
int reservarSegmentoSegmentacionPaginada(t_gdt * gdt, int pid);
int flushSegmentacionPaginada(t_package pkg, int socketSolicitud, t_datosFlush * data);
int ejecutarGuardarEsquemaSegPag(t_package pkg, t_infoGuardadoLinea* datosPaquete, int socket);
int cerrarArchivoSegPag(t_package pkg, t_infoCerrarArchivo* datosPaquete, int socketSolicitud);

bool filtrarPorNroPagina(t_pagina * pagina);

#endif /* SRC_SEGPAGINADA_H_ */
