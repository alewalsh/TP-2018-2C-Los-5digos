/*
 * TPI.c
 *
 *  Created on: 19 nov. 2018
 *      Author: utnso
 */

#include "TPI.h"

int devolverInstruccionTPI(t_package pkg, t_infoDevolverInstruccion* datosPaquete, int socketSolicitud)
{
	pthread_mutex_lock(&mutexPIDBuscado);
	pidBuscado = datosPaquete->pid;
	pathBuscado = datosPaquete->path;
	t_list * paginasProceso = list_filter(tablaPaginasInvertida,(void *)filtrarPorPidYPath);
	pthread_mutex_unlock(&mutexPIDBuscado);
	int posicionBuscada = datosPaquete->posicion;
	int cantidadPaginas = list_size(paginasProceso);
	int cantidadLineas = obtenerLineasProceso(datosPaquete->pid, datosPaquete->path);
	if (cantidadPaginas <= 0)
	{
		return FM9_CPU_PROCESO_INEXISTENTE;
	}
	else if (cantidadPaginas > 0)
	{
		if (cantidadLineas > posicionBuscada)
		{
			int nroPaginaCorrespondiente = datosPaquete->posicion / lineasXPagina;
			t_pagina * paginaCorrespondiente = list_get(paginasProceso, nroPaginaCorrespondiente);
			if (strcmp(paginaCorrespondiente->path,datosPaquete->path) == 0)
			{
				while (posicionBuscada >= lineasXPagina)
				{
					posicionBuscada -= lineasXPagina;
				}
				enviarInstruccion(direccion(paginaCorrespondiente->nroMarco,posicionBuscada), socketSolicitud);
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

int finGDTTPI(t_package pkg, int idGDT, int socketSolicitud)
{
	pthread_mutex_lock(&mutexPIDBuscado);
	pidBuscado = idGDT;
	t_list * paginasProceso = list_filter(tablaPaginasInvertida,(void *)filtrarPorPid);
	pthread_mutex_unlock(&mutexPIDBuscado);
	int cantidadPaginas = list_size(paginasProceso);
	if (list_is_empty(paginasProceso))
	{
		return FM9_CPU_PROCESO_INEXISTENTE;
	}
	int i = 0;
	while(i < cantidadPaginas)
	{
		t_pagina * pagina = list_get(paginasProceso, i);
		liberarMarco(pagina->nroMarco);
		i++;
	}
	//ENVIAR MSJ DE EXITO A CPU
	if (enviar(socketSolicitud,FM9_CPU_GDT_FINALIZADO,pkg.data,pkg.size,logger->logger))
	{
		log_error_mutex(logger, "Error al avisar al CPU que se ha guardado correctamente la línea.");
		exit_gracefully(-1);
	}
	eliminarPaginasDeTPI(paginasProceso);
	//	list_clean_and_destroy_elements(paginasProceso,(void *)liberarPagina);

	return EXIT_SUCCESS;
}

int flushTPI(int socketSolicitud, t_datosFlush * data, int accion)
{
	t_list * paginasProceso;
	if (accion == AccionFLUSH)
	{
		pthread_mutex_lock(&mutexPIDBuscado);
		pidBuscado = data->pid;
		pathBuscado = data->path;
		paginasProceso = list_filter(tablaPaginasInvertida,(void *)filtrarPorPidYPath);
		pthread_mutex_unlock(&mutexPIDBuscado);
	}
	if (accion == AccionDUMP)
	{
		pthread_mutex_lock(&mutexPIDBuscado);
		pidBuscado = data->pid;
		paginasProceso = list_filter(tablaPaginasInvertida,(void *)filtrarPorPid);
		pthread_mutex_unlock(&mutexPIDBuscado);
	}
	int cantidadPaginas = list_size(paginasProceso);
	if (cantidadPaginas <= 0)
	{
		return FM9_CPU_PROCESO_INEXISTENTE;
	}
	else if (cantidadPaginas > 0)
	{
		if (accion == AccionFLUSH)
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
			char * buffer = malloc(sizeof(int));
			char * p = buffer;
			copyIntToBuffer(&p, cantidadLineas);
			if (enviar(socketSolicitud,FM9_DAM_FLUSH,buffer,sizeof(int),logger->logger))
			{
				log_error_mutex(logger, "Error al avisar al CPU que se ha guardado correctamente la línea.");
				exit_gracefully(-1);
			}
		}
		if (accion == AccionDUMP)
		{
			imprimirInfoAdministrativaTPI(data->pid);
		}

		// LUEGO RECORRO CADA SEGMENTO Y VOY ENVIANDO DE A UNA LINEA
		int nroLinea = 1;
		int i = 0;
		while (i < cantidadPaginas)
		{
			t_pagina * pagina = list_get(paginasProceso, i);
			int j = 0;
			if (accion == AccionFLUSH)
			{
				if (strcmp(pagina->path, data->path) == 0)
				{
					while(j < pagina->lineasUtilizadas)
					{
						char * linea = obtenerLinea(direccion(pagina->nroMarco, j));
						realizarFlush(linea, nroLinea, data->transferSize, socketSolicitud);
						nroLinea++;
						j++;
					}
				}
			}
			if (accion == AccionDUMP)
			{
				while(j < pagina->lineasUtilizadas)
				{
					char * linea = obtenerLinea(direccion(pagina->nroMarco, j));
					printf("Nro Pagina %d Linea %d PID %d: %s\n", pagina->nroPagina, j, data->pid, linea);
					log_info_mutex(logger, "Nro Pagina %d Linea %d PID %d: %s\n", pagina->nroPagina, j, data->pid, linea);
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

void imprimirInfoAdministrativaTPI(int pid)
{
	t_list * paginasProceso = list_filter(tablaPaginasInvertida,(void *)filtrarPorPid);
	int cantidadPaginas = list_size(paginasProceso);
	int i = 0;
	while(i < cantidadPaginas)
	{
		t_pagina * pagina = list_get(paginasProceso, i);
		printf("PID %d: Nro Marco %d - Lineas utilizadas %d \n", pid, pagina->nroMarco, pagina->lineasUtilizadas);
		log_info_mutex(logger, "PID %d: Nro Marco %d - Lineas utilizadas %d", pid, pagina->nroMarco, pagina->lineasUtilizadas);
		i++;
	}
}

int ejecutarCargarEsquemaTPI(t_package pkg, t_infoCargaEscriptorio* datosPaquete, int socketSolicitud)
{
	int paginasNecesarias = datosPaquete->cantidadLineasARecibir / lineasXPagina;
	if(datosPaquete->cantidadLineasARecibir % lineasXPagina != 0){
		paginasNecesarias++;
	}
	if(tengoMemoriaDisponible(paginasNecesarias) == 1)
	{
		log_error_mutex(logger, "No hay memoria disponible para cargar el Escriptorio.");
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		logPosicionesLibres(estadoMarcos,TPI);
		if (enviar(socketSolicitud,FM9_DAM_MEMORIA_INSUFICIENTE,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al DAM de la memoria insuficiente.");
			//exit_gracefully(-1);
		}
		return EXIT_FAILURE;
	}
	else
	{
		int code = reservarPaginasNecesarias(paginasNecesarias, datosPaquete->pid, datosPaquete->path, datosPaquete->cantidadLineasARecibir, 0);
		if (code == FM9_DAM_MEMORIA_INSUFICIENTE)
		{
			logPosicionesLibres(estadoMarcos,TPI);
			if (enviar(socketSolicitud,FM9_DAM_MEMORIA_INSUFICIENTE,pkg.data,pkg.size,logger->logger))
			{
				log_error_mutex(logger, "Error al avisar al DAM de la memoria insuficiente.");
			}
			return code;
		}

		pthread_mutex_lock(&mutexPIDBuscado);
		pidBuscado = datosPaquete->pid;
		pathBuscado = datosPaquete->path;
		t_list * paginasProceso = list_filter(tablaPaginasInvertida, (void *) filtrarPorPidYPath);
		pthread_mutex_unlock(&mutexPIDBuscado);

		// ESTO PARA QUE ESTA?????
		contLineasUsadas += datosPaquete->cantidadLineasARecibir;

		// Aviso al DAM que efectivamente hay memoria disponible
		if (enviar(socketSolicitud, FM9_DAM_HAY_MEMORIA, NULL, 0, logger->logger)) {
			log_error_mutex(logger, "Error al enviar aviso de memoria disponible al DAM");
			return EXIT_FAILURE;
		}
		char * bufferGuardado = malloc(config->tamMaxLinea);
		int i = 0, offset = 0, lineasGuardadas = 0, paginaActual = 0, tamanioPaqueteReal = 0;
		int lineaLeida = 1;
		int cantidadLineas = datosPaquete->cantidadLineasARecibir;
		while(i < cantidadLineas)
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
				t_pagina * pagina = list_get(paginasProceso, paginaActual);
				memcpy(bufferGuardado+offset, contenidoLinea, tamanioPaquete);
				offset += tamanioPaquete;
				tamanioPaqueteReal += tamanioPaquete;
				lineaLeida = nroLinea;
				bufferGuardado[tamanioPaqueteReal] = '\n';
				char * lineaAGuardar = prepararLineaMemoria(bufferGuardado);
				guardarLinea(direccion(pagina->nroMarco,lineasGuardadas), lineaAGuardar);
				i++;
				offset = 0;
				tamanioPaqueteReal= 0;
				lineasGuardadas++;
				if (lineasGuardadas == lineasXPagina)
				{
					lineasGuardadas = 0;
					paginaActual++;
				}
			}
			else
			{
				memcpy(bufferGuardado+offset, contenidoLinea, tamanioPaquete);
				offset += tamanioPaquete;
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

int ejecutarGuardarEsquemaTPI(t_package pkg, t_infoGuardadoLinea* datosPaquete, int socket){

	bool pudeGuardar = false;
	pthread_mutex_lock(&mutexPIDBuscado);
	pidBuscado = datosPaquete->pid;
	pathBuscado = datosPaquete->path;
	t_list * paginasProceso = list_filter(tablaPaginasInvertida,(void *)filtrarPorPidYPath);
	pthread_mutex_unlock(&mutexPIDBuscado);
	int posicionReal = datosPaquete->linea - 1;
	int cantidadPaginas = list_size(paginasProceso);
	int cantidadLineas = obtenerLineasProceso(datosPaquete->pid, datosPaquete->path);
	if (cantidadPaginas <= 0)
	{
		return FM9_CPU_PROCESO_INEXISTENTE;
	}
	else if (cantidadPaginas > 0)
	{
		if (cantidadLineas > posicionReal)
		{
			int nroPaginaCorrespondiente = posicionReal / lineasXPagina;
			t_pagina * paginaCorrespondiente = list_get(paginasProceso, nroPaginaCorrespondiente);
			if (strcmp(paginaCorrespondiente->path,datosPaquete->path) == 0)
			{
				while (posicionReal >= lineasXPagina)
				{
					posicionReal -= lineasXPagina;
				}
				guardarLinea(direccion(paginaCorrespondiente->nroMarco,posicionReal), datosPaquete->datos);
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
	bool pudoCerrar = false;
	pthread_mutex_lock(&mutexPIDBuscado);
	pidBuscado = datosPaquete->pid;
	pathBuscado = datosPaquete->path;
	t_list * paginasProceso = list_filter(tablaPaginasInvertida,(void *)filtrarPorPidYPath);
	pthread_mutex_unlock(&mutexPIDBuscado);
	int cantidadPaginas = list_size(paginasProceso);
	if (list_is_empty(paginasProceso))
	{
		// TODO: Si no encuentro una solución realmente elegante, aviso al DAM del error al cerrar el archivo.
//		return EXIT_SUCCESS;
		return FM9_CPU_PROCESO_INEXISTENTE;
	}
	int i = 0;
	while(i < cantidadPaginas)
	{
		t_pagina * pagina = list_get(paginasProceso, i);
		if (strcmp(pagina->path, datosPaquete->path) == 0)
		{
			liberarMarco(pagina->nroMarco);
			pudoCerrar = true;
		}
		i++;
	}
	//ENVIAR MSJ DE EXITO A CPU
	if (enviar(socketSolicitud,FM9_CPU_ARCHIVO_CERRADO,pkg.data,pkg.size,logger->logger))
	{
		log_error_mutex(logger, "Error al avisar al CPU que se ha guardado correctamente la línea.");
		exit_gracefully(-1);
	}
	eliminarPaginasDeTPI(paginasProceso);
	if (pudoCerrar)
	{
		log_trace_mutex(logger, "El archivo %s del proceso %d fue cerrado correctamente", datosPaquete->path, datosPaquete->pid);
	}
	else
	{
		log_trace_mutex(logger, "El archivo %s del proceso %d no fue encontrado para cerrar.", datosPaquete->path, datosPaquete->pid);
	}
//	list_clean_and_destroy_elements(paginasProceso,(void *)liberarPagina);

	return EXIT_SUCCESS;
}

void eliminarPaginasDeTPI(t_list * paginasProceso)
{
	int cantidadPaginas = list_size(paginasProceso);
	int i = 0;
	while(i < cantidadPaginas)
	{
		t_pagina * pagina = list_get(paginasProceso, i);
		int indice = buscarPaginaEnTPI(tablaPaginasInvertida,pagina);
		t_pagina * paginaOriginal = list_remove(tablaPaginasInvertida, indice);
		liberarPagina(paginaOriginal);
		i++;
	}
}

int buscarPaginaEnTPI(t_list * cola, t_pagina * paginaABuscar)
{
	int index = -1;
	int listSize = list_size(cola);
	if(listSize<= 0) return index;

	for(int i = 0; i<listSize;i++){
		t_pagina * pagina = list_get(cola,i);
		if( pagina->pid == paginaABuscar->pid &&
			pagina->nroPagina == paginaABuscar->nroPagina &&
			pagina->nroMarco == paginaABuscar->nroMarco &&
			strcmp(pagina->path, paginaABuscar->path) == 0)
		{
			index = i;
			break;
		}
	}
	return index;
}
