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

void enviarInstruccion(int posicionMemoria, int socketSolicitud)
{
	char * lineaCompleta = obtenerLinea(posicionMemoria);
	char ** split = string_split(lineaCompleta,"\n");
	int size = strlen(split[0]) + 1 + sizeof(int);
	char * buffer = malloc(size);
	char * p = buffer;
	copyStringToBuffer(&p, split[0]);
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
		tamBitarray = cantLineas/8;
	else
		tamBitarray = cantPaginas/8;

	if(cantLineas % 8 != 0){
		tamBitarray++;
	}
	char* data=malloc(tamBitarray);
	*bitArray = bitarray_create_with_mode(data,tamBitarray,LSB_FIRST); // if create falla error.

	int bit;
	bit = 0;
	while(bit <= cantLineas){
		bitarray_clean_bit(*bitArray, bit);
		bit ++;
	}
}

void liberarLineas(int base, int limite)
{
	int i = base;
	int ultimaLinea = base + limite;
	while(i <= ultimaLinea)
	{
		bitarray_clean_bit(estadoLineas, i);
		i++;
	}
}

int tengoMemoriaDisponible(int cantACargar)
{
	int memoriaDisponible = 0;
	if (config->modoEjecucion == SEG || config->modoEjecucion == SPA)
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
	memcpy(storage+posicionMemoria, linea, strlen(linea) + 1);
}

void realizarFlush(char * linea, int nroLinea, int transferSize, int socket)
{
	char** arrayLineas = string_split(linea,"\n");
	char * lineaAEnviar = arrayLineas[0];
	int tamanioLinea = string_length(lineaAEnviar) + 1;
	int cantidadPaquetes = tamanioLinea / transferSize;
	if(tamanioLinea % transferSize != 0){
		cantidadPaquetes++;
	}
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


bool filtrarPorPid(t_pagina * pagina)
{
	if (pagina->pid == pidBuscado)
	{
		return true;
	}
	return false;
}

void enviarLineaComoPaquetes(char * lineaAEnviar, int tamanioLinea, int transferSize, int cantidadPaquetes, int nroLinea, int socket)
{
	char * buffer = lineaAEnviar;
	//Si la linea es mayor a mi transfer size debo enviarlo en varios paquetes
	if(tamanioLinea > transferSize)
	{
		//por cada paquete...
		for (int l = 0; l < cantidadPaquetes; l++)
		{
			char * sub; //substring a enviar
			int inicio = transferSize * nroLinea, //posicion inicial del substring
			fin = (transferSize * (nroLinea + 1))-1; //posicion final del substring

			//Si es el ultimo paquete a enviar el fin es el tamanio de linea
			if (nroLinea + 1 == cantidadPaquetes) {
				fin = tamanioLinea;
			}

			sub = string_substring(buffer,inicio,fin);

			int size = sizeof(int) + (strlen(sub)+1) * sizeof(char);
			char * bufferAEnviar = (char *) malloc(size);
			char * p = bufferAEnviar;
			copyStringToBuffer(&p, sub); //BUFFER

			//enviar
			if(enviar(socket,DAM_FM9_ENVIO_PKG,bufferAEnviar,size,logger->logger))
			{
				log_error_mutex(logger, "Error al enviar info del escriptorio a FM9");
				free(bufferAEnviar);
			}

			free(bufferAEnviar);
		}
	}
	else
	{
		//Si está dentro del tamaño permitido se envía la linea
		int size = sizeof(int) + tamanioLinea;
		char * bufferAEnviar = malloc(size);
		char * p = bufferAEnviar;
		copyStringToBuffer(&p, buffer);

		if(enviar(socket,DAM_FM9_ENVIO_PKG,bufferAEnviar,size,logger->logger))
		{
			log_error_mutex(logger, "Error al enviar info del escriptorio a FM9");
			free(buffer);
			free(bufferAEnviar);
		}
		free(bufferAEnviar);
	}
	free(buffer);
}

char * obtenerLinea(int posicionMemoria)
{
	char * buffer = malloc(config->tamMaxLinea);
	memcpy(buffer, storage+posicionMemoria, config->tamMaxLinea);
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
	pthread_mutex_destroy(&mutexPaginaBuscada);
	pthread_mutex_destroy(&mutexPIDBuscado);

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
	t_pagina * pagina = malloc(sizeof(t_pagina));
	pagina->pid = pid;
	pagina->path = path;
	t_gdt * proceso;
	if (config->modoEjecucion == SPA)
	{
		proceso = dictionary_remove(tablaProcesos, intToString(pid));
		pagina->nroSegmento = nroSegmento;
	}
	while(paginasReservadas != paginasAReservar)
	{
		if (i == bitarray_get_max_bit(estadoMarcos))
		{
			break;
		}
		if(bitarray_test_bit(estadoMarcos,i) == 0)
		{
			pagina->nroPagina = paginasReservadas;
			pagina->nroMarco = i;
			if (lineasAOcupar > lineasXPagina)
			{
				pagina->lineasUtilizadas = lineasXPagina;
				lineasAOcupar -= lineasXPagina;
			}
			else
			{
				pagina->lineasUtilizadas = lineasAOcupar;
			}
			if (config->modoEjecucion == TPI)
				actualizarTPI(pagina);
			if (config->puertoFM9 == SPA)
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

int obtenerLineasProcesoPath(int pid, char * path)
{
	int cantidadLineas = 0;


	return cantidadLineas;
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
				for(int j = segmento->base; j < (segmento->base+segmento->limite); j++)
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
		t_list * paginasProceso = list_filter(tablaPaginasInvertida,(void *)filtrarPorPid);
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
		list_destroy_and_destroy_elements(paginasProceso, (void *)liberarPagina);
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

void liberarMarco(t_pagina * pagina)
{
	bitarray_clean_bit(estadoMarcos, pagina->nroMarco);
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
	if (config->modoEjecucion == SEG){
		while(i <= cantLineas)
		{
			if(bitarray_test_bit(estadoLineas,i) == 0)
			{
				if(base<0)
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
		actualizarPosicionesLibres(base, lineasEsperadas, estadoLineas);
	}
	if (config->modoEjecucion == SPA)
	{
		if (hayXMarcosLibres(paginasAReservar))
			lineasLibresContiguas = lineasEsperadas;
		else
			return NULL;
	}
	if (lineasLibresContiguas == lineasEsperadas)
	{
		segmento->base = base;
		segmento->limite = lineasEsperadas-1;
		int nroSegmento = tablaSegmentos->table_current_size;
		segmento->nroSegmento = nroSegmento;
		segmento->archivo = archivo;
		return segmento;
	}
	else
		return NULL;
}

void actualizarPosicionesLibres(int base, int lineasEsperadas, t_bitarray * bitArray)
{
	int posicionInicial = base;
	while (posicionInicial < lineasEsperadas)
	{
		bitarray_set_bit(bitArray, posicionInicial);
		posicionInicial++;
	}
}
