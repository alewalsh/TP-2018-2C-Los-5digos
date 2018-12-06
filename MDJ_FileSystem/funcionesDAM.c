#include "FileSystem.h"

void consoleExit() {
	setExit();
}

void manejarDAM(t_package pkg)
{
	sleep(configuracion->retardo/1000);
	int opcion = pkg.code;
	switch (opcion)
	{
		case DAM_MDJ_CONFIRMAR_EXISTENCIA_ARCHIVO:
			{
				//validar archivo ++ [Path]
				char *bufferValidar = pkg.data;
				char * pathValidarSinPM = copyStringFromBuffer(&bufferValidar);

				char *pathValidar = string_new();
				char * pathArchivosVal = "Archivos";
				string_append(&pathValidar, configuracion->puntoMontaje);
				string_append(&pathValidar, pathArchivosVal);
				string_append(&pathValidar, pathValidarSinPM);

				int statusValidar = validarArchivo(pathValidar);
				if (statusValidar) {
					log_info_mutex(loggerAtencionDAM, "Archivo: %s OK.", pathValidar);
					metadataArchivo * metadataArchivoValidar = leerMetadata(pathValidar);
					int tamanio = metadataArchivoValidar->tamanio;
					int sizeEnvioValidacion = sizeof(int);
					char* bufferEnvioValidacion = malloc(sizeEnvioValidacion);
					char * p = bufferEnvioValidacion;
					copyIntToBuffer(&p, tamanio);
					//envia OK y el tamanio del archivo a validar. No utilizo TS porque seguro entra (?
					if(enviar(socketDAM, DAM_MDJ_OK, bufferEnvioValidacion, sizeEnvioValidacion, loggerAtencionDAM->logger)){
						log_error_mutex(loggerAtencionDAM, "Error al enviar validacion de archivo al DAM.");
					}
//					free(metadataArchivoValidar->bloques);
//					free(metadataArchivoValidar);
//					free(bufferEnvioValidacion);
				} else {
					//manda fail
					int size = sizeof(int);
					int respuesta = -1;
					char * buffer = malloc(size);
					char * p = buffer;
					copyIntToBuffer(&p, respuesta);
					if(enviar(socketDAM, DAM_MDJ_FAIL, buffer, size,loggerAtencionDAM->logger)){
						log_error_mutex(loggerAtencionDAM, "Error al enviar path inexistente al DAM.");
					}
					log_error_mutex(loggerAtencionDAM, "Error. Path: %s inexistente.", pathValidar);
				}
				free(pkg.data);
				free(pathValidar);
				break;
			}
		case DAM_MDJ_CREAR_ARCHIVO: //crear archivo ++ [Path, N Cantidad de /n's]
			{

				char *bufferCrear = pkg.data;
				char * pathArchivoNuevoSinPM = copyStringFromBuffer(&bufferCrear);
				int cantidad_Ns = copyIntFromBuffer(&bufferCrear);

				char *pathArchivoNuevo = string_new();

				string_append(&pathArchivoNuevo, configuracion->puntoMontaje);
				char * pathArchivosCreo = "Archivos";
				string_append(&pathArchivoNuevo, pathArchivosCreo);
				string_append(&pathArchivoNuevo, pathArchivoNuevoSinPM);

				crearPathArchivo(pathArchivoNuevo);

				//creo String de /n's a ser escrito
				char *NcantidadDeNs = string_new();
				int i;
				i=0;
				while(i < cantidad_Ns){
					string_append(&NcantidadDeNs, "\n");
					i++;
				}

				if(escribirStringEnArchivo(pathArchivoNuevo, NcantidadDeNs)){
					if(enviar(socketDAM, DAM_MDJ_OK, NULL,0, loggerAtencionDAM->logger)){
						log_error_mutex(loggerAtencionDAM, "Error al enviar OK de crear archivo al DAM.");
					}
				}else{
					if(enviar(socketDAM, DAM_MDJ_FAIL, NULL, 0,loggerAtencionDAM->logger)){
						log_error_mutex(loggerAtencionDAM, "Error al enviar FAIL por crear archivo al DAM.");
					}
				}

				free(bufferCrear);
				free(pathArchivoNuevo);

				break;
			}
		case DAM_MDJ_CARGAR_ESCRIPTORIO: //obtener datos ++ [Path, Offset, Size]
			{
				char *bufferObtenerDatos = pkg.data;
				char * pathArchivoALeerSinPM = copyStringFromBuffer(&bufferObtenerDatos);
				int offset = copyIntFromBuffer(&bufferObtenerDatos);
				int size = copyIntFromBuffer(&bufferObtenerDatos);

				char *pathArchivoALeer = string_new();
				char * pathArchivosCarga = "Archivos";
				string_append(&pathArchivoALeer, configuracion->puntoMontaje);
				string_append(&pathArchivoALeer, pathArchivosCarga);
				string_append(&pathArchivoALeer, pathArchivoALeerSinPM);

				int statusObtener;
				statusObtener = validarArchivo(pathArchivoALeer);

				if (statusObtener) {
					//obtengo datos
					char *datosArchivo = obtenerDatos(pathArchivoALeer,offset,size);
					log_info_mutex(loggerAtencionDAM, "Datos obtenidos de: %s son: %s", pathArchivoALeer, datosArchivo);

					//envio datosArchivos en paquetes de tamanio del transferSize.
					enviarStringDAMporTRansferSize(datosArchivo);

					free(datosArchivo);
				} else {
					//manda fail
					log_error_mutex(loggerAtencionDAM, "Error no se pudo leer de: &s", pathArchivoALeer);
					enviar(socketDAM, DAM_MDJ_FAIL, NULL, 0,loggerAtencionDAM->logger);
				}
				free(pathArchivoALeer);
				break;
			}
		case DAM_MDJ_HACER_FLUSH: //guardar datos ++ [Path, Offset, Size, Buffer]
			{

				char *bufferGuardarDatos = pkg.data;
				char * pathArchivoAModificarSinPM = copyStringFromBuffer(&bufferGuardarDatos);
				int offsetGuardar = copyIntFromBuffer(&bufferGuardarDatos);
				int sizeGuardar = copyIntFromBuffer(&bufferGuardarDatos);

				//leo por TS cada paquete y lo concateno. sizeGuardar es el peso a recivir.
				char * bufferGuardar = rebirStringDAMporTRansferSize(sizeGuardar);

				log_info_mutex(loggerAtencionDAM, "Datos recibidos a ser guardados son: %s", bufferGuardar);

				char *pathArchivoAModificar = string_new();
				char * pathArchivosFlush = "Archivos";
				string_append(&pathArchivoAModificar, configuracion->puntoMontaje);
				string_append(&pathArchivoAModificar, pathArchivosFlush);
				string_append(&pathArchivoAModificar, pathArchivoAModificarSinPM);

				int statusGuardar;
				statusGuardar = validarArchivo(pathArchivoAModificar);
				if (statusGuardar) {
					//obtengo datos
					char *datosArchivo = obtenerDatos(pathArchivoAModificar,offsetGuardar,sizeGuardar);

					//moficio dichos datos
					char *principio = string_substring_until(datosArchivo, offsetGuardar);
					char *final = string_substring_from(datosArchivo, offsetGuardar);

					char *stringModificado = string_new();
					string_append(&stringModificado, principio);
					string_append(&stringModificado, bufferGuardar);
					string_append(&stringModificado, final);

					//borro archivo anterior.
					borrarArchivo(pathArchivoAModificar);
					//creo nuevo archivo
					escribirStringEnArchivo(pathArchivoAModificar, stringModificado);

					log_info_mutex(loggerAtencionDAM, "Datos guardados de: %s\n Antiguos: %s\n Nuevos: %s\n", pathArchivoAModificar, datosArchivo, stringModificado);

				} else {
					log_error_mutex(loggerAtencionDAM, "Error no se pudo leer de: %s", pathArchivoAModificar);
					enviar(socketDAM, DAM_MDJ_FAIL, NULL, 0,loggerAtencionDAM->logger);
				}
				free(pathArchivoAModificar);
				free(bufferGuardar);
				break;
			}
		case DAM_MDJ_BORRAR_ARCHIVO: //borrar archivo ++ [Path]
			{

				char *bufferBorrar = pkg.data;
				char * pathArchivoAEliminarSinPM = copyStringFromBuffer(&bufferBorrar);

				char *pathArchivoAEliminar = string_new();
				char * pathArchivosBorra = "Archivos";
				string_append(&pathArchivoAEliminar, configuracion->puntoMontaje);
				string_append(&pathArchivoAEliminar, pathArchivosBorra);
				string_append(&pathArchivoAEliminar, pathArchivoAEliminarSinPM);

				borrarArchivo(pathArchivoAEliminar);

				int statusEliminiar;
				statusEliminiar = validarArchivo(pathArchivoAEliminar);
				if (statusEliminiar) {
					log_error_mutex(loggerAtencionDAM, "Error no se pudo borrar: %s", pathArchivoAEliminar);
				}else{
					log_error_mutex(loggerAtencionDAM, "Archivo: %s Borrado\n", pathArchivoAEliminar);
				}

				free(pathArchivoAEliminar);
			}
			break;
		case 6:
			break;
		default:
			log_warning_mutex(loggerAtencionDAM, "Ojo, estás recibiendo una operacion que no esperabas.");
			break;
	}
}

//void responderDAM(t_package pkg)
//{
//	sleep(configuracion->retardo/1000);
//	int opcion = pkg.code;
//	switch (opcion)
//	{
//		case DAM_MDJ_CONFIRMAR_EXISTENCIA_ARCHIVO:
//			{
//				//validar archivo ++ [Path]
//				char *bufferValidar = pkg.data;
//				char * pathValidarSinPM = copyStringFromBuffer(&bufferValidar);
//
//				char *pathValidar = string_new();
//				char * pathArchivosVal = "Archivos";
//				string_append(&pathValidar, configuracion->puntoMontaje);
//				string_append(&pathValidar, pathArchivosVal);
//				string_append(&pathValidar, pathValidarSinPM);
//
//				int statusValidar = validarArchivo(pathValidar);
//				if (statusValidar) {
//					log_info_mutex(loggerAtencionDAM, "Archivo: %s OK.", pathValidar);
//					metadataArchivo * metadataArchivoValidar = leerMetadata(pathValidar);
//					int tamanio = metadataArchivoValidar->tamanio;
//					int sizeEnvioValidacion = sizeof(int);
//					char* bufferEnvioValidacion = malloc(sizeEnvioValidacion);
//					char * p = bufferEnvioValidacion;
//					copyIntToBuffer(&p, tamanio);
//					//envia OK y el tamanio del archivo a validar. No utilizo TS porque seguro entra (?
//					if(enviar(socketDAM, DAM_MDJ_OK, bufferEnvioValidacion, sizeEnvioValidacion, loggerAtencionDAM->logger)){
//						log_error_mutex(loggerAtencionDAM, "Error al enviar validacion de archivo al DAM.");
//					}
////					free(metadataArchivoValidar->bloques);
////					free(metadataArchivoValidar);
////					free(bufferEnvioValidacion);
//				} else {
//					//manda fail
//					int size = sizeof(int);
//					int respuesta = -1;
//					char * buffer = malloc(size);
//					char * p = buffer;
//					copyIntToBuffer(&p, respuesta);
//					if(enviar(socketDAM, DAM_MDJ_FAIL, buffer, size,loggerAtencionDAM->logger)){
//						log_error_mutex(loggerAtencionDAM, "Error al enviar path inexistente al DAM.");
//					}
//					log_error_mutex(loggerAtencionDAM, "Error. Path: %s inexistente.", pathValidar);
//				}
//				free(pkg.data);
//				free(pathValidar);
//				break;
//			}
//		case DAM_MDJ_CREAR_ARCHIVO: //crear archivo ++ [Path, N Cantidad de /n's]
//			{
//
//				char *bufferCrear = pkg.data;
//				char * pathArchivoNuevoSinPM = copyStringFromBuffer(&bufferCrear);
//				int cantidad_Ns = copyIntFromBuffer(&bufferCrear);
//
//				char *pathArchivoNuevo = string_new();
//
//				string_append(&pathArchivoNuevo, configuracion->puntoMontaje);
//				char * pathArchivosCreo = "Archivos";
//				string_append(&pathArchivoNuevo, pathArchivosCreo);
//				string_append(&pathArchivoNuevo, pathArchivoNuevoSinPM);
//
//				crearPathArchivo(pathArchivoNuevo);
//
//				//creo String de /n's a ser escrito
//				char *NcantidadDeNs = string_new();
//				int i;
//				i=0;
//				while(i < cantidad_Ns){
//					string_append(&NcantidadDeNs, "\n");
//					i++;
//				}
//
//				if(escribirStringEnArchivo(pathArchivoNuevo, NcantidadDeNs)){
//					if(enviar(socketDAM, DAM_MDJ_OK, NULL,0, loggerAtencionDAM->logger)){
//						log_error_mutex(loggerAtencionDAM, "Error al enviar OK de crear archivo al DAM.");
//					}
//				}else{
//					if(enviar(socketDAM, DAM_MDJ_FAIL, NULL, 0,loggerAtencionDAM->logger)){
//						log_error_mutex(loggerAtencionDAM, "Error al enviar FAIL por crear archivo al DAM.");
//					}
//				}
//
//				free(bufferCrear);
//				free(pathArchivoNuevo);
//
//				break;
//			}
//		case DAM_MDJ_CARGAR_ESCRIPTORIO: //obtener datos ++ [Path, Offset, Size]
//			{
//				char *bufferObtenerDatos = pkg.data;
//				char * pathArchivoALeerSinPM = copyStringFromBuffer(&bufferObtenerDatos);
//				int offset = copyIntFromBuffer(&bufferObtenerDatos);
//				int size = copyIntFromBuffer(&bufferObtenerDatos);
//
//				char *pathArchivoALeer = string_new();
//				char * pathArchivosCarga = "Archivos";
//				string_append(&pathArchivoALeer, configuracion->puntoMontaje);
//				string_append(&pathArchivoALeer, pathArchivosCarga);
//				string_append(&pathArchivoALeer, pathArchivoALeerSinPM);
//
//				int statusObtener;
//				statusObtener = validarArchivo(pathArchivoALeer);
//
//				if (statusObtener) {
//					//obtengo datos
//					char *datosArchivo = obtenerDatos(pathArchivoALeer,offset,size);
//					log_info_mutex(loggerAtencionDAM, "Datos obtenidos de: %s son: %s", pathArchivoALeer, datosArchivo);
//
//					//envio datosArchivos en paquetes de tamanio del transferSize.
//					enviarStringDAMporTRansferSize(datosArchivo);
//
//					free(datosArchivo);
//				} else {
//					//manda fail
//					log_error_mutex(loggerAtencionDAM, "Error no se pudo leer de: &s", pathArchivoALeer);
//					enviar(socketDAM, DAM_MDJ_FAIL, NULL, 0,loggerAtencionDAM->logger);
//				}
//				free(pathArchivoALeer);
//				break;
//			}
//		case DAM_MDJ_HACER_FLUSH: //guardar datos ++ [Path, Offset, Size, Buffer]
//			{
//
//				char *bufferGuardarDatos = pkg.data;
//				char * pathArchivoAModificarSinPM = copyStringFromBuffer(&bufferGuardarDatos);
//				int offsetGuardar = copyIntFromBuffer(&bufferGuardarDatos);
//				int sizeGuardar = copyIntFromBuffer(&bufferGuardarDatos);
//
//				//leo por TS cada paquete y lo concateno. sizeGuardar es el peso a recivir.
//				char * bufferGuardar = rebirStringDAMporTRansferSize(sizeGuardar);
//
//				log_info_mutex(loggerAtencionDAM, "Datos recibidos a ser guardados son: %s", bufferGuardar);
//
//				char *pathArchivoAModificar = string_new();
//				char * pathArchivosFlush = "Archivos";
//				string_append(&pathArchivoAModificar, configuracion->puntoMontaje);
//				string_append(&pathArchivoAModificar, pathArchivosFlush);
//				string_append(&pathArchivoAModificar, pathArchivoAModificarSinPM);
//
//				int statusGuardar;
//				statusGuardar = validarArchivo(pathArchivoAModificar);
//				if (statusGuardar) {
//					//obtengo datos
//					char *datosArchivo = obtenerDatos(pathArchivoAModificar,offsetGuardar,sizeGuardar);
//
//					//moficio dichos datos
//					char *principio = string_substring_until(datosArchivo, offsetGuardar);
//					char *final = string_substring_from(datosArchivo, offsetGuardar);
//
//					char *stringModificado = string_new();
//					string_append(&stringModificado, principio);
//					string_append(&stringModificado, bufferGuardar);
//					string_append(&stringModificado, final);
//
//					//borro archivo anterior.
//					borrarArchivo(pathArchivoAModificar);
//					//creo nuevo archivo
//					escribirStringEnArchivo(pathArchivoAModificar, stringModificado);
//
//					log_info_mutex(loggerAtencionDAM, "Datos guardados de: %s\n Antiguos: %s\n Nuevos: %s\n", pathArchivoAModificar, datosArchivo, stringModificado);
//
//				} else {
//					log_error_mutex(loggerAtencionDAM, "Error no se pudo leer de: %s", pathArchivoAModificar);
//					enviar(socketDAM, DAM_MDJ_FAIL, NULL, 0,loggerAtencionDAM->logger);
//				}
//				free(pathArchivoAModificar);
//				free(bufferGuardar);
//				break;
//			}
//		case DAM_MDJ_BORRAR_ARCHIVO: //borrar archivo ++ [Path]
//			{
//
//				char *bufferBorrar = pkg.data;
//				char * pathArchivoAEliminarSinPM = copyStringFromBuffer(&bufferBorrar);
//
//				char *pathArchivoAEliminar = string_new();
//				char * pathArchivosBorra = "Archivos";
//				string_append(&pathArchivoAEliminar, configuracion->puntoMontaje);
//				string_append(&pathArchivoAEliminar, pathArchivosBorra);
//				string_append(&pathArchivoAEliminar, pathArchivoAEliminarSinPM);
//
//				borrarArchivo(pathArchivoAEliminar);
//
//				int statusEliminiar;
//				statusEliminiar = validarArchivo(pathArchivoAEliminar);
//				if (statusEliminiar) {
//					log_error_mutex(loggerAtencionDAM, "Error no se pudo borrar: %s", pathArchivoAEliminar);
//				}else{
//					log_error_mutex(loggerAtencionDAM, "Archivo: %s Borrado\n", pathArchivoAEliminar);
//				}
//
//				free(pathArchivoAEliminar);
//			}
//			break;
//		case 6:
//			break;
//		default:
//			log_warning_mutex(loggerAtencionDAM, "Ojo, estás recibiendo una operacion que no esperabas.");
//			break;
//	}
//}

int validarArchivo(char *path) {
	struct stat buffer;
	int status;
	status = stat(path, &buffer);
	if (status < 0) {
		return 0;
	} else {
		return 1;
	}
}

int getExit() {
	return shouldExit;
}

void setExit() {
	pthread_mutex_lock(&mutexExit);
	shouldExit = 1;
	pthread_mutex_unlock(&mutexExit);
}

void escribirMetadata(char *path, struct metadataArchivo *metadata) {

	FILE *archivoNuevo;
	archivoNuevo = fopen(path, "wb");

	char *metadataNewArchivo = string_new();
	string_append(&metadataNewArchivo,"TAMANIO_ARCHIVO=");
	string_append(&metadataNewArchivo,string_itoa(metadata->tamanio));
	string_append(&metadataNewArchivo,"\n");

	string_append(&metadataNewArchivo,"BLOQUES_ARCHIVOS=");
	string_append(&metadataNewArchivo,metadata->bloques);
	string_append(&metadataNewArchivo,"\0");

	char *metadataParaFwrite = string_from_format(metadataNewArchivo);
	fwrite(metadataParaFwrite , 1 , strlen(metadataParaFwrite) , archivoNuevo );
	fclose(archivoNuevo);
	free(metadataNewArchivo);
	free(metadataParaFwrite);
}

int escribirStringEnArchivo(char *pathArchivo, char *stringAEscribir) {

	int tamanioString;
	tamanioString = strlen(stringAEscribir) * sizeof(char);

	struct metadataArchivo *metadataNewArchivo = malloc(sizeof(struct metadataArchivo));
	metadataNewArchivo->tamanio = tamanioString;

	//calculo cuantos bloques necesito

	int cantidadTotalBloques = tamanioString / configuracion->tam_bloq;

	if(tamanioString % configuracion->tam_bloq != 0){
			cantidadTotalBloques++;
	}

	//WAIT de bitarray
	pthread_mutex_lock(&semaforoBitarray);
	if(cantidadTotalBloques <= cuantosBitsLibres()){

		//crea path (de ser necesario)
		crearPathArchivo(pathArchivo);

		//busca bloques disponibles
		int bloques[cantidadTotalBloques];

		int posicion;
		posicion = 0;

		int ocupados;
		ocupados = 0;
		while(ocupados < cantidadTotalBloques){
			if(bitarray_test_bit(bitarray, posicion) == 0){
				bitarray_set_bit(bitarray, posicion);
				bloques[ocupados] = posicion;
				ocupados ++;
			}
			posicion ++;
		}

		//paso los nuevos bloques tomados al bitarray fisico.
		actualizarBitmapHDD();
		//SIGNAL de bitarray
		pthread_mutex_unlock(&semaforoBitarray);

		char *stringBloques = string_new();
		string_append(&stringBloques,"[");
		string_append(&stringBloques,string_itoa(bloques[0]));

		int posicionBloques;
		posicionBloques = 1;

		while(posicionBloques <= (ocupados-1)){
			string_append(&stringBloques,",");
			string_append(&stringBloques,string_itoa(bloques[posicionBloques]));
			posicionBloques ++;
		}
		string_append(&stringBloques,"]");

		char *bloquesParametadata = string_from_format(stringBloques);
		metadataNewArchivo->bloques = malloc(string_length(stringBloques));
		strcpy(metadataNewArchivo->bloques, bloquesParametadata);
		free(stringBloques);

		escribirMetadata(pathArchivo, metadataNewArchivo);
		free(metadataNewArchivo->bloques);
		free(metadataNewArchivo);

		int bloquesCreados;
		bloquesCreados = 0;

		int avance;
		avance = 0;

		char *aEscribir = string_new();
		string_append(&aEscribir, stringAEscribir);

		while(bloquesCreados < (ocupados)){
			char *pathBloqueI = string_new();
			string_append(&pathBloqueI,configuracion->puntoMontaje);
			string_append(&pathBloqueI,"Bloques/");
			string_append(&pathBloqueI,string_itoa(bloques[bloquesCreados]));
			string_append(&pathBloqueI,".bin");

			FILE *bloqueI;
			bloqueI = fopen(pathBloqueI, "wb");

			char *lineaABloque = string_substring(aEscribir, avance, configuracion->tam_bloq);

			fwrite(lineaABloque, 1, strlen(lineaABloque), bloqueI);

			avance = avance + configuracion->tam_bloq;

			bloquesCreados ++;
			fclose(bloqueI);
			free(pathBloqueI);
			free(lineaABloque);
		}
		free(aEscribir);
	}else{
		log_error_mutex(loggerAtencionDAM, "Error. No hay sificientes bloques para crear archivo: %s.", pathArchivo);
		return 0;
	}
	return 1;
}

metadataArchivo * leerMetadata(char *path)
{
	FILE *archivoNuevo = fopen(path, "rb");
	if(archivoNuevo == NULL)
	{
		log_error_mutex(loggerAtencionDAM, "Error al arbir el archivo: %s para leer su metadata.", path);
	}
	metadataArchivo * metadata = malloc(sizeof(metadataArchivo));
	char stringMetadata [200];

	//TODO: Ojo, te agrego el int tamanioLeido para evitar warnings, verificar si sigue funcionando como lo esperabas. - Ale
	int tamanioLeido = fread(stringMetadata, 200, 1, archivoNuevo);
	fclose(archivoNuevo);
	if (tamanioLeido < 0)
		return NULL;
	char *token;

	token = strtok(stringMetadata, "=]");
	int i;
	i = 0;
	while (i<4) {
		if(i==1){
			metadata->tamanio = atoi(token);
		}
		if(i == 2){
			metadata->bloques = malloc(strlen(token) + 1);
			strcat(token, "]");
			memcpy(metadata->bloques, token, strlen(token) + 1);
		}
		token = strtok(NULL, "=]");
		i++;
	}
	return metadata;
}

char* obtenerDatos(char*pathArhivo,int offset,int size){

	metadataArchivo *metadataArchivoALeer = leerMetadata(pathArhivo);

	char **bloquesMetadata = string_get_string_as_array(metadataArchivoALeer->bloques);
	free(metadataArchivoALeer->bloques);
	free(metadataArchivoALeer);

	int bloqueI;
	bloqueI = 0;

	char *contenido = string_new();

	while (bloquesMetadata[bloqueI]){
		char *pathBloqueI = string_new();
		string_append(&pathBloqueI,configuracion->puntoMontaje);
		string_append(&pathBloqueI,"Bloques/");
		string_append(&pathBloqueI,bloquesMetadata[bloqueI]);
		string_append(&pathBloqueI,".bin");

		FILE *archivoBloqueI;
		archivoBloqueI = fopen(pathBloqueI, "rb");


		if(archivoBloqueI == NULL){
			log_error_mutex(loggerAtencionDAM, "Error. No se puedo arbir el bloque: %d del archivo: %s",bloqueI, pathArhivo);
		}

		fseek(archivoBloqueI, 0, SEEK_END);
		long tamanioReal = ftell(archivoBloqueI);

		char* contenidoBloque = malloc(tamanioReal+1);

		fseek(archivoBloqueI, 0, SEEK_SET);
		//TODO: Ojo, te agrego el int tamanioLeido para evitar warnings, verificar si sigue funcionando como lo esperabas. - Ale
		int tamanioLeido = fread(contenidoBloque, 1, tamanioReal, archivoBloqueI);
		if (tamanioLeido < 0)
				return NULL;
		contenidoBloque[tamanioReal] = '\0';

		string_append(&contenido,contenidoBloque);

		free(contenidoBloque);
		fclose(archivoBloqueI);
		free(pathBloqueI);
		bloqueI ++;
	}

	free(*bloquesMetadata);

	//quito los primeros caracteres no necesarios que no formen parte del length
	char*retornoOffset = string_substring_from(contenido, offset);
	//quito los ultimos caraccteres que no entran en en size.
	char*retornoFinal = string_substring_until(retornoOffset, size);
	free(contenido);
	free(retornoOffset);
	return retornoFinal;
}

void borrarArchivo(char *path){
	metadataArchivo *metadataArchivoALeer = leerMetadata(path);

	char **bloquesMetadata = string_get_string_as_array(metadataArchivoALeer->bloques);
	free(metadataArchivoALeer);

	int bloqueI;
	bloqueI = 0;

	int bloqueArray;

	//WAIT bitarray
	pthread_mutex_lock(&semaforoBitarray);
	while (bloquesMetadata[bloqueI]){
		char *pathBloqueI = string_new();
		string_append(&pathBloqueI,configuracion->puntoMontaje);
		string_append(&pathBloqueI,"Bloques/");
		string_append(&pathBloqueI,bloquesMetadata[bloqueI]);
		string_append(&pathBloqueI,".bin");

		char *bloqueABorrar = string_from_format(pathBloqueI);

		int exitoso;
		exitoso = remove(bloqueABorrar);

		 if(exitoso == 0) {
			 bloqueArray = atoi(bloquesMetadata[bloqueI]);
			 bitarray_clean_bit(bitarray, bloqueArray);
		 }else{
			 log_error_mutex(loggerAtencionDAM, "Error. No se puedo borar el bloque: %s", bloqueABorrar);
		 }
		 bloqueI ++;
	}
	//acutalizo bitarray persisitdo en emmoria con bllques liberados.
	actualizarBitmapHDD();
	//SIGNAL bitarray
	pthread_mutex_unlock(&semaforoBitarray);

	//elimnimo el arhivo metadata
	int exitosoArchivo;
	exitosoArchivo = remove(path);

	if(exitosoArchivo == 0) {
		 printf("Se elimino el archivo: %s \n", path);
	}else{
		log_error_mutex(loggerAtencionDAM, "Error. No se puedo borar el archivo: %s", path);
	}
}

void enviarStringDAMporTRansferSize(char *datosEnvio){
	//calculo cantidad de paquetes.
	int cuantosPaquetes = strlen(datosEnvio)/trasnfer_size;
	if(strlen(datosEnvio) % trasnfer_size != 0){
		cuantosPaquetes ++;
	}

	int offset = 0;
	char * bufferEnvio = malloc(trasnfer_size);
	char * p = bufferEnvio;

	while(cuantosPaquetes > 0){
		char*retornoOffset = string_substring_until(datosEnvio, offset);
		offset = offset + trasnfer_size;

		copyStringToBuffer(&p,retornoOffset);

		if(enviar(socketDAM, DAM_MDJ_OK, bufferEnvio, trasnfer_size, loggerAtencionDAM->logger)){
			log_error_mutex(loggerAtencionDAM, "Error al enviar validacion de archivo al DAM.");
			free(bufferEnvio);
		}

		p = bufferEnvio;
		cuantosPaquetes--;
	}
}

char * rebirStringDAMporTRansferSize(int cantidadPaquetes){
	t_package paquete;
	char * stringTotal = string_new();

	while(cantidadPaquetes > 0){
		recibir(socketDAM,&paquete,loggerMDJ->logger);
		if(paquete.code == DAM_MDJ_GUARDAR_DATOS){
			char * subString = copyStringFromBuffer(&paquete.data);
			string_append(&stringTotal, subString);

			free(subString);
			free(paquete.data);
		}else{
			log_error_mutex(loggerAtencionDAM, "Error se recibieron menos paquetes de los esperados.");
		}
	}

	return stringTotal;
}

void crearPathArchivo(char* pathConArchivo){

	char** substrings = string_split(pathConArchivo, "/");
	char * pathSinArchivo = string_new();

	int i = 0;
	while(substrings[i+1] != NULL){
		string_append(&pathSinArchivo, substrings[i]);
	}

	crearRutaDirectorio(pathSinArchivo);

}
