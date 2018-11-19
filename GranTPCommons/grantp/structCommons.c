/*
 * structCommons.c
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#include "structCommons.h"


//void freeEsiInstruction(void *esi) {
//    t_esi_instruction *e = (t_esi_instruction *) esi;
//    free(e->operation);
//    free(e->key);
//    free(e->value);
//    free(e);
//}
//
//
//void freeEsi(void *esi) {
//    t_node_esi *esi1 = (t_node_esi *) esi;
//    free(esi1->id);
//    free(esi1);
//}
//
//
//void freeBlockedKey(void *bk) {
//    t_blocked_keys *bk1 = (t_blocked_keys *) bk;
//    free(bk1->esiId);
//    free(bk1->key);
//    free(bk1);
//}

void liberarSegmento(t_segmento *self)
{
	free(self->archivo);
	free(self);
}

void liberarPagina(t_pagina * self)
{
	free(self->path);
	free(self);
}

t_infoGuardadoLinea * guardarDatosPaqueteGuardadoLinea(t_package pkg){

	char * buffer = pkg.data;
	t_infoGuardadoLinea * datosPaquete = malloc(sizeof(t_infoGuardadoLinea*));

	datosPaquete->pid = copyIntFromBuffer(&buffer);
	datosPaquete->path = copyStringFromBuffer(&buffer);
	datosPaquete->linea = copyIntFromBuffer(&buffer);
	datosPaquete->datos = copyStringFromBuffer(&buffer);

	free(buffer);
	return datosPaquete;
}

t_infoCargaEscriptorio * guardarDatosPaqueteCargaEscriptorio(t_package pkg){

	char * buffer = pkg.data;
	t_infoCargaEscriptorio * datosPaquete = malloc(sizeof(t_infoCargaEscriptorio*));

	datosPaquete->pid = copyIntFromBuffer(&buffer);
	datosPaquete->path = copyStringFromBuffer(&buffer);
	datosPaquete->cantPaquetes = copyIntFromBuffer(&buffer);

	free(buffer);
	return datosPaquete;
}

t_infoCerrarArchivo * guardarDatosPaqueteCierreArchivo(t_package pkg){

	char * buffer = pkg.data;
	t_infoCerrarArchivo * datosPaquete = malloc(sizeof(t_infoCerrarArchivo*));

	datosPaquete->pid = copyIntFromBuffer(&buffer);
	datosPaquete->path = copyStringFromBuffer(&buffer);

	free(buffer);
	return datosPaquete;
}

t_datosFlush * guardarDatosPaqueteFlush(t_package pkg)
{
	char * buffer = pkg.data;
	t_datosFlush * infoFlush = malloc(sizeof(t_datosFlush));

	infoFlush->pid = copyIntFromBuffer(&buffer);
	infoFlush->path = copyStringFromBuffer(&buffer);
	infoFlush->transferSize = copyIntFromBuffer(&buffer);

	free(buffer);
	return infoFlush;
}
