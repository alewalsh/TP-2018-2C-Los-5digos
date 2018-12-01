#include "FileSystem.h"

int main(int argc, char ** argv) {

	fileSystemAvtivo = 1;
	inicializarMDJ(argv[1]);
	crearFifa();

	pthread_create(&threadDAM, &tattr, (void *) responderDAM, NULL); //No va.
	pthread_create(&threadConsola, &tattr, (void *) inicializarCosnola, NULL); // ultimo de tod (?
	inicializarConexion();

	exit_gracefully(FIN_EXITOSO);
}

void inicializarMDJ(char * pathConfig){

	loggerMDJ = log_create("MDJ.log", "MDJ", true, LOG_LEVEL_INFO);
	loggerAtencionDAM = log_create("FS.log", "FS", true, LOG_LEVEL_ERROR);
	pthread_mutex_init(&semaforoBitarray, NULL);

	if (pathConfig != NULL){
		configuracion = cargarConfiguracion(pathConfig, MDJ, loggerMDJ);
	}
	else{
		log_error(loggerMDJ, "No hay un path correcto a un archivo de configuracion");
		exit_gracefully(ERROR_PATH_CONFIG);
	}
	if (configuracion == NULL){
		log_error(loggerMDJ, "Error en el archivo de configuracion");
		exit_gracefully(ERROR_CONFIG);
	}

	//Imprimo lo leido en config.cfg
	printf("Configuracion inicial: \n");
	log_info(loggerMDJ, "Puerto = %d", configuracion->puertoMDJ);
	log_info(loggerMDJ, "Punto montaje = %s", configuracion->puntoMontaje);
	log_info(loggerMDJ, "Retardo = %d", configuracion->retardo);
	log_info(loggerMDJ, "IP propia = %s", configuracion->ip_propia);
	log_info(loggerMDJ, "Cantidad de bloques del FS = %d", configuracion->cant_bloq);
	log_info(loggerMDJ, "TamaNio de bloques del FS = %d", configuracion->tam_bloq);
	log_info(loggerMDJ, "Magic number = %s", configuracion->magic_num);
}

void crearFifa(){

	//creo punto de montaje
	crearRutaDirectorio(configuracion->puntoMontaje);

	//creo metadata
	char *pathMetadata = string_new();
	string_append(&pathMetadata,configuracion->puntoMontaje);
	string_append(&pathMetadata,"Metadata");
	crearRutaDirectorio(pathMetadata);
	free(pathMetadata);

	//al pedo?
	char *pathInicial = malloc(10240);
	getcwd(pathInicial,10240);

	char *pathArchivoMetadata = string_new();
	string_append(&pathArchivoMetadata,configuracion->puntoMontaje);
	string_append(&pathArchivoMetadata,"Metadata/Metadata.bin");
	FILE *metadata;
	metadata = fopen(pathArchivoMetadata, "wb");

	if( metadata == NULL){
		log_error(loggerMDJ, "Error al crear archivo metadata.bin.");
		//exit?
	}else{
		char buffer [100];
		sprintf(buffer, "TAMANIO_BLOQUES=%d \nCANTIDAD_BLOQUES=%d \nMAGIC_NUMBER=%s", configuracion->tam_bloq, configuracion->cant_bloq, configuracion->magic_num);

		int escritura = fwrite(buffer , 1 , sizeof(buffer) , metadata );

		if(escritura != sizeof(buffer)){
			log_error(loggerMDJ, "Error al escribir archivo metadata.bin.");
		}else{
			log_info(loggerMDJ, "Se carga archivo metadata con informacion = %s", buffer);

			fclose(metadata);
			free(pathArchivoMetadata);
		}
	}

	//creo bitmap
	int tamBitarray = configuracion->cant_bloq/8;
	if(configuracion->cant_bloq % 8 != 0){
		tamBitarray++;
	}

	char* data=malloc(tamBitarray);
	bitarray = bitarray_create_with_mode(data,tamBitarray,MSB_FIRST); // if create falla error.

	int bit;
	bit = 0;
	while(bit <= configuracion->cant_bloq){
		bitarray_clean_bit(bitarray, bit);
		bit ++;
	}


	//checkeo si ya hay btimap previo.

	char *pathArchivoBitmap = string_new();
	string_append(&pathArchivoBitmap,configuracion->puntoMontaje);
	string_append(&pathArchivoBitmap,"Metadata/Bitmap.bin");

	int status;
	status = validarArchivo(pathArchivoBitmap);
	free(pathArchivoBitmap);

	if(status){
		log_info(loggerMDJ, "Se detecto un bitmap previo. Se procede a cargarlo a memoria.");

		char *pathArchivoBitmap = string_new();
		string_append(&pathArchivoBitmap,configuracion->puntoMontaje);
		string_append(&pathArchivoBitmap,"Metadata/Bitmap.bin");

		FILE *bitmap;
		bitmap = fopen(pathArchivoBitmap, "rb");

		if(bitmap == NULL){
			log_error(loggerMDJ, "No se puede abrir bitmap que supuestamente ya existe.");
		}else{
			int posicion;
			posicion = 0;

			char * bitarrayCompleto = malloc(configuracion->cant_bloq);

			fseek(bitmap, 0, SEEK_SET);
			fread(bitarrayCompleto, 1, configuracion->cant_bloq, bitmap);

			while(posicion <= configuracion->cant_bloq){
				if((bitarrayCompleto[BIT_CHAR(posicion)] & _bit_in_char(posicion, bitarray->mode)) != 0){
					bitarray_set_bit(bitarray, posicion);
				}

				posicion ++;
			}
		}
	}else{
		log_info(loggerMDJ, "No se detecto un bitmap previo. Se procede a crearlo vacio.");
		actualizarBitmapHDD();
	}

	//creo base archivos
	char *pathArchivos = string_new();
	string_append(&pathArchivos,configuracion->puntoMontaje);
	string_append(&pathArchivos,"Archivos");
	crearRutaDirectorio(pathArchivos);
	free(pathArchivos);

	//creo base data
	char *pathBloques = string_new();
	string_append(&pathBloques,configuracion->puntoMontaje);
	string_append(&pathBloques,"Bloques");
	crearRutaDirectorio(pathBloques);
	free(pathBloques);
}

// ****CONEXION****

void inicializarConexion(){
	int * socketPropio;
    uint16_t handshake;
	if (escuchar(configuracion->puertoMDJ, &socketPropio, loggerMDJ)) {
		exit_gracefully(1);
	}
	if (acceptConnection(socketPropio, &socketDAM, MDJ_HSK, &handshake, loggerMDJ)) {
		log_error(loggerMDJ, "No se acepta la conexion");
	}
	socketEscucha = inicializarTSocket(*socketPropio, loggerMDJ);

	//recibo transer size
	t_package paqueteTS;
	if(recibir(socketEscucha->socket,&paqueteTS,loggerMDJ)){
		log_error(loggerMDJ, "Error al esperar recivir Transfer Size");
	}
	if(paqueteTS.code != DAM_MDJ_TRANSFER_SIZE){
		log_error(loggerMDJ, "No se recivio el Transfer Size");
		exit(-1);
	}
	char *bufferTS = paqueteTS.data;
	trasnfer_size = copyIntFromBuffer(&bufferTS);

	free(bufferTS);

	printf("Se conecto el DAM");

    pthread_create(&threadDAM, &tattr, (void *) esperarInstruccionDAM, NULL);
}

void esperarInstruccionDAM(){
	while(!getExit()){
		t_package paquete;
		if (recibir(socketEscucha->socket,&paquete,loggerMDJ)) {
			log_error_mutex(loggerMDJ, "No se pudo recibir el mensaje.");
			//handlerDisconnect(i);
		}
		else{
			//TODO
//			pthread_create(&threadDAM, &tattr, (void *) responderDAM, NULL); //funciona con el mismo nombre de thread?? Donde va el paquete?
			responderDAM(paquete); //todo en thread distinto por cada recibir.
		}
	}
}

// ****ADMINISTRATIVAS****

void crearRutaDirectorio(char *ruta){
	//MANDAR SIEMPRE CON EL PUNTO DE MONTAJE.

	char **carpetas = string_split(ruta, "/");

	int i = 0;
	int a = 0;
	char *pathCompleto = string_new();

	struct stat buffer;

	while((carpetas[i] != NULL) && (a == 0)){
		string_append(&pathCompleto, "/");
		string_append(&pathCompleto,carpetas[i]);
		if(stat(pathCompleto, &buffer) != 0){
			a = mkdir(pathCompleto, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			if(a != 0){
				log_error(loggerAtencionDAM, "Error  al crear path: %s", ruta);
			}
			i++;
		}else{
			i++;
		}
	}
	log_info(loggerAtencionDAM, "Se creo la siguiente ruta: = %s", ruta);

}

void actualizarBitmapHDD(){
	char *pathArchivoBitmap = string_new();
	string_append(&pathArchivoBitmap,configuracion->puntoMontaje);
	string_append(&pathArchivoBitmap,"Metadata/Bitmap.bin");

	FILE *bitmap;
	bitmap = fopen(pathArchivoBitmap, "wb");

	fseek(bitmap, 0, SEEK_SET);
	fwrite(bitarray->bitarray, 1, bitarray->size, bitmap);

	log_info(loggerAtencionDAM, "Bitmap persistido en memoria.");

	fclose(bitmap);
	free(pathArchivoBitmap);
}

int cuantosBitsLibres(){
	int posicion;
	posicion = 0;
	int libres;
	libres = 0;
	while(posicion < configuracion->cant_bloq){
		if(bitarray_test_bit(bitarray, posicion) == 0){
			libres ++;
		}
		posicion ++;
	}

	libres --; //TODO ver porque me deveulve uno mas (???

	log_info(loggerAtencionDAM, "Quedan %d bloques libres.", libres);

	return libres;
}

void exit_gracefully(int error){
	if (error != ERROR_PATH_CONFIG)
	{
		free(configuracion);
	}
	log_destroy(loggerMDJ);
	log_destroy(loggerAtencionDAM);
	fileSystemAvtivo = 0;
	bitarray_destroy(bitarray);
	exit(error);
}
