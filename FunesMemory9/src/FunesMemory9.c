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
	tablaProcesos = dictionary_create();
	inicializarBitmapLineas();
}

void exit_gracefully(int error)
{
	liberarRecursos();
	exit(error);
}

void inicializarBitmapLineas()
{
	int tamBitarray = cantLineas/8;
	if(cantLineas % 8 != 0){
		tamBitarray++;
	}
	char* data=malloc(tamBitarray);
	estadoLineas = bitarray_create_with_mode(data,tamBitarray,LSB_FIRST); // if create falla error.

	int bit;
	bit = 0;
	while(bit <= cantLineas){
		bitarray_clean_bit(estadoLineas, bit);
		bit ++;
	}
}

t_segmento * nuevoSegmento(int nroSegmento, int cantidadLineas, char * archivo, int base)
{
	t_segmento * segmento;
	segmento->nroSegmento = nroSegmento;
	segmento->limite = cantidadLineas;
	segmento->archivo = archivo;
	segmento->base = base;
	return segmento;
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
					if (acceptConnection(socketListen, &nuevoFd, FM9_HSK, &handshake, logger->logger)) {
						log_error_mutex(logger, "No se acepto la nueva conexion solicitada");
					} else {
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

        case DAM_FM9_GUARDARLINEA:
        	if(guardarLineaSegunEsquemaMemoria(pkg,socketFD)){
				log_error_mutex(logger,"Error al guardar la data recibida en memoria");
			}
			break;

        case DAM_FM9_RETORNARlINEA:
        	if(retornarLineaSolicitada(pkg,socketFD)){
				log_error_mutex(logger,"Error al retornar la memoria solicitada");
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
//--------------------------------------GUARDAR DATOS EN MEMORIA SEGUN ESQUEMA ELEGIDO

int guardarLineaSegunEsquemaMemoria(t_package pkg, int socketSolicitud){
	switch (config->modoEjecucion){
	case SEG:

		if(ejecutarGuardarEsquemaSegmentacion(pkg)){
			log_error_mutex(logger,"Error al guardar la linea recibida en Memoria. Esquema: SEG");
			//ENVIAR ERROR AL DMA (socketSolicitud)
		}else{
			log_info_mutex(logger, "Se cargó correctamente la linea recibida en Memoria");
		}
	    break;

	case TPI:
		if(ejecutarGuardarEsquemaTPI(pkg)){
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


int ejecutarGuardarEsquemaSegmentacion(t_package pkg){
	//logica de segmentacion pura


	return EXIT_SUCCESS;

}

int ejecutarGuardarEsquemaTPI(t_package pkg){
	//logica de tabla de paginas invertida
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
		int error = ejecutarCargarEsquemaSegmentacion(pkg,socketSolicitud);
		if (error != 0){
			log_error_mutex(logger,"Error al cargar el Escriptorio recibido. Esquema: SEG");
			if (enviar(socketSolicitud,FM9_DAM_MEMORIA_INSUFICIENTE,pkg.data,pkg.size,logger->logger))
			{
				log_error_mutex(logger, "Error al avisar al DAM del error en");
				exit_gracefully(-1);
			}
		}else{
			log_info_mutex(logger, "Se cargó correctamente el Escriptorio recibido.");
		}
	    break;

	case TPI:
	    ejecutarCargarEsquemaTPI(pkg,socketSolicitud);
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

int tengoMemoriaDisponible(int cantidadACargarBytes)
{
	int cantACargarEnLineas = cantidadACargarBytes / config->tamMaxLinea;
	int memoriaDisponible = cantLineas - contLineasUsadas;

	if(memoriaDisponible > cantACargarEnLineas){
		//Tengo espacio disponible.
		return EXIT_SUCCESS;
	}else{
		//No tengo espacio disponible.
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


t_segmento * reservarSegmento(int lineasEsperadas, t_dictionary * tablaSegmentos, char * archivo)
{
	t_segmento * segmento;
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
	// TERMINAR ESTO, ES IMPORTANTE PARA YA GUARDAR UN SEGMENTO
	if (lineasLibresContiguas == lineasEsperadas)
	{
		segmento->base = base;
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
void crearProceso(int pid)
{
	t_gdt * gdt;
	gdt->tablaSegmentos = dictionary_create();
	dictionary_put(tablaProcesos,pid,gdt);
}

void actualizarTablaDeSegmentos(int pid, t_segmento * segmento)
{
	t_gdt * gdt = dictionary_get(tablaProcesos,pid);
	dictionary_put(gdt->tablaSegmentos,segmento->nroSegmento,segmento);
	dictionary_put(tablaProcesos,pid,gdt);
}

// Lógica de segmentacion pura
int ejecutarCargarEsquemaSegmentacion(t_package pkg, int socketSolicitud)
{
	//En el 1er paquete recibo la cantidad de paquetes a recibir y el tamaño de cada paquete
	char * buffer= pkg.data;
	int pid = copyIntFromBuffer(&buffer);
	int cantPaquetes = copyIntFromBuffer(&buffer);
	int tamanioPaquetes = copyIntFromBuffer(&buffer);
	free(buffer);
	int cantidadACargar = cantPaquetes * tamanioPaquetes;

	//CON EL TAMAÑO PUEDO CALCULAR CUANTOS PAQUETES PUEDEN ENTRAR EN 1 LINEA DE MEMORIA
	//Calcular la parte entera
	int paquetesXLinea = config->tamMaxLinea / tamanioPaquetes;

	char * bufferConcatenado = malloc(config->tamMaxLinea);

	for(int i = 0; i < cantPaquetes; i++){
		t_package paquete;
		if(recibir(socketSolicitud,&paquete,logger->logger)){
			log_error_mutex(logger, "Error al recibir el paquete N°: %d",i);
			//enviarErrorAlDam();
		}else{

			if((i+1) < paquetesXLinea){
				//si no supere los paquetes por linea lo sumo al bufferConcatenado
				copyStringToBuffer(&bufferConcatenado,paquete.data);
			}else{
				//si llego a los paquetes máximos -> Guardo la linea
				copyStringToBuffer(&bufferConcatenado,paquete.data);

				//TODO GUARDAR LINEA EN SEGMENTO
				//guardarlinea(bufferConcatenado,segmento);

				//Descarto el buffer y lo creo de nuevo para la nueva linea
				free(bufferConcatenado);
				bufferConcatenado = malloc(config->tamMaxLinea);
			}
		}
	}
	if(tengoMemoriaDisponible(cantidadACargar) == 1){
		// Avisarle al socket que no hay memoria disponible
		log_error_mutex(logger, "No hay memoria disponible para cargar el Escriptorio.");
		if (enviar(socketSolicitud,FM9_DAM_MEMORIA_INSUFICIENTE,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al DAM de la memoria insuficiente.");
			exit_gracefully(-1);
		}
	}
	crearProceso(pid);

	// ACA HAY QUE FIJARSE EN EL STORAGE SI TENGO UN SEGMENTO CONTIGUO PARA ALMACENAR LOS DATOS
	// EN CASO QUE SI RESERVAR UN SEGMENTO
	// TODO: ACA HAY QUE MANDARLE LAS LINEAS QUE TIENE EL ARCHIVO EN EL PRIMER PARAMETRO
	// TODO: REEMPLAZAR EL TEXTO DEL ARCHIVO POR LO QUE RECIBA DEL DAM.
	t_gdt * gdt = dictionary_get(tablaProcesos,pid);
	t_segmento * segmento = reservarSegmento(cantidadACargar, gdt->tablaSegmentos, "/PuntoMontaje/archivo.txt");
	if (segmento == NULL)
		return FM9_DAM_MEMORIA_INSUFICIENTE;
	actualizarTablaDeSegmentos(pid,segmento);
	// TODO PENSARLO BIEN, QUE GUARDE LA LINEA Y SE VAYA CORRIENDO DE A TAMAÑO MAXIMO DE LINEA
	char* token;
	int i = 0;
	//TODO GUARDAR LINEA EN SEGMENTO
	while ((token = strsep(&bufferConcatenado, "\n")) != NULL)
	{
		// VERIFICAR SI PUEDO USAR LA FUNCION DIRECCION
		guardarLinea((segmento->base+i)*config->tamMaxLinea, token);
	}

	free(bufferConcatenado);
	//ENVIAR MSJ DE EXITO A DAM
	if (enviar(socketSolicitud,FM9_DAM_ESCRIPTORIO_CARGADO,pkg.data,pkg.size,logger->logger))
	{
		log_error_mutex(logger, "Error al avisar al DAM el aviso de que se ha cargado el Escriptorio.");
		exit_gracefully(-1);
	}
	return EXIT_SUCCESS;

}

void guardarLinea(int posicionMemoria, char * linea)
{
	switch(config->modoEjecucion)
	{
		case SEG:
			guardarLineaSegmentacionSimple(posicionMemoria, linea);
			break;
		default:
			break;
	}
}

void guardarLineaSegmentacionSimple(int posicion, char * linea)
{
	memcpy(&storage+posicion, &linea, strlen(linea));
}

int direccion(int base, int desplazamiento)
{
	// Usar la base del Storage, y desde la base que llega como parametro, sumar el desplazamiento.
	int direccion = storage + base + desplazamiento;
	return direccion;
}

void ejecutarCargarEsquemaTPI(t_package pkg,int socketSolicitud){
	//logica de tabla de paginas invertida
	//En el 1er paquete recibo la cantidad de paquetes a recibir y el tamaño de cada paquete
	char * buffer= pkg.data;
	int pid = copyIntFromBuffer(&buffer);
	int cantPaquetes = copyIntFromBuffer(&buffer);
	int tamanioPaquetes = copyIntFromBuffer(&buffer);
	free(buffer);

	int cantidadACargar = cantPaquetes * tamanioPaquetes;

	if(tengoMemoriaDisponible(cantidadACargar) == 1){
		//fallo
		//avisarle al socket que no hay memoria disponible
		//enviarMsjErrorAlDam()
	}

	//ACA HAY QUE FIJARSE EN LA TABLA INVERTIDA SI TENGO UNA PAGINA SIN PID ASOCIADO PARA ALMACENAR LOS DATOS
	//EN CASO QUE SI RESERVAR DICHA PAGINA
	//int pagina = reservarPagina();
	//actualizar tabla invertida
	//actualizarTablaInvertida(pid,pagina);

	//CON EL TAMAÑO PUEDO CALCULAR CUANTOS PAQUETES PUEDEN ENTRAR EN 1 LINEA DE MEMORIA
	//Calcular la parte entera
	int paquetesXLinea = config->tamMaxLinea / tamanioPaquetes;

	char * bufferConcatenado = malloc(config->tamMaxLinea);

	for(int i = 0; i < cantPaquetes; i++){
		t_package paquete;
		if(recibir(socketSolicitud,&paquete,logger->logger)){
			log_error_mutex(logger, "Error al recibir el paquete N°: %d",i);
			//enviarErrorAlDam();
		}else{

			if((i+1) < paquetesXLinea){
				//si no supere los paquetes por linea lo sumo al bufferConcatenado
				copyStringToBuffer(&bufferConcatenado,paquete.data);
			}else{
				//si llego a los paquetes máximos -> Guardo la linea
				copyStringToBuffer(&bufferConcatenado,paquete.data);

				//TODO GUARDAR LINEA EN PAGINA
				//guardarlineaEsquemaTPI(bufferConcatenado,segmento);

				//Descarto el buffer y lo creo de nuevo para la nueva linea
				free(bufferConcatenado);
				bufferConcatenado = malloc(config->tamMaxLinea);
			}
		}
	}

	//TODO GUARDAR LINEA EN PAGINA
	//guardarlinea(bufferConcatenado,segmento);
	free(bufferConcatenado);


	//Enviar msj de confirmacion al dam
}

void ejecutarCargarEsquemaSegPag(t_package pkg, int socketSolicitud){
	//logica de segmentacion paginada
	//En el 1er paquete recibo la cantidad de paquetes a recibir y el tamaño de cada paquete
	char * buffer= pkg.data;
	int pid = copyIntFromBuffer(&buffer);
	int cantPaquetes = copyIntFromBuffer(&buffer);
	int tamanioPaquetes = copyIntFromBuffer(&buffer);
	free(buffer);

	int cantidadACargar = cantPaquetes * tamanioPaquetes;

	if(tengoMemoriaDisponible(cantidadACargar) == 1){
		//fallo
		//avisarle al socket que no hay memoria disponible
		//enviarErrorAlDam();
	}

	//ACA HAY QUE FIJARSE EN LAS TABLAS SI TENGO UNA PAGINA ASOCIADA A UN SEGMENTE LIBRE PARA ALMACENAR LOS DATOS
	//EN CASO QUE SI RESERVAR DICHA PAGINA
	//int pagina = reservarPagina();
	//actualizar las tablas
	//actualizarTablaSegmento(pid,segmento);
	//actualizarTablaPaginas(pid,pagina);

	//CON EL TAMAÑO PUEDO CALCULAR CUANTOS PAQUETES PUEDEN ENTRAR EN 1 LINEA DE MEMORIA
	//Calcular la parte entera
	int paquetesXLinea = config->tamMaxLinea / tamanioPaquetes;

	char * bufferConcatenado = malloc(config->tamMaxLinea);

	for(int i = 0; i < cantPaquetes; i++){
		t_package paquete;
		if(recibir(socketSolicitud,&paquete,logger->logger)){
			log_error_mutex(logger, "Error al recibir el paquete N°: %d",i);
			//enviarErrorAlDam();
		}else{

			if((i+1) < paquetesXLinea){
				//si no supere los paquetes por linea lo sumo al bufferConcatenado
				copyStringToBuffer(&bufferConcatenado,paquete.data);
			}else{
				//si llego a los paquetes máximos -> Guardo la linea
				copyStringToBuffer(&bufferConcatenado,paquete.data);

				//TODO GUARDAR LINEA EN PAGINA
				//guardarlineaEsquemaSPA(bufferConcatenado,segmento);

				//Descarto el buffer y lo creo de nuevo para la nueva linea
				free(bufferConcatenado);
				bufferConcatenado = malloc(config->tamMaxLinea);
			}
		}
	}

	//TODO GUARDAR LINEA EN PAGINA
	//guardarlineaEsquemaSPA(bufferConcatenado,segmento);
	free(bufferConcatenado);


	//Enviar msj confirmacion al dam
}

//--------------------------------------------------RETORNAR DATOS DE MEMORIA

int retornarLineaSolicitada(t_package pkg, int socketSolicitud){

	//logica para retornar una linea pedida
	return EXIT_SUCCESS;
}

