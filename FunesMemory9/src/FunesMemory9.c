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
#include "consolaFM9.h"

int main(int argc, char** argv) {
	logger = log_create_mutex("FM9.log", "FM9", true, LOG_LEVEL_INFO);
	config = cargarConfiguracion(argv[1], FM9, logger->logger);
	inicializarContadores();
	storage = malloc(config->tamMemoria);
	pthread_create(&threadConsolaFM9, &tattr, (void *) manejarConsolaFM9, NULL);
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

//--------------------------------------CERRAR ARCHIVO SEGUN ESQUEMA ELEGIDO

int cerrarArchivoSegunEsquemaMemoria(t_package pkg, int socketSolicitud)
{
	switch(config->modoEjecucion)
	{
		case SEG:
				logicaCerrarArchivo(pkg, socketSolicitud, SEG);
				break;
		case TPI:
				logicaCerrarArchivo(pkg, socketSolicitud, TPI);
				break;
		case SPA:
				logicaCerrarArchivo(pkg, socketSolicitud, SPA);
				break;
		default:
				return EXIT_FAILURE;
		}
	return EXIT_SUCCESS;
}

void logicaCerrarArchivo(t_package pkg, int socketSolicitud, int code)
{
	t_infoCerrarArchivo * datosPaquete = guardarDatosPaqueteCierreArchivo(pkg);
	int resultado = 0;
	switch(code)
	{
		case SEG:
			resultado = cerrarArchivoSegmentacion(pkg, datosPaquete, socketSolicitud);
			break;
		case TPI:
			resultado = cerrarArchivoTPI(pkg, datosPaquete, socketSolicitud);
			break;
		case SPA:
			resultado = cerrarArchivoSegPag(pkg, datosPaquete, socketSolicitud);
			break;
		default:
			log_warning_mutex(logger, "No se especifico el esquema para cerrar un archivo.");
	}
	if (resultado != 0)
	{
		log_error_mutex(logger,"Error al cerrar el archivo indicado.");
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

//--------------------------------------GUARDAR DATOS EN MEMORIA SEGUN ESQUEMA ELEGIDO

int guardarLineaSegunEsquemaMemoria(t_package pkg, int socketSolicitud)
{
	switch (config->modoEjecucion){
	case SEG:
		logicaGuardarLinea(pkg, socketSolicitud, SEG);
	    break;
	case TPI:
		logicaGuardarLinea(pkg, socketSolicitud, TPI);
		break;
	case SPA:
		logicaGuardarLinea(pkg, socketSolicitud, SPA);
		break;
	default:
		log_warning_mutex(logger, "No se especifico el esquema para el guardado de lineas.");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void logicaGuardarLinea(t_package pkg, int socketSolicitud, int code)
{
	t_infoGuardadoLinea * datosPaquete = guardarDatosPaqueteGuardadoLinea(pkg);
	int resultado = 0;
	switch(code)
	{
		case SEG:
			resultado = ejecutarGuardarEsquemaSegmentacion(pkg, datosPaquete, socketSolicitud);
			break;
		case TPI:
			resultado = ejecutarGuardarEsquemaTPI(pkg, datosPaquete, socketSolicitud);
			break;
		case SPA:
			resultado = ejecutarGuardarEsquemaSegPag(pkg, datosPaquete, socketSolicitud);
			break;
		default:
			log_warning_mutex(logger, "No se especifico el esquema para el guardado de lineas.");
	}
	if (resultado != 0)
	{
		log_error_mutex(logger,"Error al guardar la línea especificada.");
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		if (enviar(socketSolicitud,FM9_CPU_ERROR_LINEA_GUARDADA,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar a la CPU del error en el guardado de la linea");
			exit_gracefully(-1);
		}
	}
	else
	{
		log_info_mutex(logger, "Se cargó correctamente el Escriptorio recibido.");
	}
	free(datosPaquete);
}

//------------------------------CARGAR ESCRIPTORIO EN MEMORIA SEGUN ESQUEMA ELEGIDO

int cargarEscriptorioSegunEsquemaMemoria(t_package pkg, int socketSolicitud)
{
	switch (config->modoEjecucion){
	case SEG:
		logicaCargarEscriptorio(pkg, socketSolicitud, SEG);
	    break;
	case TPI:
		logicaCargarEscriptorio(pkg, socketSolicitud, TPI);
		break;
	case SPA:
		logicaCargarEscriptorio(pkg, socketSolicitud, SPA);
		break;
	default:
		log_warning_mutex(logger, "No se especifico el esquema para la carga de escriptorio");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void logicaCargarEscriptorio(t_package pkg, int socketSolicitud, int code)
{
	t_infoCargaEscriptorio * datosPaquete = guardarDatosPaqueteCargaEscriptorio(pkg);
	int resultado = 0;
	switch(code)
	{
		case SEG:
			resultado = ejecutarCargarEsquemaSegmentacion(pkg, datosPaquete, socketSolicitud);
			break;
		case TPI:
			resultado = ejecutarCargarEsquemaTPI(pkg, datosPaquete, socketSolicitud);
			break;
		case SPA:
			resultado = ejecutarCargarEsquemaSegPag(pkg, datosPaquete, socketSolicitud);
			break;
		default:
			log_warning_mutex(logger, "No se especifico el esquema para la carga de un escriptorio.");
	}
	if (resultado != 0)
	{
		log_error_mutex(logger,"Error al cargar el Escriptorio recibido.");
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		if (enviar(socketSolicitud,FM9_DAM_ERROR_CARGA_ESCRIPTORIO,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al DAM del error en la carga del escriptorio");
			exit_gracefully(-1);
		}
	}
	else
	{
		log_info_mutex(logger, "Se cargó correctamente el Escriptorio recibido.");
	}
	free(datosPaquete);
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
	t_datosFlush * infoFlush = guardarDatosPaqueteFlush(pkg);
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
			log_warning_mutex(logger, "No se especifico el esquema para el flush");
	}
	if (resultado != 0)
	{
		log_error_mutex(logger,"Error al realizar el flush.");
		// TODO: ESCIRIBIR EN EL LOG EL BIT VECTOR PARA COMPROBAR QUÉ PÁGINAS HAY LIBRES
		if (enviar(socketSolicitud,FM9_DAM_ERROR_FLUSH,pkg.data,pkg.size,logger->logger))
		{
			log_error_mutex(logger, "Error al avisar al DAM del error en el flush");
			exit_gracefully(-1);
		}
	}
	else
	{
		log_info_mutex(logger, "Se ha realizado correctamente el flush.");
	}
	free(infoFlush);
}
