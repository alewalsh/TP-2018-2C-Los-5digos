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

int direccion(int base, int desplazamiento)
{
	// Usar la la base que llega como parametro, sumar el desplazamiento.
	int direccion = 0;
	switch(config->modoEjecucion)
	{
		case SEG:
			direccion = (base + desplazamiento)*config->tamMaxLinea;
			break;
		case TPI:
			direccion = (base * config->tamPagina + desplazamiento * config->tamMaxLinea);
			break;
		default:
			break;
	}
//	int direccion = storage + base + desplazamiento;
	return direccion;
}

void inicializarBitmap(t_bitarray * bitArray)
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
	bitArray = bitarray_create_with_mode(data,tamBitarray,LSB_FIRST); // if create falla error.

	int bit;
	bit = 0;
	while(bit <= cantLineas){
		bitarray_clean_bit(bitArray, bit);
		bit ++;
	}
}

void liberarLineas(int base, int limite)
{
	int i = base;
	while(i <= limite)
	{
		bitarray_clean_bit(estadoLineas, i);
		i++;
	}
}

int tengoMemoriaDisponible(int cantACargar)
{
	int memoriaDisponible = 0;
	if (config->modoEjecucion == SEG)
		memoriaDisponible = posicionesLibres(estadoLineas);
	else
		memoriaDisponible = posicionesLibres(estadoMarcos);

	if(memoriaDisponible > cantACargar){
		//Tengo espacio disponible.
		return EXIT_SUCCESS;
	}else{
		//No tengo espacio disponible.
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int posicionesLibres(t_bitarray * bitArray)
{
	int bit = 0, libres = 0;
	while(bit <= cantLineas)
	{
		if(bitarray_test_bit(bitArray, bit) == 0)
		{
			libres++;
		}
		bit ++;
	}
	return libres;
}

void guardarLinea(int posicionMemoria, char * linea)
{
	memcpy(&storage+posicionMemoria, &linea, strlen(linea));
}

void realizarFlush(char * linea, int nroLinea, int transferSize, int socket)
{
	int noLoUso = 0;
	char** arrayLineas = str_split(linea,'\n',noLoUso);
	char * lineaAEnviar = arrayLineas[0];
	int tamanioLinea = strlen(lineaAEnviar);
	int cantidadPaquetes = tamanioLinea / transferSize;
	if(tamanioLinea % transferSize != 0){
		cantidadPaquetes++;
	}
	char * bufferInicial;
	copyIntToBuffer(&bufferInicial,cantidadPaquetes);
	int size = sizeof(int);
	if (enviar(socket,FM9_DAM_FLUSH,bufferInicial,size,logger->logger))
	{
		log_error_mutex(logger, "Error al avisar al DAM del error en la carga del escriptorio");
		exit_gracefully(-1);
	}
	free(bufferInicial);

	enviarLineaComoPaquetes(lineaAEnviar, tamanioLinea, transferSize, cantidadPaquetes, nroLinea, socket);
	free(lineaAEnviar);
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
		for(int i = 0; i < cantidadPaquetes; i++){
			char sub[transferSize]; // substring a enviar
			int inicio = transferSize*i, // posicion inicial del substring
			fin = transferSize * (i+1); // posicion final del substring

			// Si es el ultimo paquete a enviar el fin es el tamanio de linea
			if(i+1 == cantidadPaquetes){
				fin = tamanioLinea;
			}

			int count = 0;
			while(inicio < fin){
				sub[count] = buffer[inicio];
				inicio++;
				count++;
			}
			//sub[c] = '\0'; <-----------------------TODO VER SI HACE FALTA ESTO

			char * bufferAEnviar;
			int tamanioLineaPkg = count*sizeof(char);
			copyIntToBuffer(&bufferAEnviar, nroLinea);
			copyIntToBuffer(&bufferAEnviar, tamanioLineaPkg);
			copyStringToBuffer(&bufferAEnviar,sub);
			int size = sizeof(int)*2 + tamanioLineaPkg;

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
		char * bufferAEnviar;
		copyIntToBuffer(&bufferAEnviar, nroLinea);
		copyIntToBuffer(&bufferAEnviar, tamanioLinea*sizeof(char));
		copyStringToBuffer(&bufferAEnviar, buffer);
		int size = sizeof(int)*2 + tamanioLinea*sizeof(char);

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
	char * buffer = malloc(sizeof(config->tamMaxLinea));
	memcpy(&buffer, &storage+posicionMemoria, config->tamMaxLinea);
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
	log_destroy_mutex(logger);
	freeConfig(config, FM9);
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
	free(gdt);
}

void reservarPaginasNecesarias(int paginasAReservar, int pid, char * path, int lineasAOcupar)
{
	int i = 0, paginasReservadas = 0;
	t_pagina * pagina = malloc(sizeof(t_pagina));
	pagina->pid = pid;
	pagina->path = path;
	t_gdt * proceso;
	if (config->modoEjecucion == SPA)
	{
		proceso = dictionary_remove(tablaProcesos, intToString(pid));
	}
	while(paginasReservadas != paginasAReservar)
	{
		if(bitarray_test_bit(estadoMarcos,i) == 0)
		{
			pagina->nroPagina = i;
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

			ocuparMarco(pagina->nroPagina);
			paginasReservadas++;
		}
		i++;
	}
	if (config->modoEjecucion == SPA)
	{
		dictionary_put(tablaProcesos, intToString(pid), proceso);
	}
}

void ocuparMarco(int pagina)
{
	bitarray_set_bit(estadoMarcos, pagina);
}

void actualizarTPI(t_pagina * pagina)
{
	list_add(tablaPaginasInvertida, pagina);
}