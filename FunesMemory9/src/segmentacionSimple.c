/*
 * segmentacionSimple.c
 *
 *  Created on: 19 nov. 2018
 *      Author: Alejandro Walsh
 */

#include "segmentacionSimple.h"

int cerrarArchivoSegmentacion(t_package pkg, t_infoCerrarArchivo* datosPaquete, int socketSolicitud)
{
	char * pidString = intToString(datosPaquete->pid);
	t_gdt * gdt = dictionary_get(tablaProcesos,pidString);
	if (gdt == NULL)
	{
		return FM9_CPU_PROCESO_INEXISTENTE;
	}
	int cantidadSegmentos = dictionary_size(gdt->tablaSegmentos);
	if(cantidadSegmentos > 0)
	{
		for(int i = 0; i < cantidadSegmentos; i++)
		{
			t_segmento * segmento = dictionary_get(gdt->tablaSegmentos, intToString(i));
			if (strcmp(segmento->archivo,datosPaquete->path) == 0)
			{
				liberarLineas(segmento->base,segmento->limite);
				break;
			}
		}
		//ENVIAR MSJ DE EXITO A CPU
		if (enviar(socketSolicitud,FM9_CPU_ARCHIVO_CERRADO,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al CPU que se ha guardado correctamente la línea.");
			exit_gracefully(-1);
		}
		dictionary_clean_and_destroy_elements(gdt->tablaSegmentos,(void *)liberarSegmento);
	}
	return EXIT_SUCCESS;
}

// Lógica de segmentacion pura
int ejecutarCargarEsquemaSegmentacion(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud)
{
	if(tengoMemoriaDisponible(datosPaquete->cantPaquetes) == 1){
		log_error_mutex(logger, "No hay memoria disponible para cargar el Escriptorio.");
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		logPosicionesLibres(estadoLineas,SEG);
		if (enviar(socketSolicitud,FM9_DAM_MEMORIA_INSUFICIENTE,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al DAM de la memoria insuficiente.");
			exit_gracefully(-1);
		}
	}
	if (!existeProceso(datosPaquete->pid))
		crearProceso(datosPaquete->pid);

	char * pidString = intToString(datosPaquete->pid);
	t_gdt * gdt = dictionary_get(tablaProcesos,pidString);
	free(pidString);
	t_segmento * segmento = reservarSegmento(datosPaquete->cantPaquetes, gdt->tablaSegmentos, datosPaquete->path);
	// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ LINEAS HAY LIBRES
	if (segmento == NULL)
	{
		logPosicionesLibres(estadoLineas,SEG);
		return FM9_DAM_MEMORIA_INSUFICIENTE_FRAG_EXTERNA;
	}
	actualizarTablaDeSegmentos(datosPaquete->pid,segmento);
	// ACTUALIZO LA GLOBAL CON LAS LINEAS QUE UTILICE RECIEN
	// ESTO PARA QUE ESTA?????
	contLineasUsadas += datosPaquete->cantPaquetes;

	char * bufferGuardado = malloc(config->tamMaxLinea);
	int i = 0, offset = 0;
	int lineaLeida = 1;
	while(i < segmento->limite)
	{
		t_package paquete;
		if(recibir(socketSolicitud,&paquete,logger->logger))
		{
			log_error_mutex(logger, "Error al recibir el paquete N°: %d",i);
			if (enviar(socketSolicitud,FM9_DAM_ERROR_PAQUETES,pkg.data,pkg.size,logger->logger))
			{
				log_error_mutex(logger, "Error al recibir los paquetes del DAM.");
				exit_gracefully(-1);
			}
		}
		char * bufferLinea = paquete.data;
		int nroLinea = copyIntFromBuffer(&bufferLinea);
		int tamanioPaquete = copyIntFromBuffer(&bufferLinea);
		char * contenidoLinea = copyStringFromBuffer(&bufferLinea);
		if (nroLinea != lineaLeida)
		{
			int tamanioBuffer = strlen(bufferGuardado);
			bufferGuardado[tamanioBuffer] = '\n';
			guardarLinea(direccion(segmento->base,i), bufferGuardado);
			i++;
			free(bufferGuardado);
			bufferGuardado = malloc(config->tamMaxLinea);
			memcpy(&bufferGuardado+offset, &contenidoLinea, tamanioPaquete - 2*sizeof(int));
			offset += tamanioPaquete - 2*sizeof(int);
		}
		else
		{
			memcpy(&bufferGuardado+offset, &contenidoLinea, tamanioPaquete - 2*sizeof(int));
			offset += tamanioPaquete - 2*sizeof(int);
		}
		lineaLeida = nroLinea;
	}
	free(bufferGuardado);

	//ENVIAR MSJ DE EXITO A DAM
	if (enviar(socketSolicitud,FM9_DAM_ESCRIPTORIO_CARGADO,pkg.data,pkg.size,logger->logger))
	{
		log_error_mutex(logger, "Error al avisar al DAM el aviso de que se ha cargado el Escriptorio.");
		exit_gracefully(-1);
	}
	return EXIT_SUCCESS;
}

int flushSegmentacion(int socketSolicitud, t_datosFlush * data, int accion)
{
	t_gdt * gdt = dictionary_get(tablaProcesos,intToString(data->pid));
	if (gdt == NULL)
	{
		return FM9_DAM_ARCHIVO_INEXISTENTE;
	}
	int cantidadSegmentos = dictionary_size(gdt->tablaSegmentos);
	if(cantidadSegmentos > 0)
	{
		if (accion == AccionFLUSH)
		{
			// PRIMERO ENVÍO LA CANTIDAD DE LINEAS DEL ARCHIVO
			int cantidadLineas = obtenerLineasProceso(data->pid);
			char * buffer;
			copyIntToBuffer(&buffer, cantidadLineas);
			if (enviar(socketSolicitud,FM9_DAM_FLUSH,buffer,sizeof(int),logger->logger))
			{
				log_error_mutex(logger, "Error al avisar al CPU que se ha guardado correctamente la línea.");
				exit_gracefully(-1);
			}
		}

		// LUEGO RECORRO CADA SEGMENTO Y VOY ENVIANDO DE A UNA LINEA
		int nroLinea = 1;
		for(int i = 0; i < cantidadSegmentos; i++)
		{
			t_segmento * segmento = dictionary_get(gdt->tablaSegmentos, intToString(i));
			int j = 0;
			if (strcmp(segmento->archivo, data->path) == 0)
			{
				while(j < segmento->limite)
				{
					char * linea = obtenerLinea(direccion(segmento->base, j));
					if (accion == AccionDUMP)
					{
						printf("Linea %d PID %d: %s\n", j, data->pid, linea);
						log_info_mutex(logger, "Linea %d PID %d: %s\n", j, data->pid, linea);
					}
					if (accion == AccionFLUSH)
					{
						realizarFlush(linea, nroLinea, data->transferSize, socketSolicitud);
					}
					j++;
					nroLinea++;
				}
			}
			i++;
		}
	}
	else
	{
		return FM9_CPU_FALLO_SEGMENTO_MEMORIA;
	}
	return EXIT_SUCCESS;
}

t_segmento * reservarSegmento(int lineasEsperadas, t_dictionary * tablaSegmentos, char * archivo)
{
	t_segmento * segmento = malloc(sizeof(t_segmento));
	int lineasLibresContiguas = 0, i = 0, base;
	while(i <= cantLineas)
	{
		if(bitarray_test_bit(estadoLineas,i) == 0)
		{
			base = i;
			lineasLibresContiguas++;
			if (lineasLibresContiguas == lineasEsperadas)
			{
				break;
			}
		}
		else
		{
			lineasLibresContiguas = 0;
		}
		i++;
	}
	actualizarPosicionesLibres(base, lineasEsperadas, estadoLineas);
	if (lineasLibresContiguas == lineasEsperadas)
	{
		segmento->base = base - lineasEsperadas;
		segmento->limite = lineasEsperadas;
		int nroSegmento = 0;
		if (!dictionary_is_empty(tablaSegmentos))
			nroSegmento = dictionary_size(tablaSegmentos);
		segmento->nroSegmento = nroSegmento;
		segmento->archivo = archivo;
		return segmento;
	}
	else
		return NULL;

}

void actualizarTablaDeSegmentos(int pid, t_segmento * segmento)
{
	char *  pidString = intToString(pid);
	t_gdt * gdt = dictionary_remove(tablaProcesos,pidString);
	char * nroSegmentoString = intToString(segmento->nroSegmento);
	dictionary_put(gdt->tablaSegmentos,nroSegmentoString,segmento);
	free(nroSegmentoString);
	dictionary_put(tablaProcesos,pidString,gdt);
}

void actualizarPosicionesLibres(int finalBitArray, int lineasEsperadas, t_bitarray * bitArray)
{
	int posicionInicial = finalBitArray - lineasEsperadas;
	while (posicionInicial <= finalBitArray)
	{
		bitarray_set_bit(bitArray, posicionInicial);
		posicionInicial++;
	}
}

// Lógica de segmentación pura
int ejecutarGuardarEsquemaSegmentacion(t_package pkg, t_infoGuardadoLinea* datosPaquete, int socket)
{
	t_gdt * gdt = dictionary_get(tablaProcesos,intToString(datosPaquete->pid));
	if (gdt == NULL)
	{
		return FM9_CPU_PROCESO_INEXISTENTE;
	}
	int cantidadSegmentos = dictionary_size(gdt->tablaSegmentos);
	bool pudeGuardar = false;
	if(cantidadSegmentos > 0)
	{
		for(int i = 0; i < cantidadSegmentos; i++)
		{
			t_segmento * segmento = dictionary_get(gdt->tablaSegmentos, intToString(i));
			if (strcmp(segmento->archivo,datosPaquete->path) == 0 && segmento->limite >= datosPaquete->linea)
			{
				guardarLinea(direccion(segmento->base,datosPaquete->linea), datosPaquete->datos);
				pudeGuardar = true;
				break;
			}
		}
		if (pudeGuardar)
		{
			//ENVIAR MSJ DE EXITO A CPU
			if (enviar(socket,FM9_CPU_LINEA_GUARDADA,pkg.data,pkg.size,logger->logger))
			{
				log_error_mutex(logger, "Error al avisar al CPU que se ha guardado correctamente la línea.");
				exit_gracefully(-1);
			}
		}
		else
		{
			return FM9_CPU_ACCESO_INVALIDO;
		}
	}
	else
	{
		return FM9_CPU_FALLO_SEGMENTO_MEMORIA;
	}
	return EXIT_SUCCESS;
}

