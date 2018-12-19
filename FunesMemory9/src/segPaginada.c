/*
 * segPaginada.c
 *
 *  Created on: 19 nov. 2018
 *      Author: utnso
 */

#include "segPaginada.h"

int devolverInstruccionSegPag(t_package pkg, t_infoDevolverInstruccion* datosPaquete, int socketSolicitud)
{
	t_gdt * gdt = dictionary_get(tablaProcesos,intToString(datosPaquete->pid));
	if (gdt == NULL)
	{
		return FM9_CPU_PROCESO_INEXISTENTE;
	}
	int cantidadSegmentos = dictionary_size(gdt->tablaSegmentos);
	bool pudeGuardar = false;
	int cantidadLineas = obtenerLineasProceso(datosPaquete->pid, datosPaquete->path);
	int lineaBuscada = datosPaquete->posicion;
	// PRIMERO VERIFICO SI TENGO LA CANTIDAD DE LINEAS DISPONIBLES PARA REALIZAR EL GUARDADO
	if (cantidadLineas < lineaBuscada)
	{
		return FM9_CPU_ACCESO_INVALIDO;
	}
	if(cantidadSegmentos > 0)
	{
		for(int i = 0; i < cantidadSegmentos; i++)
		{
			// LUEGO RECORRO DE A SEGMENTOS Y DE A PAGINAS PARA LOCALIZAR DONDE IRÍA EL DATO
			t_segmento * segmento = dictionary_get(gdt->tablaSegmentos, intToString(i));
			if (strcmp(segmento->archivo,datosPaquete->path) == 0)
			{
					int posicionPagina = lineaBuscada / lineasXPagina;
					pthread_mutex_lock(&mutexSegmentoBuscado);
					nroSegmentoBuscado = segmento->nroSegmento;
					t_list * listaPorSegmento = list_filter(gdt->tablaPaginas, (void *)filtrarPorSegmento);
					pthread_mutex_unlock(&mutexSegmentoBuscado);
					t_pagina * pagina = list_get(listaPorSegmento,posicionPagina);
					while(lineaBuscada > lineasXPagina)
					{
						lineaBuscada -= lineasXPagina;
					}
					if (strcmp(pagina->path,datosPaquete->path) == 0)
					{
						int posicionRelativaPagina = obtenerPosicionRelativaPagina(lineaBuscada);
						enviarInstruccion(direccion(pagina->nroMarco, posicionRelativaPagina), socketSolicitud);
						pudeGuardar = true;
						break;
					}
			}
			if (pudeGuardar)
			{
				break;
			}
		}
		if (pudeGuardar)
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

int finGDTSegPag(t_package pkg, int idGDT, int socketSolicitud)
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
			pthread_mutex_lock(&mutexSegmentoBuscado);
			nroSegmentoBuscado = segmento->nroSegmento;
			t_list * listaPorSegmento = list_filter(gdt->tablaPaginas, (void *)filtrarPorSegmento);
			pthread_mutex_unlock(&mutexSegmentoBuscado);
			for(int j = 0; j < list_size(listaPorSegmento);j++)
			{
				t_pagina * pagina = list_get(listaPorSegmento,j);
				liberarMarco(pagina->nroMarco);
			}
		}
		//ENVIAR MSJ DE EXITO A CPU
		if (enviar(socketSolicitud,FM9_CPU_GDT_FINALIZADO,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al CPU que se ha guardado correctamente la línea.");
			exit_gracefully(-1);
		}
		dictionary_clean_and_destroy_elements(gdt->tablaSegmentos,(void *)liberarSegmento);
		list_clean_and_destroy_elements(gdt->tablaPaginas,(void *)liberarPagina);
	}
	return EXIT_SUCCESS;

}

int ejecutarCargarEsquemaSegPag(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud)
{
	//logica de segmentacion paginada
	//En el 1er paquete recibo la cantidad de paquetes a recibir y el tamaño de cada paquete

	int paginasNecesarias = datosPaquete->cantidadLineasARecibir / lineasXPagina;
	if(datosPaquete->cantidadLineasARecibir % lineasXPagina != 0){
		paginasNecesarias++;
	}
	if(tengoMemoriaDisponible(paginasNecesarias) == 1)
	{
		log_error_mutex(logger, "No hay memoria disponible para cargar el Escriptorio.");
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		logPosicionesLibres(estadoMarcos,SPA);
		if (enviar(socketSolicitud,FM9_DAM_MEMORIA_INSUFICIENTE,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al DAM de la memoria insuficiente.");
//			exit_gracefully(-1);
		}
		return EXIT_FAILURE;
	}
	else
	{
		if (!existeProceso(datosPaquete->pid))
			crearProceso(datosPaquete->pid);

		//ACA HAY QUE FIJARSE EN LAS TABLAS SI TENGO UNA PAGINA ASOCIADA A UN SEGMENTE LIBRE PARA ALMACENAR LOS DATOS
		//EN CASO QUE SI RESERVAR DICHA PAGINA

		char * pidString = intToString(datosPaquete->pid);
		t_gdt * gdt = dictionary_get(tablaProcesos,pidString);
		free(pidString);
		t_segmento * segmento = reservarSegmento(datosPaquete->cantidadLineasARecibir,gdt->tablaSegmentos,datosPaquete->path,paginasNecesarias);
	//	int code = reservarPaginasNecesarias(paginasNecesarias, datosPaquete->pid, datosPaquete->path, cantLineas);
		if (segmento == NULL)
		{
			logPosicionesLibres(estadoLineas,SPA);
			return FM9_DAM_MEMORIA_INSUFICIENTE;
		}
		actualizarTablaDeSegmentos(datosPaquete->pid,segmento);
		reservarPaginasParaSegmento(segmento, datosPaquete, paginasNecesarias);
	//	reservarSegmentoSegmentacionPaginada(gdt, datosPaquete->pid);
		// ESTO PARA QUE ESTA?????
		contLineasUsadas += datosPaquete->cantidadLineasARecibir;

		gdt = dictionary_get(tablaProcesos,pidString);
		char * bufferGuardado = malloc(config->tamMaxLinea);
		int i = 0, lineasGuardadas = 0, tamanioPaqueteReal = 0, offset = 0;
		int lineaLeida = 1;
		int lineaActual = segmento->base;
		int finalSegmento = segmento->base + segmento->limite;
		while (lineaActual < finalSegmento)
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
				int posicionPagina = lineasGuardadas / lineasXPagina;
				pthread_mutex_lock(&mutexSegmentoBuscado);
				nroSegmentoBuscado = segmento->nroSegmento;
				t_list * listaPorSegmento = list_filter(gdt->tablaPaginas, (void *)filtrarPorSegmento);
				pthread_mutex_unlock(&mutexSegmentoBuscado);
				t_pagina * pagina = list_get(listaPorSegmento, posicionPagina);
				memcpy(bufferGuardado+offset, contenidoLinea, tamanioPaquete);
				offset += (tamanioPaquete);
				tamanioPaqueteReal += tamanioPaquete;
				bufferGuardado[tamanioPaqueteReal] = '\n';
				lineaLeida = nroLinea;
				char * lineaAGuardar = prepararLineaMemoria(bufferGuardado);
				int posicionRelativaPagina = obtenerPosicionRelativaPagina(lineasGuardadas);
				guardarLinea(direccion(pagina->nroMarco,posicionRelativaPagina), lineaAGuardar);
				lineasGuardadas++;
				tamanioPaqueteReal = 0;
				lineaActual++;
				offset = 0;
			}
			else
			{
				memcpy(bufferGuardado+offset, contenidoLinea, tamanioPaquete);
				offset += (tamanioPaquete);
				tamanioPaqueteReal += tamanioPaquete;
				lineaLeida = nroLinea;
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
}

int obtenerPosicionRelativaPagina(int lineaActual)
{
	int retornoLinea = lineaActual;
	while(retornoLinea >= lineasXPagina)
	{
		retornoLinea -= lineasXPagina;
	}
	return retornoLinea;
}

int reservarPaginasParaSegmento(t_segmento * segmento, t_infoCargaEscriptorio* datosPaquete, int paginasAReservar)
{
	if (segmento != NULL)
	{
		reservarPaginasNecesarias(paginasAReservar,datosPaquete->pid,datosPaquete->path,datosPaquete->cantidadLineasARecibir,segmento->nroSegmento);
	}
	else
	{
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		logPosicionesLibres(estadoMarcos,SPA);
		return FM9_DAM_MEMORIA_INSUFICIENTE;
	}
	return EXIT_SUCCESS;
}

int flushSegmentacionPaginada(int socketSolicitud, t_datosFlush * data, int accion)
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
			int cantidadLineas = obtenerLineasProceso(data->pid, data->path);
			char * buffer = malloc(sizeof(int));
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
			int j = segmento->base;
			if (strcmp(segmento->archivo, data->path) == 0)
			{
				int limiteSegmento = segmento->base+segmento->limite;
				while(j < limiteSegmento)
				{
					int posicionPagina = j / lineasXPagina;
					pthread_mutex_lock(&mutexSegmentoBuscado);
					nroSegmentoBuscado = segmento->nroSegmento;
					t_list * listaPorSegmento = list_filter(gdt->tablaPaginas, (void *)filtrarPorSegmento);
					pthread_mutex_unlock(&mutexSegmentoBuscado);
					t_pagina * pagina = list_get(listaPorSegmento, posicionPagina);
					if (strcmp(pagina->path, data->path) == 0)
					{
						int posicionRelativaPagina = obtenerPosicionRelativaPagina(j);
						char * linea = obtenerLinea(direccion(pagina->nroMarco, posicionRelativaPagina));
						if (accion == AccionDUMP)
						{
							imprimirInfoAdministrativaSegPag(data->pid);
							printf("Linea %d PID %d: %s\n", j, data->pid, linea);
							log_info_mutex(logger, "Linea %d PID %d: %s\n", j, data->pid, linea);
						}
						if (accion == AccionFLUSH)
						{
							realizarFlush(linea, nroLinea, data->transferSize, socketSolicitud);
						}
						nroLinea++;
					}
					j++;
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

void imprimirInfoAdministrativaSegPag(int pid)
{
	t_gdt * gdt = dictionary_get(tablaProcesos,intToString(pid));
	int cantidadSegmentos = dictionary_size(gdt->tablaSegmentos);
	int i = 0;
	while(i < cantidadSegmentos)
	{
		t_segmento * segmento = dictionary_get(gdt->tablaSegmentos,intToString(i));
		printf("PID %d: Nro Segmento %d - Base %d - Límite %d \n", pid, segmento->nroSegmento, segmento->base, segmento->limite);
		log_info_mutex(logger, "PID %d: Nro Segmento %d - Base %d - Límite %d", pid, segmento->nroSegmento, segmento->base, segmento->limite);
		int j = segmento->base;
		while (j < (segmento->base + segmento->limite))
		{
			int posicionPagina = j / lineasXPagina;
			pthread_mutex_lock(&mutexSegmentoBuscado);
			nroSegmentoBuscado = segmento->nroSegmento;
			t_list * listaPorSegmento = list_filter(gdt->tablaPaginas, (void *)filtrarPorSegmento);
			pthread_mutex_unlock(&mutexSegmentoBuscado);
			t_pagina * pagina = list_get(listaPorSegmento,posicionPagina);
			printf("PID %d: Nro Marco %d - Lineas utilizadas %d \n", pid, pagina->nroMarco, pagina->lineasUtilizadas);
			log_info_mutex(logger, "PID %d: Nro Marco %d - Lineas utilizadas %d", pid, pagina->nroMarco, pagina->lineasUtilizadas);
			j++;
		}
		i++;
	}
}

int ejecutarGuardarEsquemaSegPag(t_package pkg, t_infoGuardadoLinea* datosPaquete, int socket)
{
	t_gdt * gdt = dictionary_get(tablaProcesos,intToString(datosPaquete->pid));
	if (gdt == NULL)
	{
		return FM9_CPU_PROCESO_INEXISTENTE;
	}
	int cantidadSegmentos = dictionary_size(gdt->tablaSegmentos);
	bool pudeGuardar = false;
	int cantidadLineas = obtenerLineasProceso(datosPaquete->pid, datosPaquete->path);
	int lineaBuscada = datosPaquete->linea;
	// PRIMERO VERIFICO SI TENGO LA CANTIDAD DE LINEAS DISPONIBLES PARA REALIZAR EL GUARDADO
	if (cantidadLineas < datosPaquete->linea)
	{
		return FM9_CPU_ACCESO_INVALIDO;
	}
	if(cantidadSegmentos > 0)
	{
		for(int i = 0; i < cantidadSegmentos; i++)
		{
			// LUEGO RECORRO DE A SEGMENTOS Y DE A PAGINAS PARA LOCALIZAR DONDE IRÍA EL DATO
			t_segmento * segmento = dictionary_get(gdt->tablaSegmentos, intToString(i));
			if (strcmp(segmento->archivo,datosPaquete->path) == 0)
			{
				int posicionPagina = lineaBuscada / lineasXPagina;
				pthread_mutex_lock(&mutexSegmentoBuscado);
				nroSegmentoBuscado = segmento->nroSegmento;
				t_list * listaPorSegmento = list_filter(gdt->tablaPaginas, (void *)filtrarPorSegmento);
				pthread_mutex_unlock(&mutexSegmentoBuscado);
				t_pagina * pagina = list_get(listaPorSegmento,posicionPagina);
				if (strcmp(pagina->path,datosPaquete->path) == 0)
				{
					int posicionRelativaPagina = obtenerPosicionRelativaPagina(lineaBuscada);
					guardarLinea(direccion(pagina->nroMarco, posicionRelativaPagina), datosPaquete->datos);
					pudeGuardar = true;
					break;
				}
			}
			if (pudeGuardar)
			{
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

int cerrarArchivoSegPag(t_package pkg, t_infoCerrarArchivo* datosPaquete, int socketSolicitud)
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
				pthread_mutex_lock(&mutexSegmentoBuscado);
				nroSegmentoBuscado = segmento->nroSegmento;
				t_list * listaPorSegmento = list_filter(gdt->tablaPaginas, (void *)filtrarPorSegmento);
				pthread_mutex_unlock(&mutexSegmentoBuscado);
				for(int j = 0; j < list_size(listaPorSegmento);j++)
				{
					t_pagina * pagina = list_get(listaPorSegmento,j);
					liberarMarco(pagina->nroMarco);
				}
			}
		}
		//ENVIAR MSJ DE EXITO A CPU
		if (enviar(socketSolicitud,FM9_CPU_ARCHIVO_CERRADO,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al CPU que se ha guardado correctamente la línea.");
			exit_gracefully(-1);
		}
	}
	return EXIT_SUCCESS;
}
