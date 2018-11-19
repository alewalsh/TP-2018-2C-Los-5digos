/*
 ============================================================================
 Name        : FunesMemory9.c
 Author      : Nicolas Barrelier
 Version     :
 Copyright   : 
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "FunesMemory9.h"

int main(int argc, char** argv) {
	logger = log_create_mutex("FM9.log", "FM9", true, LOG_LEVEL_INFO);
	config = cargarConfiguracion(argv[1], FM9, logger->logger);
	inicializarContadores();
	storage = malloc(config->tamMemoria);
	manejarConexiones();
	free(storage);
	return EXIT_SUCCESS;
}

void inicializarContadores(){
	contLineasUsadas = 0;
	cantLineas = config->tamMemoria / config->tamMaxLinea;
	cantPaginas = config->tamMemoria / config->tamPagina;
	lineasXPagina = config->tamPagina / config->tamMaxLinea;
	tablaProcesos = dictionary_create();
	tablaPaginasInvertida = list_create();
	inicializarBitmap(estadoLineas);
	inicializarBitmap(estadoMarcos);
}

void exit_gracefully(int error)
{
	liberarRecursos();
	exit(error);
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

void manejarConexiones(){
	int socketListen, i,nuevoFd;
	uint16_t handshake;
	t_package pkg;

	printf("%d",config->puertoFM9);
	//Creo el socket y me quedo escuchando
	if (escuchar(config->puertoFM9, &socketListen, logger->logger)) {
		liberarRecursos();
		pthread_exit(NULL);
	}

	log_trace_mutex(logger, "El socket de escucha de FM9 es: %d", socketListen);
	log_info_mutex(logger, "El socket de escucha de FM9 es: %d", socketListen);

	addNewSocketToMaster(socketListen);

	while (1) {

		updateReadset();

		//Hago un select sobre el conjunto de sockets activo
		int result = select(getMaxfd() + 1, &readset, NULL, NULL, NULL);
		if (result == -1) {
			log_error_mutex(logger, "Error en el select: %s", strerror(errno));
			exit_gracefully(1);
		}

		log_trace_mutex(logger, "El valor del select es: %d", result);
		log_trace_mutex(logger, "Analizo resultado del select para ver quien me hablo");

		for (i = 0; i <= getMaxfd(); i++) {

			if (isSetted(i)) { // ¡¡tenemos datos!!

				if (i == socketListen) {
					// CAMBIOS EN EL SOCKET QUE ESCUCHA, acepto las nuevas conexiones
					log_trace_mutex(logger, "Cambios en Listener de FM9, se gestionara la conexion correspondiente");
					if (acceptConnection(socketListen, &nuevoFd, FM9_HSK, &handshake, logger->logger))
					{
						log_error_mutex(logger, "No se acepto la nueva conexion solicitada");
					} else
					{
						// añadir al conjunto maestro
						log_trace_mutex(logger, "Se acepto la nueva conexion solicitada en el SELECT");
						addNewSocketToMaster(nuevoFd);
					}
				} else {
					 //gestionar datos de un cliente
					if (recibir(i, &pkg, logger->logger)) {
						log_error_mutex(logger, "No se pudo recibir el mensaje");
						//handlerDisconnect(i);
					} else {
						manejarSolicitud(pkg, i);
					}

				}
			}
		}
	}
}

void liberarRecursos()
{
	bitarray_destroy(estadoLineas);
	pthread_mutex_destroy(&mutexMaster);
	pthread_mutex_destroy(&mutexReadset);
	log_destroy_mutex(logger);
	freeConfig(config, FM9);
}

void manejarSolicitud(t_package pkg, int socketFD) {

    switch (pkg.code) {
        case CPU_FM9_CONNECT:
			printf("Se ha conectado el CPU.");
            break;
		case DAM_FM9_CONNECT:
			printf("Se ha conectado el DAM.");
			break;
		case DAM_FM9_CARGAR_ESCRIPTORIO:
			if(cargarEscriptorioSegunEsquemaMemoria(pkg,socketFD)){
				log_error_mutex(logger,"Error al cargar el escriptorio en la memoria");
			}
			break;
        case DAM_FM9_FLUSH:
        	if(realizarFlushSegunEsquemaMemoria(pkg,socketFD)){
				log_error_mutex(logger,"Error al cargar el escriptorio en la memoria");
			}
			break;
        case CPU_FM9_ASIGNAR:
        	if(guardarLineaSegunEsquemaMemoria(pkg,socketFD)){
				log_error_mutex(logger,"Error al guardar la data recibida en memoria");
			}
			break;
        case CPU_FM9_CERRAR_ARCHIVO:
			if(cerrarArchivoSegunEsquemaMemoria(pkg,socketFD)){
				log_error_mutex(logger,"Error al guardar la data recibida en memoria");
			}
			break;
        case SOCKET_DISCONECT:
            close(socketFD);
            deleteSocketFromMaster(socketFD);
            break;
        default:
            log_warning_mutex(logger, "El mensaje recibido es: %s", codigoIDToString(pkg.code));
            log_warning_mutex(logger, "Ojo, estas recibiendo un mensaje que no esperabas.");
            break;

    }

    free(pkg.data);

}

int cerrarArchivoSegunEsquemaMemoria(t_package pkg, int socketSolicitud)
{
	switch(config->modoEjecucion)
	{
		case SEG:
			logicaCerrarArchivoSegmentacion(pkg, socketSolicitud);
			break;
		case TPI:
			logicaCerrarArchivoTPI(pkg, socketSolicitud);
			break;
		default:
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void logicaCerrarArchivoTPI(t_package pkg, int socketSolicitud)
{
	int code = cerrarArchivoTPI(pkg, socketSolicitud);
	if (code != 0)
	{
		log_error_mutex(logger,"Error al cerrar el archivo indicado. Esquema: TPI");
		if (enviar(socketSolicitud,code,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al CPU del error al cerrar un archivo.");
			exit_gracefully(-1);
		}
	}
	else
	{
		log_info_mutex(logger, "Se cerró correctamente el archivo indicado.");
	}
}

void logicaCerrarArchivoSegmentacion(t_package pkg, int socketSolicitud)
{
	int code = cerrarArchivoSegmentacion(pkg, socketSolicitud);
	if (code != 0)
	{
		log_error_mutex(logger,"Error al cerrar el archivo indicado. Esquema: SEG");
		if (enviar(socketSolicitud,code,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al CPU del error al cerrar un archivo.");
			exit_gracefully(-1);
		}
	}
	else
	{
		log_info_mutex(logger, "Se cerró correctamente el archivo indicado.");
	}
}

int cerrarArchivoTPI(t_package pkg, int socketSolicitud)
{
	char * buffer = pkg.data;
	int pid = copyIntFromBuffer(&buffer);
	char * path = copyStringFromBuffer(&buffer);
	pidBuscado = pid;
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
		if (strcmp(pagina->path, path) == 0)
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
	list_clean_and_destroy_elements(paginasProceso,(void *)liberar_pagina);

	return EXIT_SUCCESS;
}

void liberarMarco(t_pagina * pagina)
{
	bitarray_clean_bit(estadoMarcos, pagina->nroPagina);
}

int cerrarArchivoSegmentacion(t_package pkg, int socketSolicitud)
{
	char * buffer = pkg.data;
	int pid = copyIntFromBuffer(&buffer);
	char * path = copyStringFromBuffer(&buffer);
	char * pidString = intToString(pid);
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
			if (strcmp(segmento->archivo,path) == 0)
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
		dictionary_clean_and_destroy_elements(gdt->tablaSegmentos,(void *)liberar_segmento);
	}
	return EXIT_SUCCESS;
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

char * intToString(int numero)
{
	char * string = malloc(sizeof(int));
	sprintf(string, "%d", numero);
	return string;
}

static void liberar_segmento(t_segmento *self)
{
	free(self->archivo);
	free(self);
}

static void liberar_pagina(t_pagina * self)
{
	free(self->path);
	free(self);
}
//--------------------------------------GUARDAR DATOS EN MEMORIA SEGUN ESQUEMA ELEGIDO

int guardarLineaSegunEsquemaMemoria(t_package pkg, int socketSolicitud){
	switch (config->modoEjecucion)
	{
		case SEG:
			logicaGuardarSegmentacion(pkg, socketSolicitud);
			break;
		case TPI:
			if(ejecutarGuardarEsquemaTPI(pkg, socketSolicitud)){
				log_error_mutex(logger,"Error al guardar la linea recibida en Memoria. Esquema: TPI");
				//ENVIAR ERROR AL DMA (socketSolicitud)
			}else{
				log_info_mutex(logger, "Se cargó correctamente la linea recibida en Memoria");
			}
			break;
		case SPA:
			if(ejecutarGuardarEsquemaSegPag(pkg)){
				log_error_mutex(logger,"Error al guardar la linea recibida en Memoria. Esquema: SPA");
				//ENVIAR ERROR AL DMA (socketSolicitud)
			}else{
				log_info_mutex(logger, "Se cargó correctamente la linea recibida en Memoria");
			}
			break;
		default:
			log_warning_mutex(logger, "No se especifico el esquema para el guardado de lineas");
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void logicaGuardarSegmentacion(t_package pkg, int socketSolicitud)
{
	int code = ejecutarGuardarEsquemaSegmentacion(pkg, socketSolicitud);
	if(code != 0){
		log_error_mutex(logger,"Error al guardar la linea recibida en Memoria. Esquema: SEG");
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		if (code == 1)
			code = FM9_CPU_MEMORIA_INSUFICIENTE;
		if (enviar(socketSolicitud,code,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al DAM del error en el guardado de una linea.");
			exit_gracefully(-1);
		}
	}
	else
	{
		log_info_mutex(logger, "Se cargó correctamente la linea recibida en Memoria");
	}
}

// Lógica de segmentación pura
int ejecutarGuardarEsquemaSegmentacion(t_package pkg, int socket)
{
	char * buffer = pkg.data;
	int pid = copyIntFromBuffer(&buffer);
	char * path = copyStringFromBuffer(&buffer);
	int linea = copyIntFromBuffer(&buffer);
	char * datos = copyStringFromBuffer(&buffer);

	t_gdt * gdt = dictionary_get(tablaProcesos,intToString(pid));
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
			if (strcmp(segmento->archivo,path) == 0 && segmento->limite >= linea)
			{
				guardarLinea(direccion(segmento->base,linea), datos);
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

int ejecutarGuardarEsquemaTPI(t_package pkg, int socket){
	char * buffer = pkg.data;
	int pid = copyIntFromBuffer(&buffer);
	char * path = copyStringFromBuffer(&buffer);
	int linea = copyIntFromBuffer(&buffer);
	char * datos = copyStringFromBuffer(&buffer);

	bool pudeGuardar = false;
	pidBuscado = pid;
	t_list * paginasProceso = list_filter(tablaPaginasInvertida,(void *)filtrarPorPid);
	int cantidadPaginas = list_size(paginasProceso);
	if (cantidadPaginas <= 0)
	{
		return FM9_CPU_PROCESO_INEXISTENTE;
	}
	else if (cantidadPaginas > 0)
	{
		if (cantidadPaginas * lineasXPagina >= linea)
		{
			int nroPaginaCorrespondiente = linea / lineasXPagina;
			t_pagina * paginaCorrespondiente = list_get(paginasProceso, nroPaginaCorrespondiente);
			if (strcmp(paginaCorrespondiente->path,path) == 0)
			{
				while (linea >= lineasXPagina){
					linea -= lineasXPagina;
				}
				guardarLinea(direccion(paginaCorrespondiente->nroPagina,linea), datos);
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

int ejecutarGuardarEsquemaSegPag(t_package pkg){
	//logica de segmentacion paginada
	return EXIT_SUCCESS;
}

//------------------------------CARGAR ESCRIPTORIO EN MEMORIA SEGUN ESQUEMA ELEGIDO
int cargarEscriptorioSegunEsquemaMemoria(t_package pkg, int socketSolicitud){
	switch (config->modoEjecucion){
	case SEG:
		logicaCargarEscriptorioSegmentacion(pkg, socketSolicitud);
	    break;
	case TPI:
		logicaCargarEscriptorioTPI(pkg,socketSolicitud);
		break;
	case SPA:
		ejecutarCargarEsquemaSegPag(pkg,socketSolicitud);
		break;
	default:
		log_warning_mutex(logger, "No se especifico el esquema para el guardado de lineas");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void logicaCargarEscriptorioTPI(t_package pkg, int socketSolicitud)
{
	int error = ejecutarCargarEsquemaTPI(pkg,socketSolicitud);
	if (error != 0)
	{
		log_error_mutex(logger,"Error al cargar el Escriptorio recibido. Esquema: TPI");
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		if (enviar(socketSolicitud,FM9_DAM_MEMORIA_INSUFICIENTE,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al DAM del error en la carga del escriptorio");
			exit_gracefully(-1);
		}
	}
	else
	{
		log_info_mutex(logger, "Se cargó correctamente el Escriptorio recibido.");
	}
}

void logicaCargarEscriptorioSegmentacion(t_package pkg, int socketSolicitud)
{
	int error = ejecutarCargarEsquemaSegmentacion(pkg,socketSolicitud);
	if (error != 0)
	{
		log_error_mutex(logger,"Error al cargar el Escriptorio recibido. Esquema: SEG");
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		if (enviar(socketSolicitud,FM9_DAM_MEMORIA_INSUFICIENTE,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al DAM del error en la carga del escriptorio");
			exit_gracefully(-1);
		}
	}
	else
	{
		log_info_mutex(logger, "Se cargó correctamente el Escriptorio recibido.");
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

void ocuparMarco(int pagina)
{
	bitarray_set_bit(estadoMarcos, pagina);
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

void actualizarTablaDeSegmentos(int pid, t_segmento * segmento)
{
	char pidString[5];
	sprintf(pidString, "%d", pid);
	t_gdt * gdt = dictionary_remove(tablaProcesos,pidString);
	char * nroSegmentoString = intToString(segmento->nroSegmento);
	dictionary_put(gdt->tablaSegmentos,nroSegmentoString,segmento);
	free(nroSegmentoString);
	dictionary_put(tablaProcesos,pidString,gdt);
}

// Lógica de segmentacion pura
int ejecutarCargarEsquemaSegmentacion(t_package pkg, int socketSolicitud)
{
	//En el 1er paquete recibo la cantidad de paquetes a recibir y el tamaño de cada paquete
	char * buffer= pkg.data;
	int pid = copyIntFromBuffer(&buffer);
	int cantPaquetes = copyIntFromBuffer(&buffer);
	char * pathArchivo = copyStringFromBuffer(&buffer);
	free(buffer);

	if(tengoMemoriaDisponible(cantPaquetes) == 1){
		log_error_mutex(logger, "No hay memoria disponible para cargar el Escriptorio.");
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		if (enviar(socketSolicitud,FM9_DAM_MEMORIA_INSUFICIENTE,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al DAM de la memoria insuficiente.");
			exit_gracefully(-1);
		}
	}
	if (!existeProceso(pid))
		crearProceso(pid);

	char * pidString = intToString(pid);
	t_gdt * gdt = dictionary_get(tablaProcesos,pidString);
	free(pidString);
	t_segmento * segmento = reservarSegmento(cantPaquetes, gdt->tablaSegmentos, pathArchivo);
	// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
	if (segmento == NULL)
		return FM9_DAM_MEMORIA_INSUFICIENTE;
	actualizarTablaDeSegmentos(pid,segmento);
	// ACTUALIZO LA GLOBAL CON LAS LINEAS QUE UTILICE RECIEN
	// ESTO PARA QUE ESTA?????
	contLineasUsadas += cantPaquetes;

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

bool existeProceso(int pid)
{
	char * pidString = intToString(pid);
	t_gdt * gdt = dictionary_get(tablaProcesos,pidString);
	free(pidString);
	if (gdt != NULL)
		return true;
	return false;
}

void guardarLinea(int posicionMemoria, char * linea)
{
	memcpy(&storage+posicionMemoria, &linea, strlen(linea));
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

//logica de tabla de paginas invertida
int ejecutarCargarEsquemaTPI(t_package pkg,int socketSolicitud){
	//En el 1er paquete recibo la cantidad de paquetes a recibir y el tamaño de cada paquete
	char * buffer= pkg.data;
	int pid = copyIntFromBuffer(&buffer);
	int cantLineas = copyIntFromBuffer(&buffer);
	char * pathArchivo = copyStringFromBuffer(&buffer);
	free(buffer);

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

	reservarPaginasNecesarias(paginasNecesarias, pid, pathArchivo, cantLineas);
	pidBuscado = pid;
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

bool filtrarPorPid(t_pagina * pagina)
{
	if (pagina->pid == pidBuscado)
	{
		return true;
	}
	return false;
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

void actualizarTPI(t_pagina * pagina)
{
	list_add(tablaPaginasInvertida, pagina);
}

//logica de segmentacion paginada
int ejecutarCargarEsquemaSegPag(t_package pkg, int socketSolicitud){
	//En el 1er paquete recibo la cantidad de paquetes a recibir
	char * buffer= pkg.data;
	int pid = copyIntFromBuffer(&buffer);
	int cantLineas = copyIntFromBuffer(&buffer);
	char * pathArchivo = copyStringFromBuffer(&buffer);
	free(buffer);

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
	if (!existeProceso(pid))
		crearProceso(pid);

	//ACA HAY QUE FIJARSE EN LAS TABLAS SI TENGO UNA PAGINA ASOCIADA A UN SEGMENTE LIBRE PARA ALMACENAR LOS DATOS
	//EN CASO QUE SI RESERVAR DICHA PAGINA

	reservarPaginasNecesarias(paginasNecesarias, pid, pathArchivo, cantLineas);

	char * pidString = intToString(pid);
	t_gdt * gdt = dictionary_remove(tablaProcesos,pidString);
	free(pidString);
	reservarSegmentoSegmentacionPaginada(gdt, pid);
	// ESTO PARA QUE ESTA?????
	contLineasUsadas += cantLineas;

	//int pagina = reservarPagina();
	//actualizar las tablas
	//actualizarTablaSegmento(pid,segmento);
	//actualizarTablaPaginas(pid,pagina);

	gdt = dictionary_get(tablaProcesos,pidString);
	char * bufferGuardado = malloc(config->tamMaxLinea);
	int i = 0, offset = 0, lineasGuardadas = 0;
	int lineaLeida = 1;
	int cantidadSegmentos = dictionary_size(gdt->tablaSegmentos);
	while(i < cantidadSegmentos)
	{
		t_segmento * segmento = dictionary_get(gdt->tablaSegmentos, intToString(i));
		int paginaActual = 0;
		while (paginaActual < segmento->limite)
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
				t_pagina * pagina = list_get(gdt->tablaPaginas, paginaActual);
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
		return FM9_DAM_MEMORIA_INSUFICIENTE;
	}
	return EXIT_SUCCESS;
}

//------------------------------REALIZAR FLUSH SEGUN ESQUEMA ELEGIDO

int realizarFlushSegunEsquemaMemoria(t_package pkg, int socketSolicitud)
{
	switch (config->modoEjecucion)
	{
		case SEG:
			logicaFlush(pkg, socketSolicitud, SEG);
		    break;
		case TPI:
			logicaFlush(pkg,socketSolicitud, TPI);
			break;
		case SPA:
			logicaFlush(pkg,socketSolicitud, SPA);
			break;
		default:
			log_warning_mutex(logger, "No se especifico el esquema para el guardado de lineas");
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void logicaFlush(t_package pkg, int socketSolicitud, int code)
{
	t_datosFlush * infoFlush = malloc(sizeof(t_datosFlush));
	char * buffer = pkg.data;
	infoFlush->pid = copyIntFromBuffer(&buffer);
	infoFlush->path = copyStringFromBuffer(&buffer);
	infoFlush->transferSize = copyIntFromBuffer(&buffer);
	int resultado = 0;
	switch(code)
	{
		case SEG:
			resultado = flushSegmentacion(pkg, socketSolicitud, infoFlush);
			break;
		case TPI:
			resultado = flushTPI(pkg, socketSolicitud, infoFlush);
			break;
		case SPA:
			resultado = flushSegmentacionPaginada(pkg, socketSolicitud, infoFlush);
			break;
		default:
			log_warning_mutex(logger, "No se especifico el esquema para el guardado de lineas");
	}
	if (resultado != 0)
	{
		log_error_mutex(logger,"Error al cargar el Escriptorio recibido. Esquema: TPI");
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		if (enviar(socketSolicitud,FM9_DAM_MEMORIA_INSUFICIENTE,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al DAM del error en la carga del escriptorio");
			exit_gracefully(-1);
		}
	}
	else
	{
		log_info_mutex(logger, "Se cargó correctamente el Escriptorio recibido.");
	}
}

int flushSegmentacion(t_package pkg, int socketSolicitud, t_datosFlush * data)
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
		int cantidadLineas = 0;
		for (int i = 0; i < cantidadSegmentos; i++)
		{
			t_segmento * segmento = dictionary_get(gdt->tablaSegmentos, intToString(i));
			if (strcmp(segmento->archivo, data->path) == 0)
			{
				cantidadLineas += segmento->limite;
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
		for(int i = 0; i < cantidadSegmentos; i++)
		{
			t_segmento * segmento = dictionary_get(gdt->tablaSegmentos, intToString(i));
			int j = 0;
			if (strcmp(segmento->archivo, data->path) == 0)
			{
				while(j < segmento->limite)
				{
					char * linea = obtenerLinea(direccion(segmento->base, j));
					realizarFlush(linea, nroLinea, data->transferSize, socketSolicitud);
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

int flushSegmentacionPaginada(t_package pkg, int socketSolicitud, t_datosFlush * data)
{

	return EXIT_SUCCESS;
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
