#include "FileSystem.h"

//todo ver envio por transfer size.

//TODO CAMBIAR CASES POR CODIGOS EN SOCKET.H

//calloc para cuando leo metadata?

void consoleExit() {
	setExit();
}

void responderDAM() {
//void responderDAM(t_package pkg) {

	sleep(configuracion->retardo/1000);

//	int opcion = pkg.code; //sera asignado por escuchar al DAM

	int opcion;
	opcion = 2;

	switch (opcion) {
	case 1: //validar archivo ++ [Path]
		printf("");

		//TODO ver como consigo el filename. con path incluido. o estan todos si o si dentro de /archivos?
		//en pkg.data ???

		char *pathValidar = string_new();

//		char *path = pkg.data;

		int statusValidar;
		statusValidar = validarArchivo(pathValidar);
		if (statusValidar) {
			log_info(loggerAtencionDAM, "Archivo: %s OK.", pathValidar);
			//madnar ok
			//mandar cuanto pesa realmente.

//			enviar(socketEscucha->socket, DAM_MDJ_OK, , ,logger);
		} else {
			//mandar fail
			enviar(socketEscucha->socket, DAM_MDJ_FAIL, NULL, 0,loggerAtencionDAM);
			log_error(loggerAtencionDAM, "Error. Path: %s inexistente.", pathValidar);
		}

		free(pathValidar);

		break;

	case 2: //crear archivo ++ [Path, N Cantidad de /n's]
		printf("");

		int cantidad_Ns; //paramentros de la funcion

		char *pathArchivoNuevo = string_new();

		//creo String de /n's a ser escrito
		char *NcantidadDeNs = string_new();
		int i;
		i=0;
		while(i < cantidad_Ns){
			string_append(&NcantidadDeNs, "\n");
			i++;
		}

		escribirStringEnArchivo(pathArchivoNuevo, NcantidadDeNs);

		//todo validar archivo para ver si esta todo bien? yo o el dam?
		free(pathArchivoNuevo);

		break;

	case 3: //obtener datos ++ [Path, Offset, Size]
		printf("");
		char *pathArchivoALeer = string_new();

		int offset;

		int size;

		int statusObtener;
		statusObtener = validarArchivo(pathArchivoALeer);
		if (statusObtener) {
			//obtengo datos
			char *datosArchivo = obtenerDatos(pathArchivoALeer,offset,size);
			//todo envio al DAM
			log_info(loggerAtencionDAM, "Datos obtenidos de: %s son: %s", pathArchivoALeer, datosArchivo);
		} else {
			//manda fail
			log_error(loggerAtencionDAM, "Error no se pudo leer de: &s", pathArchivoALeer);
			enviar(socketEscucha->socket, DAM_MDJ_FAIL, NULL, 0,loggerAtencionDAM);
		}
		free(pathArchivoALeer);
		break;

	case 4: //guardar datos ++ [Path, Offset, Size, Buffer]
		printf("");
		char *pathArchivoAModificar = string_new();

		int offsetGuardar;

		int sizeGuardar;

		char * bufferGuardar = string_new();

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

//			char *stringAescribir = string_from_format(stringModificado);

			//borro archivo anterior.
			borrarArchivo(pathArchivoAModificar);
			//creo nuevo archivo
			escribirStringEnArchivo(pathArchivoAModificar, stringModificado);

			log_info(loggerAtencionDAM, "Datos guardados de: %s\n Antiguos: %s\n Nuevos: %s\n", pathArchivoAModificar, datosArchivo, stringModificado);

		} else {
			log_error(loggerAtencionDAM, "Error no se pudo leer de: &s", pathArchivoALeer);
			enviar(socketEscucha->socket, DAM_MDJ_FAIL, NULL, 0,loggerAtencionDAM);
		}
		free(pathArchivoAModificar);
		free(bufferGuardar);
		break;

	case 5: //borrar archivo ++ [Path]
		printf("");
		char *pathArchivoAEliminar = string_new();

		borrarArchivo(pathArchivoAEliminar);

		int statusEliminiar;
		statusEliminiar = validarArchivo(pathArchivoAEliminar);
		if (statusEliminiar) {
			log_error(loggerAtencionDAM, "Error no se pudo borrar: %s", pathArchivoAEliminar);
		}else{
			log_info(loggerAtencionDAM, "Archivo: %s Borrado\n", pathArchivoAEliminar);
		}
		//todo validar archivo para ver si esta todo bien?
		free(pathArchivoAEliminar);
		break;
	}
}

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

void escribirStringEnArchivo(char *pathArchivo, char *stringAEscribir) {

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
		log_error(loggerAtencionDAM, "Error. No hay sificientes bloques para crear archivo: %s.", pathArchivo);
	}
}

void leerMetadata(char *path, struct metadataArchivo *metadata) {
	FILE *archivoNuevo;
	archivoNuevo = fopen(path, "rb");
	if(archivoNuevo == NULL){
		log_error(loggerAtencionDAM, "Error al arbir el archivo: %s para leer su metadata.", path);
	}

	char stringMetadata [200];

	fread(stringMetadata, 200, 1, archivoNuevo);
	fclose(archivoNuevo);

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
			strcpy(metadata->bloques, token);
		}
		token = strtok(NULL, "=]");
		i++;
	}
}

char* obtenerDatos(char*pathArhivo,int offset,int size){

	struct metadataArchivo *metadataArchivoALeer = malloc(sizeof(struct metadataArchivo));
	leerMetadata(pathArhivo, metadataArchivoALeer);

	char **bloquesMetadata = string_get_string_as_array(metadataArchivoALeer->bloques);
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
			log_error(loggerAtencionDAM, "Error. No se puedo arbir el bloque: %d del archivo: %s",bloqueI, pathArhivo);
		}

		char *contenidoBloque = malloc(configuracion->tam_bloq);

		fread(contenidoBloque, configuracion->tam_bloq, 1, archivoBloqueI);

		string_append(&contenido,contenidoBloque);

		bloqueI ++;
		fclose(archivoBloqueI);
		free(pathBloqueI);
		free(contenidoBloque);
	}

	char*retorno = string_from_format(contenido);
	free(contenido);
	return retorno;
}

void borrarArchivo(char *path){
	struct metadataArchivo *metadataArchivoALeer = malloc(sizeof(struct metadataArchivo));
	leerMetadata(path, metadataArchivoALeer);

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
			log_error(loggerAtencionDAM, "Error. No se puedo borar el bloque: %s", bloqueABorrar);
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
		log_error(loggerAtencionDAM, "Error. No se puedo borar el archivo: %s", path);
	}
}
