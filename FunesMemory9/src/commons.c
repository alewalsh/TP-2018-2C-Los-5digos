/*
 * commons.c
 *
 *  Created on: 19 nov. 2018
 *      Author: utnso
 */

#include "commons.h"

char * intToString(int numero)
{
	char * string = malloc(sizeof(int));
	sprintf(string, "%d", numero);
	return string;
}

char * prepararLineaMemoria(char * buffer)
{
	char * lineaAGuardar = string_new();
	char ** split = string_split(buffer, "\n");
	int tamanioLinea = string_length(split[0]);
	char * linea = split[0];
	linea[tamanioLinea] = '\n';
	string_append(&lineaAGuardar, linea);
//	lineaAGuardar = string_substring_until(lineaAGuardar, config->tamMaxLinea - 1);
	free(split);
	return lineaAGuardar;
}

void enviarInstruccion(int posicionMemoria, int socketSolicitud)
{
	char * lineaCompleta = obtenerLinea(posicionMemoria);
	char ** split = string_split(lineaCompleta,"\n");
	int size = strlen(split[0]) + 1 + sizeof(int);
	char * buffer = malloc(size);
	char * p = buffer;
	copyStringToBuffer(&p, split[0]);
	log_trace_mutex(logger, "Se devuelve la linea %s", split[0]);
	if (enviar(socketSolicitud,FM9_CPU_DEVUELVO_LINEA,buffer,size,logger->logger))
	{
		log_error_mutex(logger, "Error al avisar al CPU que se ha devuelto correctamente la línea.");
		exit_gracefully(-1);
	}
	free(split);
	free(lineaCompleta);
}

int direccion(int base, int desplazamiento)
{
	// Usar la la base que llega como parametro, sumar el desplazamiento.
	int direccion = 0;
	switch(config->modoEjecucion)
	{
		case SEG:
			direccion = (base + desplazamiento)*config->tamMaxLinea;
			break;
		case TPI: case SPA:
			direccion = (base * config->tamPagina + desplazamiento * config->tamMaxLinea);
			break;
		default:
			break;
	}
//	int direccion = storage + base + desplazamiento;
	return direccion;
}

void inicializarBitmap(t_bitarray **bitArray)
{
	int tamBitarray = 0;
	if (config->modoEjecucion == SEG)
	{
		tamBitarray = cantLineas/8;
		if(cantLineas % 8 != 0)
		{
			tamBitarray++;
		}
	}
	else
	{
		tamBitarray = cantPaginas/8;
		if(cantPaginas % 8 != 0)
		{
			tamBitarray++;
		}
	}


	char* data=malloc(tamBitarray);
	*bitArray = bitarray_create_with_mode(data,tamBitarray,LSB_FIRST); // if create falla error.

	int bit;
	bit = 0;
	if (config->modoEjecucion == SEG)
	{
		while(bit <= cantLineas)
		{
			bitarray_clean_bit(*bitArray, bit);
			bit ++;
		}
	}
	else
	{
		while(bit <= cantPaginas)
		{
			bitarray_clean_bit(*bitArray, bit);
			bit ++;
		}
	}
}

void liberarLineas(int base, int limite)
{
	int i = base;
	int ultimaLinea = base + limite;
	while(i < ultimaLinea)
	{
		bitarray_clean_bit(estadoLineas, i);
		i++;
	}
}

int tengoMemoriaDisponible(int cantACargar)
{
	int memoriaDisponible = 0;
	if (config->modoEjecucion == SEG)
		memoriaDisponible = posicionesLibres(&estadoLineas);
	else
		memoriaDisponible = posicionesLibres(&estadoMarcos);

	if(memoriaDisponible > cantACargar){
		//Tengo espacio disponible.
		return EXIT_SUCCESS;
	}else{
		//No tengo espacio disponible.
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int posicionesLibres(t_bitarray ** bitArray)
{
	int bit = 0, libres = 0;
	while(bit <= cantLineas)
	{
		if(bitarray_test_bit(*bitArray, bit) == 0)
		{
			libres++;
		}
		bit ++;
	}
	return libres;
}

void guardarLinea(int posicionMemoria, char * linea)
{
//	pthread_mutex_lock(&mutexSolicitudes);
	memcpy(storage+posicionMemoria, linea, strlen(linea) + 1);
//	pthread_mutex_unlock(&mutexSolicitudes);
}

void realizarFlush(char * linea, int nroLinea, int transferSize, int socket)
{
	char** arrayLineas = string_split(linea,"\n");
	char * lineaAEnviar = arrayLineas[0];
	string_trim_right(&lineaAEnviar);
	int tamanioLinea = string_length(lineaAEnviar) + 1;
	int tamanioReal = transferSize - sizeof(int) -1;
	int cantidadPaquetes = calcularCantidadPaquetes(tamanioLinea, tamanioReal);
	int size = sizeof(int);
	char * bufferInicial = malloc(size);
	char * p = bufferInicial;
	copyIntToBuffer(&p,cantidadPaquetes);
	if (enviar(socket,FM9_DAM_FLUSH,bufferInicial,size,logger->logger))
	{
		log_error_mutex(logger, "Error al avisar al DAM del error en la carga del escriptorio");
		exit_gracefully(-1);
	}
	free(bufferInicial);

	enviarLineaComoPaquetes(lineaAEnviar, tamanioLinea, transferSize, cantidadPaquetes, nroLinea, socket);
//	free(lineaAEnviar);
	free(arrayLineas);
}


bool filtrarPorPidYPath(t_pagina * pagina)
{
	if (pagina->pid == pidBuscado && strcmp(pagina->path, pathBuscado) == 0)
	{
		return true;
	}
	return false;
}

bool filtrarPorPid(t_pagina * pagina)
{
	if (pagina->pid == pidBuscado)
	{
		return true;
	}
	return false;
}

int calcularCantidadPaquetes(int sizeOfFile, int tamPaqueteRecibir) {
	//Lo divido por mi transfer Size y me quedo con la parte entera
	int cantPart = sizeOfFile / tamPaqueteRecibir;
	if(sizeOfFile % tamPaqueteRecibir != 0){
		cantPart++;
	}
	log_trace_mutex(logger, "Se recibiran %d paquetes", cantPart);

	return cantPart;
}

void enviarLineaComoPaquetes(char * lineaAEnviar, int tamanioLinea, int transferSize, int cantidadPaquetes, int nroLinea, int socket)
{
	char * buffer = lineaAEnviar;
	//Tomo el tamaño total de la linea
	int tamanioReal = transferSize - sizeof(int) - 1;
	int viejoOffset = 0, offset = tamanioReal;
	char * bufferEnvio;
	char * p;
	while (cantidadPaquetes > 0)
	{
		// TODO: Analizar el caso donde hay más cantidad de paquetes, ese offset + transfer_size me genera desconfianza - Ale
		if (cantidadPaquetes == 1)
		{
			char * datosRestantes = string_substring_from(buffer, viejoOffset);
			int longitudDatos = string_length(datosRestantes) + 1;
			int size = sizeof(int) + longitudDatos * sizeof(char);
			if (size > transferSize)
				log_error_mutex(logger, "Ojo, el size %d es mayor al transfer size %d, esto no deberia suceder", size, transferSize);
			bufferEnvio = malloc(size);
			p = bufferEnvio;
			copyStringToBuffer(&p, datosRestantes); //BUFFER
			if(enviar(socket, DAM_FM9_ENVIO_PKG, bufferEnvio, size, logger->logger))
			{
				log_error_mutex(logger, "Error al enviar paquete al FM9.");
				free(bufferEnvio);
			}
			free(datosRestantes);
		}
		else
		// if (cuantosPaquetes > 0)
		{
			char * retornoOffset = string_substring_until(buffer+viejoOffset, tamanioReal);
			int longitudDatos = string_length(retornoOffset);
			int size = sizeof(int) + longitudDatos * sizeof(char);
			if (size > transferSize)
				log_error_mutex(logger, "Ojo, el size %d es mayor al transfer size %d, esto no deberia suceder", size, transferSize);
			bufferEnvio = malloc(size);
			p = bufferEnvio;
			copyStringToBuffer(&p, retornoOffset); //BUFFER
			if(enviar(socket, DAM_FM9_ENVIO_PKG, bufferEnvio, transferSize, logger->logger))
			{
				log_error_mutex(logger, "Error al enviar validacion de archivo al DAM.");
				free(bufferEnvio);
			}
			free(retornoOffset);

		}
		viejoOffset = offset;
		offset = offset + tamanioReal;

		cantidadPaquetes--;
		free(bufferEnvio);
	}
}

char * obtenerLinea(int posicionMemoria)
{
//	pthread_mutex_lock(&mutexSolicitudes);
	char * buffer = malloc(config->tamMaxLinea);
	memcpy(buffer, storage+posicionMemoria, config->tamMaxLinea);
//	pthread_mutex_unlock(&mutexSolicitudes);
	return buffer;
}

void exit_gracefully(int error)
{
	liberarRecursos();
	exit(error);
}

void liberarRecursos()
{
	bitarray_destroy(estadoLineas);
	bitarray_destroy(estadoMarcos);
	pthread_mutex_destroy(&mutexMaster);
	pthread_mutex_destroy(&mutexReadset);
	pthread_mutex_destroy(&mutexExit);
	pthread_mutex_destroy(&mutexMaxfd);
	pthread_mutex_destroy(&mutexSegmentoBuscado);
	pthread_mutex_destroy(&mutexPIDBuscado);
	pthread_mutex_destroy(&mutexPathBuscado);
	pthread_mutex_destroy(&mutexSolicitudes);
	dictionary_destroy_and_destroy_elements(tablaProcesos, (void *)liberarGDT);
	list_destroy_and_destroy_elements(tablaPaginasInvertida, (void *)liberarPagina);
	log_destroy_mutex(logger);
	freeConfig(config, FM9);
	free(storage);
}

bool existeProceso(int pid)
{
	char * pidString = intToString(pid);
	t_gdt * gdt = dictionary_get(tablaProcesos,pidString);
	free(pidString);
	if (gdt != NULL)
		return true;
	return false;
}

void crearProceso(int pid)
{
	t_gdt * gdt = malloc(sizeof(t_gdt));
	gdt->tablaSegmentos = dictionary_create();
	gdt->tablaPaginas = list_create();
	char * pidString = intToString(pid);
	dictionary_put(tablaProcesos,pidString,gdt);
	free(pidString);
}

int reservarPaginasNecesarias(int paginasAReservar, int pid, char * path, int lineasAOcupar, int nroSegmento)
{
	int i = 0, paginasReservadas = 0;
	t_gdt * proceso;
	if (config->modoEjecucion == SPA)
	{
		proceso = dictionary_remove(tablaProcesos, intToString(pid));
	}
	while(paginasReservadas != paginasAReservar)
	{
		if (i == bitarray_get_max_bit(estadoMarcos))
		{
			break;
		}
		if(bitarray_test_bit(estadoMarcos,i) == 0)
		{
			t_pagina * pagina = malloc(sizeof(int)*5 + strlen(path) + 1);
			pagina->pid = pid;
			pagina->path = string_new();
			string_append(&pagina->path,path);
			pagina->nroSegmento = nroSegmento;
			if (config->modoEjecucion == SPA)
			{
				pthread_mutex_lock(&mutexSegmentoBuscado);
				nroSegmentoBuscado = nroSegmento;
				pagina->nroPagina = list_size(list_filter(proceso->tablaPaginas, (void *)filtrarPorSegmento));
				pthread_mutex_unlock(&mutexSegmentoBuscado);
			}
			if (config->modoEjecucion == TPI)
			{
				pthread_mutex_lock(&mutexPIDBuscado);
				pidBuscado = pid;
				t_list * paginasProceso = list_filter(tablaPaginasInvertida,(void *)filtrarPorPid);
				pthread_mutex_unlock(&mutexPIDBuscado);
				pagina->nroPagina = paginasProceso->elements_count;
			}
			pagina->nroMarco = i;
			if (lineasAOcupar >= lineasXPagina)
			{
				pagina->lineasUtilizadas = lineasXPagina;
				lineasAOcupar -= lineasXPagina;
			}
			else
			{
				pagina->lineasUtilizadas = lineasAOcupar;
			}
			if (config->modoEjecucion == TPI)
			{
				actualizarTPI(pagina);
			}
			if (config->modoEjecucion == SPA)
			{
				list_add(proceso->tablaPaginas, pagina);
			}

			ocuparMarco(pagina->nroMarco);
			paginasReservadas++;
		}
		i++;
	}
	if (paginasReservadas != paginasAReservar)
		return FM9_DAM_MEMORIA_INSUFICIENTE;
	if (config->modoEjecucion == SPA)
	{
		dictionary_put(tablaProcesos, intToString(pid), proceso);
	}
	return EXIT_SUCCESS;
}

bool filtrarPorSegmento(t_pagina * pagina)
{
	if (pagina->nroSegmento == nroSegmentoBuscado)
	{
		return true;
	}
	return false;
}

void ocuparMarco(int pagina)
{
	bitarray_set_bit(estadoMarcos, pagina);
}

void actualizarTPI(t_pagina * pagina)
{
	list_add(tablaPaginasInvertida, pagina);
}

void logPosicionesLibres(t_bitarray * bitarray, int modo)
{
	char * modoEjecucion;
	if (modo == TPI)
		modoEjecucion = "página";
	else
		modoEjecucion = "línea";
	int i = 0;
	while (i < cantLineas)
	{
		if(bitarray_test_bit(bitarray,i) == 0)
		{
			log_info(logger->logger, "La %s %d está libre", modoEjecucion, i);
		}
		else
		{
			log_info(logger->logger, "La %s %d está ocupada", modoEjecucion, i);
		}
		i++;
	}
}

int obtenerLineasProceso(int pid, char * path)
{
	int cantidadLineas = 0;
	if (config->modoEjecucion == SPA)
	{
		t_gdt * proceso = dictionary_get(tablaProcesos, intToString(pid));
		int cantidadSegmentos = dictionary_size(proceso->tablaSegmentos);
		for(int i = 0; i < cantidadSegmentos; i++)
		{
			// PRIMERO VERIFICO SI TENGO LA CANTIDAD DE LINEAS DISPONIBLES PARA REALIZAR EL GUARDADO
			t_segmento * segmento = dictionary_get(proceso->tablaSegmentos, intToString(i));
			if (strcmp(segmento->archivo,path)==0){
				for(int j = segmento->base; j < proceso->tablaPaginas->elements_count; j++)
				{
					t_pagina * pagina = list_get(proceso->tablaPaginas, j);
					if (strcmp(pagina->path,path)==0)
						cantidadLineas += pagina->lineasUtilizadas;
				}
			}
		}
	}
	if (config->modoEjecucion == TPI)
	{
		pthread_mutex_lock(&mutexPIDBuscado);
		pidBuscado = pid;
		pathBuscado = path;
		t_list * paginasProceso = list_filter(tablaPaginasInvertida,(void *)filtrarPorPidYPath);
		pthread_mutex_unlock(&mutexPIDBuscado);
		int i = 0, cantPaginas = list_size(paginasProceso);
		while(i < cantPaginas)
		{
			t_pagina * pagina = list_get(paginasProceso,i);
			if (strcmp(pagina->path,path)==0){
				cantidadLineas += pagina->lineasUtilizadas;
			}
			i++;
		}
	}
	if (config->modoEjecucion == SEG)
		{
			t_gdt * gdt = dictionary_get(tablaProcesos, intToString(pid));
			int cantidadSegmentos = dictionary_size(gdt->tablaSegmentos);
			for (int i = 0; i < cantidadSegmentos; i++)
			{
				t_segmento * segmento = dictionary_get(gdt->tablaSegmentos, intToString(i));
				if (strcmp(segmento->archivo,path)==0)
					cantidadLineas += segmento->limite;
			}
		}
	return cantidadLineas;
}


void liberarMarco(int nroMarco)
{
	bitarray_clean_bit(estadoMarcos, nroMarco);
}

bool hayXMarcosLibres(int cantidad)
{
	int i = 0, marcosLibres = 0;
	while(i < bitarray_get_max_bit(estadoMarcos))
	{
		if (bitarray_test_bit(estadoMarcos,i) == 0)
		{
			marcosLibres++;
			if (marcosLibres == cantidad)
			{
				break;
			}
		}
		i++;
	}
	if (marcosLibres < cantidad)
		return false;
	return true;
}

t_segmento * reservarSegmento(int lineasEsperadas, t_dictionary * tablaSegmentos, char * archivo, int paginasAReservar)
{
	int size = sizeof(t_segmento) + string_length(archivo) + 1;
	t_segmento * segmento = malloc(size);
	int lineasLibresContiguas = 0, i = 0, base = -1;
	if (config->modoEjecucion == SEG)
	{
		while(i < cantLineas)
		{
			if(bitarray_test_bit(estadoLineas,i) == 0)
			{
				if(base < 0)
					base = i;
				lineasLibresContiguas++;
				if (lineasLibresContiguas == lineasEsperadas)
				{
					break;
				}
			}
			else
			{
				base = -1;
				lineasLibresContiguas = 0;
			}
			i++;
		}
		actualizarPosicionesLibres(base, lineasEsperadas, &estadoLineas);
	}
	if (config->modoEjecucion == SPA)
	{
		if (hayXMarcosLibres(paginasAReservar)){
			lineasLibresContiguas = lineasEsperadas;
			base =0;
		}
		else
			return NULL;
	}
	if (lineasLibresContiguas == lineasEsperadas)
	{
		segmento->base = base;
		segmento->limite = lineasEsperadas;
		int nroSegmento = tablaSegmentos->table_current_size;
		segmento->nroSegmento = nroSegmento;
		segmento->archivo = archivo;
		return segmento;
	}
	else
		return NULL;
}

void actualizarPosicionesLibres(int base, int lineasEsperadas, t_bitarray ** bitArray)
{
	int posicionInicial = base;
	// TODO: Esto es lo que estaba mal, en vez de comparar contra base + lineas, comparaba solo contra lineas, por lo tanto
	// solo funcionaba para la 1er o 2da carga de escriptorio y después empezaba a superponer las cosas.
	while (posicionInicial < (base + lineasEsperadas))
	{
		bitarray_set_bit((*bitArray), posicionInicial);
		posicionInicial++;
	}
}


void actualizarTablaDeSegmentos(int pid, t_segmento * segmento)
{
	char *  pidString = intToString(pid);
	t_gdt * gdt = dictionary_get(tablaProcesos,pidString);
	char * nroSegmentoString = intToString(segmento->nroSegmento);
	dictionary_put(gdt->tablaSegmentos,nroSegmentoString,segmento);
	free(nroSegmentoString);
	dictionary_put(tablaProcesos,pidString,gdt);
}
