#include "FileSystem.h"

int main(int argc, char ** argv) {
	fileSystemAvtivo = 1;
	inicializarMDJ(argv[1]);
	crearFifa();

	pthread_create(&threadConsola, &tattr, (void *) inicializarCosnola, NULL); // ultimo de tod (?
	pthread_create(&threadDAM, &tattr, (void *) responderDAM, NULL); //VA DESPUES DE INICIALIZAR CONEXION,  o no va para nada.
	inicializarConexion();

	exit_gracefully(FIN_EXITOSO);
}

void inicializarMDJ(char * pathConfig){

	logger = log_create("MDJ.log", "MDJ", true, LOG_LEVEL_INFO);
	if (pathConfig != NULL){
		configuracion = cargarConfiguracion(pathConfig, MDJ, logger);
	}
	else{
		log_error(logger, "No hay un path correcto a un archivo de configuracion");
		exit_gracefully(ERROR_PATH_CONFIG);
	}
	if (configuracion == NULL){
		log_error(logger, "Error en el archivo de configuracion");
		exit_gracefully(ERROR_CONFIG);
	}

	//Imprimo lo leido en config.cfg
	printf("Configuracion inicial: \n");
	log_info(logger, "Puerto = %d", configuracion->puertoMDJ);
	log_info(logger, "Punto montaje = %s", configuracion->puntoMontaje);
	log_info(logger, "Retardo = %d", configuracion->retardo);
	log_info(logger, "IP propia = %s", configuracion->ip_propia);
	log_info(logger, "Cantidad de bloques del FS = %d", configuracion->cant_bloq);
	log_info(logger, "TamaNio de bloques del FS = %d", configuracion->tam_bloq);
	log_info(logger, "Magic number = %s", configuracion->magic_num);
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

	char *pathInicial = malloc(10240);
	getcwd(pathInicial,10240);

	char *pathArchivoMetadata = string_new();
	string_append(&pathArchivoMetadata,configuracion->puntoMontaje);
	string_append(&pathArchivoMetadata,"Metadata/metadata.bin");
	FILE *metadata;
	metadata = fopen(pathArchivoMetadata, "ab");

	if( metadata == NULL){
		log_error(logger, "Error al crear archivo metadata.bin.");
		//exit?
	}else{
		char buffer [100];
		sprintf(buffer, "TAMANIO_BLOQUES=%d \nCANTIDAD_BLOQUES=%d \nMAGIC_NUMBER=%s", configuracion->tam_bloq, configuracion->cant_bloq, configuracion->magic_num);

		int escritura = fwrite(buffer , 1 , sizeof(buffer) , metadata );

		if(escritura != sizeof(buffer)){
			log_error(logger, "Error al escribir archivo metadata.bin.");
		}else{
			log_info(logger, "Se carga archivo metadata con informacion = %s", buffer);

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
	bitarray = bitarray_create_with_mode(data,tamBitarray,LSB_FIRST); // if create falla error.

	int bit;
	bit = 0;
	while(bit <= configuracion->cant_bloq){
		bitarray_clean_bit(bitarray, bit);
		bit ++;
	}


	//checkeo si ya hay btimap previo.

	char *pathArchivoBitmap = string_new();
	string_append(&pathArchivoBitmap,configuracion->puntoMontaje);
	string_append(&pathArchivoBitmap,"Metadata/bitmap.bin");

	int status;
	status = validarArchivo(pathArchivoBitmap);
	free(pathArchivoBitmap);

	if(status){
		log_info(logger, "Se detecto un bitmap previo. Se procede a cargarlo a memoria.");

		char *pathArchivoBitmap = string_new();
		string_append(&pathArchivoBitmap,configuracion->puntoMontaje);
		string_append(&pathArchivoBitmap,"Metadata/bitmap.bin");

		FILE *bitmap;
		bitmap = fopen(pathArchivoBitmap, "rb");

		if(bitmap == NULL){
			log_error(logger, "No se puede abrir bitmap que supuestamente ya existe.");
		}else{
			int posicion;
			posicion = 0;

			char bit[2];

			while(posicion <= configuracion->cant_bloq){
				fseek(bitmap, posicion, SEEK_SET);
				fread(bit, 1, 1, bitmap);

				if(strcmp(bit, "1") == 0){
					bitarray_set_bit(bitarray, posicion);
				}
				posicion ++;
			}
		}
	}else{
		log_info(logger, "No se detecto un bitmap previo. Se procede a crearlo vacio.");
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
	if (escuchar(configuracion->puertoMDJ, &socketPropio, logger)) {
		exit_gracefully(1);
	}
	if (acceptConnection(socketPropio, &socketDAM, MDJ_HSK, &handshake, logger)) {
		log_error(logger, "No se acepta la conexion");
	}
	printf("Se conecto el DAM");
   // pthread_create(&threadDAM, &tattr, (void *) esperarInstruccionDAM, NULL);
}

void esperarInstruccionDAM(){
	while(!getExit()){

		//TODO socketEscucha no tiene nada asignado(?
		t_package paquete;
		if (recibir(socketEscucha->socket,&paquete,logger)) {
		          log_error_mutex(logger, "No se pudo recibir el mensaje.");
		         //handlerDisconnect(i);
		}
		else
		{
//			responderDAM(paquete, socketEscucha->socket);
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
				log_error(logger, "Error  al crear path: %s", ruta);
			}
			i++;
		}else{
			i++;
		}
	}
	log_info(logger, "Se creo la siguiente ruta: = %s", ruta);

}

void actualizarBitmapHDD(){
	char *pathArchivoBitmap = string_new();
	string_append(&pathArchivoBitmap,configuracion->puntoMontaje);
	string_append(&pathArchivoBitmap,"Metadata/bitmap.bin");

	FILE *bitmap;
	bitmap = fopen(pathArchivoBitmap, "wb");

	int posicion;
	posicion = 0;

	char cero[] = "0";
	char uno[] = "1";

	while(posicion <= configuracion->cant_bloq){
		 if(bitarray_test_bit(bitarray, posicion) == 0){
			 fseek(bitmap, posicion, SEEK_SET);
			 fwrite(cero, 1, sizeof(cero), bitmap);
		 }else{
			 fseek(bitmap, posicion, SEEK_SET);
			 fwrite(uno, 1, sizeof(uno), bitmap);
		 }
			posicion ++;
	}

	log_info(logger, "Bitmap persistido en memoria.");

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

	log_info(logger, "Quedan %d bloques libres.", libres);

	return libres;
}


void exit_gracefully(int error){
	if (error != ERROR_PATH_CONFIG)
	{
		free(configuracion);
	}
	log_destroy(logger);
	fileSystemAvtivo = 0;
	bitarray_destroy(bitarray);
	exit(error);
}
