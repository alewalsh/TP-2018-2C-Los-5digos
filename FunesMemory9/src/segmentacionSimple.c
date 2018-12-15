/*
 * segmentacionSimple.c
 *
 *  Created on: 19 nov. 2018
 *      Author: Alejandro Walsh
 */

#include "segmentacionSimple.h"

int finGDTSegmentacion(t_package pkg, int idGDT, int socketSolicitud)
{
	char * pidString = intToString(idGDT);
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
			liberarLineas(segmento->base,segmento->limite);
		}
		//ENVIAR MSJ DE EXITO A CPU
		if (enviar(socketSolicitud,FM9_CPU_GDT_FINALIZADO,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al CPU que se ha guardado correctamente la línea.");
			exit_gracefully(-1);
		}
		dictionary_clean_and_destroy_elements(gdt->tablaSegmentos,(void *)liberarSegmento);
	}
	return EXIT_SUCCESS;
}

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
				char * nroSegmentoString = intToString(segmento->nroSegmento);
				dictionary_remove(gdt->tablaSegmentos,nroSegmentoString);
				free(nroSegmentoString);
				dictionary_put(tablaProcesos, pidString, gdt);
				break;
			}
		}
		//ENVIAR MSJ DE EXITO A CPU
		if (enviar(socketSolicitud,FM9_CPU_ARCHIVO_CERRADO,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al CPU que se ha guardado correctamente la línea.");
			exit_gracefully(-1);
		}
	}
	free(pidString);
	return EXIT_SUCCESS;
}

int devolverInstruccionSegmentacion(t_package pkg, t_infoDevolverInstruccion* datosPaquete, int socketSolicitud)
{
	int posicionBuscada = datosPaquete->posicion;
	t_gdt * gdt = dictionary_get(tablaProcesos,intToString(datosPaquete->pid));
	if (gdt == NULL)
	{
		return FM9_CPU_PROCESO_INEXISTENTE;
	}
	int cantidadSegmentos = dictionary_size(gdt->tablaSegmentos);
	bool pudeObtener = false;
	if(cantidadSegmentos > 0)
	{
		for(int i = 0; i < cantidadSegmentos; i++)
		{
			t_segmento * segmento = dictionary_get(gdt->tablaSegmentos, intToString(i));
			if (strcmp(segmento->archivo,datosPaquete->path) == 0 && segmento->limite >= posicionBuscada)
			{
				enviarInstruccion(direccion(segmento->base,posicionBuscada), socketSolicitud);
				pudeObtener = true;
				break;
			}
		}
		if (pudeObtener)
		{
			log_trace_mutex(logger, "Ya obtuve la linea pedida por el proceso %d", datosPaquete->pid);
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

// Lógica de segmentacion pura
int ejecutarCargarEsquemaSegmentacion(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud)
{
	if(tengoMemoriaDisponible(datosPaquete->cantidadLineasARecibir) == 1)
	{
		log_error_mutex(logger, "No hay memoria disponible para cargar el Escriptorio.");
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		logPosicionesLibres(estadoLineas,SEG);
		if (enviar(socketSolicitud,FM9_DAM_MEMORIA_INSUFICIENTE,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al DAM de la memoria insuficiente.");
			// exit_gracefully(-1);
		}
	}
	if (!existeProceso(datosPaquete->pid))
		crearProceso(datosPaquete->pid);

	char * pidString = intToString(datosPaquete->pid);
	t_gdt * gdt = dictionary_get(tablaProcesos,pidString);
	free(pidString);
	t_segmento * segmento = reservarSegmento(datosPaquete->cantidadLineasARecibir, gdt->tablaSegmentos, datosPaquete->path, 0);
	// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ LINEAS HAY LIBRES
	if (segmento == NULL)
	{
		logPosicionesLibres(estadoLineas,SEG);
		return FM9_DAM_MEMORIA_INSUFICIENTE_FRAG_EXTERNA;
	}
	actualizarTablaDeSegmentos(datosPaquete->pid,segmento);
	// ACTUALIZO LA GLOBAL CON LAS LINEAS QUE UTILICE RECIEN
	// ESTO PARA QUE ESTA?????
	contLineasUsadas += datosPaquete->cantidadLineasARecibir;

	char * bufferGuardado = malloc(config->tamMaxLinea);
	int i = 0, offset = 0, tamanioPaqueteReal = 0;
	int lineaLeida = 1;
	int limite = segmento->limite;
	while(i < limite)
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
			bufferGuardado[tamanioPaqueteReal] = '\n';
			log_trace_mutex(logger, "Se guardó la linea '%s' en la posicion de memoria: %d°",bufferGuardado, direccion(segmento->base,i));
			guardarLinea(direccion(segmento->base,i), bufferGuardado);
			i++;
			offset = 0;
			tamanioPaqueteReal = 0;
			free(bufferGuardado);
			bufferGuardado = malloc(config->tamMaxLinea);
			memcpy(bufferGuardado+offset, contenidoLinea, tamanioPaquete);
			offset += (tamanioPaquete);
			tamanioPaqueteReal += tamanioPaquete;
		}
		else
		{
			memcpy(bufferGuardado+offset, contenidoLinea, tamanioPaquete);
			offset += (tamanioPaquete);
			tamanioPaqueteReal += tamanioPaquete;
		}
		lineaLeida = nroLinea;
		if (nroLinea == limite)
		{
			bufferGuardado[tamanioPaqueteReal] = '\n';
			log_trace_mutex(logger, "Se guardó la linea '%s' en la posicion de memoria: %d°",bufferGuardado, direccion(segmento->base,i));
			guardarLinea(direccion(segmento->base,i), bufferGuardado);
			i++;
		}
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
			// PRIMERO ENVÍO LA CANTIDAD DE LINEAS DEL ARCHIVO, LE SUMO UNO PORQUE EL LIMITE CONSIDERA EL VALOR 0
			// Y, EN CAMBIO, LOS PAQUETES NO
			int cantidadLineas = obtenerLineasProceso(data->pid, data->path);
			int size = sizeof(int);
			char * buffer = malloc(size);
			char * p = buffer;
			copyIntToBuffer(&p, cantidadLineas);
			if (enviar(socketSolicitud,FM9_DAM_FLUSH,buffer,sizeof(int),logger->logger))
			{
				log_error_mutex(logger, "Error al avisar al CPU que se ha guardado correctamente la línea.");
				free(buffer);
				exit_gracefully(-1);
			}
			free(buffer);
		}
		// LUEGO RECORRO CADA SEGMENTO Y VOY ENVIANDO DE A UNA LINEA
		int nroLinea = 0;
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
						imprimirInfoAdministrativaSegmentacion(data->pid);
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
		}
	}
	else
	{
		return FM9_CPU_FALLO_SEGMENTO_MEMORIA;
	}
	return EXIT_SUCCESS;
}

void imprimirInfoAdministrativaSegmentacion(int pid)
{
	t_gdt * gdt = dictionary_get(tablaProcesos,intToString(pid));
	int cantidadSegmentos = dictionary_size(gdt->tablaSegmentos);
	int i = 0;
	while(i < cantidadSegmentos)
	{
		t_segmento * segmento = dictionary_get(gdt->tablaSegmentos,intToString(i));
		printf("PID %d: Nro Segmento %d - Base %d - Límite %d \n", pid, segmento->nroSegmento, segmento->base, segmento->limite);
		log_info_mutex(logger, "PID %d: Nro Segmento %d - Base %d - Límite %d", pid, segmento->nroSegmento, segmento->base, segmento->limite);
		i++;
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
	int posicionGuardado = datosPaquete->linea - 1;
	if(cantidadSegmentos > 0)
	{
		for(int i = 0; i < cantidadSegmentos; i++)
		{
			t_segmento * segmento = dictionary_get(gdt->tablaSegmentos, intToString(i));
			if (strcmp(segmento->archivo,datosPaquete->path) == 0 && segmento->limite >= posicionGuardado)
			{
				guardarLinea(direccion(segmento->base,posicionGuardado), datosPaquete->datos);
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

