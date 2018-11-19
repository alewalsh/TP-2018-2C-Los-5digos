/*
 * TPI.c
 *
 *  Created on: 19 nov. 2018
 *      Author: utnso
 */

#include "TPI.h"

int flushTPI(t_package pkg, int socketSolicitud, t_datosFlush * data)
{
	pidBuscado = data->pid;
	t_list * paginasProceso = list_filter(tablaPaginasInvertida,(void *)filtrarPorPid);
	int cantidadPaginas = list_size(paginasProceso);
	if (cantidadPaginas <= 0)
	{
		return FM9_CPU_PROCESO_INEXISTENTE;
	}
	else if (cantidadPaginas > 0)
	{
		// PRIMERO ENVÍO LA CANTIDAD DE LINEAS DEL ARCHIVO
		int cantidadLineas = 0;
		for (int i = 0; i < cantidadPaginas; i++)
		{
			t_pagina * pagina = list_get(paginasProceso, i);
			if (strcmp(pagina->path, data->path) == 0)
			{
				cantidadLineas += pagina->lineasUtilizadas;
			}
		}
		char * buffer;
		copyIntToBuffer(&buffer, cantidadLineas);
		if (enviar(socketSolicitud,FM9_DAM_FLUSH,buffer,sizeof(int),logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al CPU que se ha guardado correctamente la línea.");
			exit_gracefully(-1);
		}

		// LUEGO RECORRO CADA SEGMENTO Y VOY ENVIANDO DE A UNA LINEA
		int nroLinea = 1;
		int i = 0;
		while (i < cantidadPaginas)
		{
			t_pagina * pagina = list_get(paginasProceso, i);
			int j = 0;
			if (strcmp(pagina->path, data->path) == 0)
			{
				while(j < pagina->lineasUtilizadas)
				{
					char * linea = obtenerLinea(direccion(pagina->nroPagina, j));
					realizarFlush(linea, nroLinea, data->transferSize, socketSolicitud);
					nroLinea++;
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

int ejecutarCargarEsquemaTPI(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud){
	int paginasNecesarias = cantLineas / lineasXPagina;
	if(cantLineas % lineasXPagina != 0){
		paginasNecesarias++;
	}
	if(tengoMemoriaDisponible(paginasNecesarias) == 1){
		log_error_mutex(logger, "No hay memoria disponible para cargar el Escriptorio.");
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		if (enviar(socketSolicitud,FM9_DAM_MEMORIA_INSUFICIENTE,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al DAM de la memoria insuficiente.");
			exit_gracefully(-1);
		}
	}

	reservarPaginasNecesarias(paginasNecesarias, datosPaquete->pid, datosPaquete->path, cantLineas);
	pidBuscado = datosPaquete->pid;
	t_list * paginasProceso = list_filter(tablaPaginasInvertida, (void *) filtrarPorPid);
	// ESTO PARA QUE ESTA?????
	contLineasUsadas += cantLineas;

	char * bufferGuardado = malloc(config->tamMaxLinea);
	int i = 0, offset = 0, lineasGuardadas = 0, paginaActual = 0;
	int lineaLeida = 1;
	while(i < cantLineas)
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
			t_pagina * pagina = list_get(paginasProceso,paginaActual);
			int tamanioBuffer = strlen(bufferGuardado);
			bufferGuardado[tamanioBuffer] = '\n';
			guardarLinea(direccion(pagina->nroPagina,lineasGuardadas), bufferGuardado);
			lineasGuardadas++;
			if (lineasGuardadas == lineasXPagina)
			{
				lineasGuardadas = 0;
				paginaActual++;
			}
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

int ejecutarGuardarEsquemaTPI(t_package pkg, t_infoGuardadoLinea* datosPaquete, int socket){

	bool pudeGuardar = false;
	pidBuscado = datosPaquete->pid;
	t_list * paginasProceso = list_filter(tablaPaginasInvertida,(void *)filtrarPorPid);
	int cantidadPaginas = list_size(paginasProceso);
	if (cantidadPaginas <= 0)
	{
		return FM9_CPU_PROCESO_INEXISTENTE;
	}
	else if (cantidadPaginas > 0)
	{
		if (cantidadPaginas * lineasXPagina >= datosPaquete->linea)
		{
			int nroPaginaCorrespondiente = datosPaquete->linea / lineasXPagina;
			t_pagina * paginaCorrespondiente = list_get(paginasProceso, nroPaginaCorrespondiente);
			if (strcmp(paginaCorrespondiente->path,datosPaquete->path) == 0)
			{
				while (datosPaquete->linea >= lineasXPagina){
					datosPaquete->linea -= lineasXPagina;
				}
				guardarLinea(direccion(paginaCorrespondiente->nroPagina,datosPaquete->linea), datosPaquete->datos);
				pudeGuardar = true;
			}
		}
		else
		{
			return FM9_CPU_ACCESO_INVALIDO;
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

int cerrarArchivoTPI(t_package pkg, t_infoCerrarArchivo* datosPaquete, int socketSolicitud)
{
	pidBuscado = datosPaquete->pid;
	t_list * paginasProceso = list_filter(tablaPaginasInvertida,(void *)filtrarPorPid);
	int cantidadPaginas = list_size(paginasProceso);
	if (list_is_empty(paginasProceso))
	{
		return FM9_CPU_PROCESO_INEXISTENTE;
	}
	int i = 0;
	while(i < cantidadPaginas)
	{
		t_pagina * pagina = list_get(paginasProceso, i);
		if (strcmp(pagina->path, datosPaquete->path) == 0)
		{
			liberarMarco(pagina);
		}
	}
	//ENVIAR MSJ DE EXITO A CPU
	if (enviar(socketSolicitud,FM9_CPU_ARCHIVO_CERRADO,pkg.data,pkg.size,logger->logger))
	{
		log_error_mutex(logger, "Error al avisar al CPU que se ha guardado correctamente la línea.");
		exit_gracefully(-1);
	}
	list_clean_and_destroy_elements(paginasProceso,(void *)liberarPagina);

	return EXIT_SUCCESS;
}

void liberarMarco(t_pagina * pagina)
{
	bitarray_clean_bit(estadoMarcos, pagina->nroPagina);
}