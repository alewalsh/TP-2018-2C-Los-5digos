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
	//free(self->path);
	free(self);
}

void liberarOperacion(t_cpu_operacion * self)
{
	switch(self->keyword)
	{
		case ABRIR:
			free(self->argumentos.ABRIR.path);
			break;
		case ASIGNAR:
			free(self->argumentos.ASIGNAR.datos);
			free(self->argumentos.ASIGNAR.path);
			break;
		case WAIT:
			free(self->argumentos.WAIT.recurso);
			break;
		case SIGNAL:
			free(self->argumentos.SIGNAL.recurso);
			break;
		case FLUSH:
			free(self->argumentos.FLUSH.path);
			break;
		case CLOSE:
			free(self->argumentos.CLOSE.path);
			break;
		case CREAR:
			free(self->argumentos.CREAR.path);
			break;
		case BORRAR:
			free(self->argumentos.BORRAR.path);
			break;
		default:
			break;
	}
	free(self);
}

t_infoGuardadoLinea * guardarDatosPaqueteGuardadoLinea(t_package pkg){

	char * buffer = pkg.data;
	t_infoGuardadoLinea * datosPaquete = malloc(pkg.size);

	datosPaquete->pid = copyIntFromBuffer(&buffer);
	datosPaquete->path = copyStringFromBuffer(&buffer);
	datosPaquete->linea = copyIntFromBuffer(&buffer);
	datosPaquete->datos = copyStringFromBuffer(&buffer);

//	free(buffer);
	return datosPaquete;
}

t_infoCargaEscriptorio * guardarDatosPaqueteCargaEscriptorio(t_package pkg){

	char * buffer = pkg.data;
	t_infoCargaEscriptorio * datosPaquete = malloc(pkg.size);

	datosPaquete->pid = copyIntFromBuffer(&buffer);
	datosPaquete->path = copyStringFromBuffer(&buffer);
	datosPaquete->cantidadLineasARecibir = copyIntFromBuffer(&buffer);

	//free(buffer);
	return datosPaquete;
}

t_infoCerrarArchivo * guardarDatosPaqueteCierreArchivo(t_package pkg){

	char * buffer = pkg.data;
	t_infoCerrarArchivo * datosPaquete = malloc(pkg.size);

	datosPaquete->pid = copyIntFromBuffer(&buffer);
	datosPaquete->path = copyStringFromBuffer(&buffer);

//	free(buffer);
	return datosPaquete;
}

t_datosFlush * guardarDatosPaqueteFlush(t_package pkg)
{
	char * buffer = pkg.data;
	t_datosFlush * infoFlush = malloc(pkg.size);

	infoFlush->pid = copyIntFromBuffer(&buffer);
	infoFlush->path = copyStringFromBuffer(&buffer);
	infoFlush->transferSize = copyIntFromBuffer(&buffer);

//	free(buffer);
	return infoFlush;
}

t_infoDevolverInstruccion * guardarDatosPaqueteInstruccion(t_package pkg)
{
	char * buffer = pkg.data;
	t_infoDevolverInstruccion * infoInstruccion = malloc(pkg.size);

	infoInstruccion->path = copyStringFromBuffer(&buffer);
	infoInstruccion->pid = copyIntFromBuffer(&buffer);
	infoInstruccion->posicion = copyIntFromBuffer(&buffer);

	//TODO: Revisar este free que me estaba rompiendo.
//	free(buffer);
	return infoInstruccion;
}

t_dtb * transformarPaqueteADTB(t_package paquete)
{
	// Se realiza lo que sería una deserializacion de la info dentro de paquete->data
	t_dtb * dtb = malloc(paquete.size);
	char *buffer = paquete.data;
	bool tieneTablaDirecciones;
	dtb->idGDT = copyIntFromBuffer(&buffer);
	dtb->dirEscriptorio = copyStringFromBuffer(&buffer);
	dtb->programCounter = copyIntFromBuffer(&buffer);
	dtb->flagInicializado = copyIntFromBuffer(&buffer);
	tieneTablaDirecciones = copyIntFromBuffer(&buffer);
	dtb->tablaDirecciones = list_create();
	if (tieneTablaDirecciones)
	{
		int cantidadDirecciones = copyIntFromBuffer(&buffer);
		int i = 0;
		while(i < cantidadDirecciones)
		{
			char * path = copyStringFromBuffer(&buffer);
			list_add(dtb->tablaDirecciones,path);
			i++;
		}
	}
	dtb->cantidadLineas = copyIntFromBuffer(&buffer);
	dtb->realizOpDummy = copyIntFromBuffer(&buffer);
	dtb->quantumRestante = copyIntFromBuffer(&buffer);
	dtb->cantIO = copyIntFromBuffer(&buffer);
	dtb->esDummy = copyIntFromBuffer(&buffer);

	return dtb;
}
t_package transformarDTBAPaquete(t_dtb * dtb)
{
	// Se realiza lo que sería una deserializacion de la info dentro de paquete->data
	t_package paquete;
    int stringsLength = strlen(dtb->dirEscriptorio) + 1;
    int tamanioTablaDirecciones = 0;
	bool tieneTablaDirecciones = false;
	int size = 0;
	if (dtb->tablaDirecciones != NULL && !list_is_empty(dtb->tablaDirecciones))
	{
		int i = 0;
		while(i < dtb->tablaDirecciones->elements_count)
		{
			char * path = list_get(dtb->tablaDirecciones, i);
			tamanioTablaDirecciones += (string_length(path) + 1)* sizeof(char) + sizeof(int);
			i++;
		}
		tamanioTablaDirecciones += sizeof(int);
		tieneTablaDirecciones = true;
	}
	size = sizeof(int)*10 + (stringsLength) * sizeof(char) + tamanioTablaDirecciones;
	paquete.size = size;
	char *buffer = (char *) malloc(paquete.size);
	char * p = buffer;
	copyIntToBuffer(&p,dtb->idGDT);
	copyStringToBuffer(&p,dtb->dirEscriptorio);
	copyIntToBuffer(&p,dtb->programCounter);
	copyIntToBuffer(&p,dtb->flagInicializado);
	copyIntToBuffer(&p,tieneTablaDirecciones);
	if (tieneTablaDirecciones)
	{
		copyIntToBuffer(&p,dtb->tablaDirecciones->elements_count);
		int j = 0;
		while(j < dtb->tablaDirecciones->elements_count)
		{
			char * path = list_get(dtb->tablaDirecciones, j);
			copyStringToBuffer(&p,path);
			j++;
		}
	}
	copyIntToBuffer(&p,dtb->cantidadLineas);
	copyIntToBuffer(&p,dtb->realizOpDummy);
	copyIntToBuffer(&p, dtb->quantumRestante);
	copyIntToBuffer(&p, dtb->cantIO);
	copyIntToBuffer(&p, dtb->esDummy);
	paquete.data = buffer;
	return paquete;
}
