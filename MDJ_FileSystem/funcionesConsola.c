#include "FileSystem.h"

void inicializarCosnola() {

	char * linea;
	int ejecutado;

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

		log_info_mutex(loggerMDJ, "Readline leyo: %s\n", linea);
		ejecutado = 0;

		//obtengo primer paramentro de la entrada.
		token = strtok(linea, s);

		//LS
		if((token != NULL) && (strcmp(token, "ls") == 0)){
			ejecutado = 1;
			//obtengo segundo parametro de la entrada.
			token = strtok(NULL, s);

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
			ejecutado = 1;
			token = strtok(NULL, s);

			if(token != NULL){
				if(strncmp(token, ".", strlen(token)) == 0){
					//no hago nada
				}else{
					if(strncmp(token, "..", strlen(token)) == 0){
						log_info_mutex(loggerMDJ, "Se retrocedera una carpeta. \n");
						int substract;
						substract = lenUltimaCarpeta(directorioActual);

						int total;
						total = strlen(directorioActual);

						directorioActual = string_substring_until(directorioActual, total-substract);

					}else{
						log_info_mutex(loggerMDJ, "Se agrega %s a %s \n", token, directorioActual);
						string_append(&directorioActual, token);
						string_append(&directorioActual, "/");
					}
				}
				log_info_mutex(loggerMDJ, "Directorio actual: %s \n", directorioActual);
			}
		}

		//MD5
		if((token != NULL) && (strcmp(token, "md5") == 0)){
			ejecutado = 1;
			token = strtok(NULL, s);
			char *pathMD5 = string_new();
			string_append(&pathMD5, directorioActual);
			string_append(&pathMD5, token);

			if(token != NULL){
				log_info_mutex(loggerMDJ, "MD5eara: %s \n", token);

				int statusMd5;
				statusMd5 = validarArchivo(pathMD5);
				if (statusMd5) {
					metadataArchivo *metadataArchivoALeer = leerMetadata(pathMD5);
					const char * datosMD5 = obtenerDatos(pathMD5,0, metadataArchivoALeer->tamanio);

					unsigned char digest[MD5_DIGEST_LENGTH];
					MD5_CTX context;
					MD5_Init(&context);

					MD5_Update(&context, (const unsigned char *) datosMD5, strlen(datosMD5));
					MD5_Final(digest, &context);
					printf("MD5 de %s es: ", pathMD5);

					char md5string[MD5_DIGEST_LENGTH*2];
					for(int i = 0; i < MD5_DIGEST_LENGTH; ++i){
					    sprintf(&md5string[i*2] , "%02x", (unsigned int)digest[i]);
					}

					printf("%s\n", md5string);
					printf("\n");

					free(metadataArchivoALeer->bloques);
					free(metadataArchivoALeer);
					free(datosMD5);
				}else{
					printf("Archivo inexistente. \n");
				}
				free(pathMD5);
			}
		}

		//CAT
		if((token != NULL) && (strcmp(token, "cat") == 0)){
			ejecutado = 1;
			token = strtok(NULL, s);
			char *path = string_new();
			string_append(&path, directorioActual);
			string_append(&path, token);

			if(token != NULL){
				log_info_mutex(loggerMDJ, "CATeara: %s \n", path);

				int statusCAT;
				statusCAT = validarArchivo(path);
				if (statusCAT) {

					metadataArchivo *metadataArchivoALeer = leerMetadata(path);

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
		if(ejecutado == 0){
			printf("Comando no reconocido. \n");
		}
	}
	free(linea);
}

void eleESE(char *rutaDirectorio){

	DIR *directorio;
	struct dirent *file;
	struct stat status;

	directorio = opendir(rutaDirectorio);

	if(directorio != NULL){
		log_info_mutex(loggerMDJ, "LSeara: %s \n", rutaDirectorio);

		printf("Contenido de %s: \n \n", rutaDirectorio);

		while((file = readdir(directorio)) != NULL){
			stat(file->d_name, &status);
			if(strncmp(file->d_name, ".", 1) != 0){
				printf("%s \n", file->d_name);
			}
		}
	}else{
		printf("Ruta de directorio: %s inexistente.", rutaDirectorio);
	}
	printf("\n");
	closedir(directorio);
}

int lenUltimaCarpeta(char * direcotrioActual){

	char *stringActual= string_from_format(direcotrioActual);
	int longitudUltimaCareta = 0;

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
