/*
 * segPaginada.c
 *
 *  Created on: 19 nov. 2018
 *      Author: utnso
 */

#include "segPaginada.h"

int ejecutarCargarEsquemaSegPag(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud)
{
	//logica de segmentacion paginada
	//En el 1er paquete recibo la cantidad de paquetes a recibir y el tamaño de cada paquete

	int paginasNecesarias = cantLineas / lineasXPagina;
	if(cantLineas % lineasXPagina != 0){
		paginasNecesarias++;
	}
	if(tengoMemoriaDisponible(paginasNecesarias) == 1){
		log_error_mutex(logger, "No hay memoria disponible para cargar el Escriptorio.");
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		logPosicionesLibres(estadoMarcos,SPA);
		if (enviar(socketSolicitud,FM9_DAM_MEMORIA_INSUFICIENTE,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al DAM de la memoria insuficiente.");
			exit_gracefully(-1);
		}
	}
	if (!existeProceso(datosPaquete->pid))
		crearProceso(datosPaquete->pid);

	//ACA HAY QUE FIJARSE EN LAS TABLAS SI TENGO UNA PAGINA ASOCIADA A UN SEGMENTE LIBRE PARA ALMACENAR LOS DATOS
	//EN CASO QUE SI RESERVAR DICHA PAGINA

	int code = reservarPaginasNecesarias(paginasNecesarias, datosPaquete->pid, datosPaquete->path, cantLineas);
	if (code == FM9_DAM_MEMORIA_INSUFICIENTE)
	{
		logPosicionesLibres(estadoMarcos,TPI);
		return code;
	}
	char * pidString = intToString(datosPaquete->pid);
	t_gdt * gdt = dictionary_remove(tablaProcesos,pidString);
	free(pidString);
	reservarSegmentoSegmentacionPaginada(gdt, datosPaquete->pid);
	// ESTO PARA QUE ESTA?????
	contLineasUsadas += cantLineas;

	gdt = dictionary_get(tablaProcesos,pidString);
	char * bufferGuardado = malloc(config->tamMaxLinea);
	int i = 0, offset = 0, lineasGuardadas = 0;
	int lineaLeida = 1;
	int cantidadSegmentos = dictionary_size(gdt->tablaSegmentos);
	while(i < cantidadSegmentos)
	{
		t_segmento * segmento = dictionary_get(gdt->tablaSegmentos, intToString(i));
		int paginaActual = segmento->base;
		while (paginaActual < (segmento->base+segmento->limite))
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
				paginaBuscada = paginaActual;
				t_pagina * pagina = list_find(gdt->tablaPaginas, (void *)filtrarPorNroPagina);
				int tamanioBuffer = strlen(bufferGuardado);
				bufferGuardado[tamanioBuffer] = '\n';
				guardarLinea(direccion(pagina->nroPagina,lineasGuardadas), bufferGuardado);
				lineasGuardadas++;
				if (lineasGuardadas == lineasXPagina)
				{
					lineasGuardadas = 0;
					paginaActual++;
				}
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
		i++;
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

int reservarSegmentoSegmentacionPaginada(t_gdt * gdt, int pid)
{
	int i = 0;
	int cantidadPaginas = list_size(gdt->tablaPaginas);
	if (cantidadPaginas > 0)
	{
		if (cantidadPaginas == 1)
		{
			t_segmento * segmento = malloc(sizeof(t_segmento));
			t_pagina * pagina = list_get(gdt->tablaPaginas,0);
			segmento->archivo = pagina->path;
			segmento->base = pagina->nroPagina;
			segmento->limite = cantidadPaginas;
			segmento->nroSegmento = 0;
			dictionary_put(gdt->tablaSegmentos,intToString(segmento->nroSegmento),segmento);
		}
		else
		{
			t_list * paginasASegmentar = list_create();
			int j = 0;
			while (i < cantidadPaginas)
			{
				t_pagina * pagina = list_get(gdt->tablaPaginas,i);
				t_pagina * pagina2 = list_get(gdt->tablaPaginas,i+1);
				if (pagina2 != NULL && pagina->nroPagina + 1 == pagina2->nroPagina)
				{
					list_add(paginasASegmentar,pagina);
				}
				else
				{
					list_add(paginasASegmentar,pagina);
					t_segmento * segmento = malloc(sizeof(t_segmento));
					segmento->archivo = pagina->path;
					segmento->base = pagina->nroPagina;
					segmento->limite = list_size(paginasASegmentar);
					segmento->nroSegmento = j;
					dictionary_put(gdt->tablaSegmentos,intToString(segmento->nroSegmento),segmento);
					list_clean(paginasASegmentar);
					j++;
				}
				i++;
			}
			list_destroy(paginasASegmentar);
		}
		dictionary_put(tablaProcesos, intToString(pid), gdt);
	}
	else
	{
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		logPosicionesLibres(estadoMarcos,SPA);
		return FM9_DAM_MEMORIA_INSUFICIENTE;
	}
	return EXIT_SUCCESS;
}

int flushSegmentacionPaginada(t_package pkg, int socketSolicitud, t_datosFlush * data)
{
	t_gdt * gdt = dictionary_get(tablaProcesos,intToString(data->pid));
	if (gdt == NULL)
	{
		return FM9_DAM_ARCHIVO_INEXISTENTE;
	}
	int cantidadSegmentos = dictionary_size(gdt->tablaSegmentos);
	if(cantidadSegmentos > 0)
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

		// LUEGO RECORRO CADA SEGMENTO Y VOY ENVIANDO DE A UNA LINEA
		int nroLinea = 1;
		for(int i = 0; i < cantidadSegmentos; i++)
		{
			t_segmento * segmento = dictionary_get(gdt->tablaSegmentos, intToString(i));
			int j = segmento->base;
			if (strcmp(segmento->archivo, data->path) == 0)
			{
				while(j < (segmento->base+segmento->limite))
				{
					paginaBuscada = j;
					int idLinea = 0;
					t_pagina * pagina = list_find(gdt->tablaPaginas, (void *) filtrarPorNroPagina);
					while (idLinea < pagina->lineasUtilizadas)
					{
						char * linea = obtenerLinea(direccion(pagina->nroPagina, idLinea));
						realizarFlush(linea, nroLinea, data->transferSize, socketSolicitud);
						idLinea++;
						nroLinea++;
					}
					j++;
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

int ejecutarGuardarEsquemaSegPag(t_package pkg, t_infoGuardadoLinea* datosPaquete, int socket)
{
	t_gdt * gdt = dictionary_get(tablaProcesos,intToString(datosPaquete->pid));
	if (gdt == NULL)
	{
		return FM9_CPU_PROCESO_INEXISTENTE;
	}
	int cantidadSegmentos = dictionary_size(gdt->tablaSegmentos);
	bool pudeGuardar = false;
	int cantidadLineas = obtenerLineasProceso(datosPaquete->pid);
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
			int j = segmento->base;
			while(j < (segmento->base + segmento->limite))
			{
				if (lineaBuscada >= lineasXPagina)
				{
					lineaBuscada -= lineasXPagina;
				}
				else
				{
					paginaBuscada = j;
					t_pagina * pagina = list_find(gdt->tablaPaginas,(void * )filtrarPorNroPagina);
					if (strcmp(pagina->path,datosPaquete->path) == 0)
					{
						guardarLinea(direccion(pagina->nroPagina, lineaBuscada), datosPaquete->datos);
						pudeGuardar = true;
						break;
					}
				}
				j++;
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

bool filtrarPorNroPagina(t_pagina * pagina)
{
	if (pagina->nroPagina == paginaBuscada)
	{
		return true;
	}
	return false;
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
				int j = segmento->base;
				while(i < (segmento->base + segmento->limite))
				{
					paginaBuscada = j;
					t_pagina * pagina = list_find(gdt->tablaPaginas, (void *)filtrarPorNroPagina);
					liberarMarco(pagina);
				}
			}
		}
		//ENVIAR MSJ DE EXITO A CPU
		if (enviar(socketSolicitud,FM9_CPU_ARCHIVO_CERRADO,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al CPU que se ha guardado correctamente la línea.");
			exit_gracefully(-1);
		}
		dictionary_clean_and_destroy_elements(gdt->tablaSegmentos,(void *)liberarSegmento);
		list_clean_and_destroy_elements(gdt->tablaPaginas,(void *)liberarSegmento);
	}
	return EXIT_SUCCESS;
}
