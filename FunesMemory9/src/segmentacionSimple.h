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

int flushSegmentacion(t_package pkg, int socketSolicitud, t_datosFlush * data);
int cerrarArchivoSegmentacion(t_package pkg, t_infoCerrarArchivo* datosPaquete, int socketSolicitud);
int ejecutarCargarEsquemaSegmentacion(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud);
t_segmento * reservarSegmento(int lineasEsperadas, t_dictionary * tablaSegmentos, char * archivo);
void actualizarTablaDeSegmentos(int pid, t_segmento * segmento);
void actualizarPosicionesLibres(int finalBitArray, int lineasEsperadas, t_bitarray * bitArray);
int ejecutarGuardarEsquemaSegmentacion(t_package pkg, t_infoGuardadoLinea* datosPaquete, int socket);

#endif /* SRC_SEGMENTACIONSIMPLE_H_ */