/*
 * segmentacionSimple.h
 *
 *  Created on: 19 nov. 2018
 *      Author: utnso
 */

#ifndef SRC_SEGMENTACIONSIMPLE_H_
#define SRC_SEGMENTACIONSIMPLE_H_

#include <grantp/socket.h>
#include <grantp/structCommons.h>
#include <grantp/configuracion.h>
#include "commons.h"

extern int contLineasUsadas;

extern t_dictionary * tablaProcesos;
extern t_log_mutex * logger;
extern configFM9 * config;

int flushSegmentacion(int socketSolicitud, t_datosFlush * data, int accion);
void imprimirInfoAdministrativaSegmentacion(int pid);
int cerrarArchivoSegmentacion(t_package pkg, t_infoCerrarArchivo* datosPaquete, int socketSolicitud);
int ejecutarCargarEsquemaSegmentacion(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud);
void actualizarTablaDeSegmentos(int pid, t_segmento * segmento);
int ejecutarGuardarEsquemaSegmentacion(t_package pkg, t_infoGuardadoLinea* datosPaquete, int socket);

#endif /* SRC_SEGMENTACIONSIMPLE_H_ */
