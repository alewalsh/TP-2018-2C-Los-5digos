#include "FileSystem.h"

void inicializarCosnola() {

	char * linea;

	const char s[2] = " ";
	char *token;

	char * directorioActual = string_new();
	string_append(&directorioActual, configuracion->puntoMontaje);

	while (1) {
		linea = readline(">");
		if (linea)
			add_history(linea);
		if (!strncmp(linea, "exit", 4)) {
			free(linea);
			exit_gracefully(-1);
			break;
		}

		log_info(loggerMDJ, "Readline leyo: %s\n", linea);

		//obtengo primer paramentro de la entrada.
		token = strtok(linea, s);

		//LS
		if((token != NULL) && (strcmp(token, "ls") == 0)){
			//obtengo segundo parametro de la entrada.
			token = strtok(NULL, s);
			log_info(loggerMDJ, "LSeara: %s \n", token);

			if(token != NULL){
				char * directorioEleseable = string_new();
				string_append(&directorioEleseable, directorioActual);
				string_append(&directorioEleseable, token);

				eleESE(directorioEleseable);
				free(directorioEleseable);
			}else{
				eleESE(directorioActual);
			}
		}

		//CD
		if((token != NULL) && (strcmp(token, "cd") == 0)){
			token = strtok(NULL, s);

			if(token != NULL){
				if(strncmp(token, ".", strlen(token)) == 0){
					//no hago nada
				}else{
					if(strncmp(token, "..", strlen(token)) == 0){
						log_info(loggerMDJ, "Se retrocedera una carpeta. \n");
						int substract;
						substract = lenUltimaCarpeta(directorioActual);

						int total;
						total = strlen(directorioActual);

						directorioActual = string_substring_until(directorioActual, total-substract);

					}else{
						log_info(loggerMDJ, "Se agrega %s a %s \n", token, directorioActual);
						string_append(&directorioActual, token);
						string_append(&directorioActual, "/");
					}
				}
				log_info(loggerMDJ, "Directorio actual: %s \n", directorioActual);
			}
		}

		//MD5
		if((token != NULL) && (strcmp(token, "md5") == 0)){
			token = strtok(NULL, s);
			char *pathMD5 = string_new();
			string_append(&pathMD5, directorioActual);
			string_append(&pathMD5, token);

			if(token != NULL){
				log_info(loggerMDJ, "MD5eara: %s \n", token);

				int statusMd5;
				statusMd5 = validarArchivo(pathMD5);
				if (statusMd5) {
					struct metadataArchivo *metadataArchivoALeer = malloc(sizeof(struct metadataArchivo));
					leerMetadata(pathMD5, metadataArchivoALeer);

					char * datosMD5 = obtenerDatos(pathMD5,0, metadataArchivoALeer->tamanio);

					void * digest = malloc(MD5_DIGEST_LENGTH); //o puede ser unsigned char*
					MD5_CTX context;
					MD5_Init(&context);
					MD5_Update(&context, datosMD5, strlen(datosMD5) + 1);
					MD5_Final(digest, &context);

//					char * md5EnChar = digest;
//
					printf("MD5 hash = (%d) \n", digest);
//					int i;
//					for (i=0;i<MD5_DIGEST_LENGTH;i++){
//					    printf ("%02x", digest[i]);
//					}

					free(metadataArchivoALeer->bloques);
					free(metadataArchivoALeer);
					free(datosMD5);
				}
				free(pathMD5);
			}
		}

		//CAT
		if((token != NULL) && (strcmp(token, "cat") == 0)){
			token = strtok(NULL, s);
			char *path = string_new();
			string_append(&path, directorioActual);
			string_append(&path, token);

			if(token != NULL){
				log_info(loggerMDJ, "CATeara: %s \n", path);

				int statusCAT;
				statusCAT = validarArchivo(path);
				if (statusCAT) {

					struct metadataArchivo *metadataArchivoALeer = malloc(sizeof(struct metadataArchivo));
					leerMetadata(path, metadataArchivoALeer);

					char * datos = obtenerDatos(path,0, metadataArchivoALeer->tamanio);

					printf("%s", datos);

					free(metadataArchivoALeer->bloques);
					free(metadataArchivoALeer);
					free(datos);
				}
				else{
					printf("Archivo inexistente.\n");
				}
			}
		free(path);
		}
	}
	free(linea);
}

void eleESE(char *rutaDirectorio){
	printf("Contenido de %s: \n \n", rutaDirectorio);

	DIR *directorio;
	struct dirent *file;
	struct stat status;

	directorio = opendir(rutaDirectorio);

	while((file = readdir(directorio)) != NULL){
		stat(file->d_name, &status);
		if(strncmp(file->d_name, ".", 1) != 0){
			printf("%s \n", file->d_name);
		}
	}
	printf("\n");
	closedir(directorio);
}

int lenUltimaCarpeta(char * direcotrioActual){

	char *stringActual= string_from_format(direcotrioActual);
	int longitudUltimaCareta;

	const char s[2] = "/";
	char *token;

	token = strtok(stringActual, s);

	while(token != NULL){
		longitudUltimaCareta = strlen(token);
		token = strtok(NULL, s);
	}

	longitudUltimaCareta ++; //por el '/'
	free(stringActual);

	return longitudUltimaCareta;

}
