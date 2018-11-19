/*
 * TPI.h
 *
 *  Created on: 19 nov. 2018
 *      Author: utnso
 */

#ifndef SRC_TPI_H_
#define SRC_TPI_H_

#include "commons.h"

extern int pidBuscado;
extern int contLineasUsadas;
extern t_list * tablaPaginasInvertida;

int ejecutarCargarEsquemaTPI(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud);
int flushTPI(t_package pkg, int socketSolicitud, t_datosFlush * data);
int ejecutarGuardarEsquemaTPI(t_package pkg, t_infoGuardadoLinea* datosPaquete, int socket);
int cerrarArchivoTPI(t_package pkg, t_infoCerrarArchivo* datosPaquete, int socketSolicitud);
void liberarMarco(t_pagina * pagina);

#endif /* SRC_TPI_H_ */
