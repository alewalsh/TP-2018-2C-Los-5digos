#include "FileSystem.h"


//AGREGAR RETARDO


void inicializarCosnola() {

	printf("\n Se ejecuta esta funcion del thread consola. \n");

	char * linea;

	const char s[2] = " ";
	char *token;
	int longitud;

	while (1) {
		linea = readline(">");
		if (linea)
			add_history(linea);
		if (!strncmp(linea, "exit", 4)) {
			free(linea);
			break;
		}

		printf("Readline leyo: %s\n", linea);

		longitud = strlen(linea);
		char lineaTokenisada[1][longitud];

		token = strtok(linea, s);
		int i;
		i = 0;
		while (token != NULL) {
			strcpy(lineaTokenisada[i], token);
			token = strtok(NULL, s);
			i++;
		}

		if(strcmp(lineaTokenisada[0], "ls") == 0){
			printf("LSeara: %s \n", lineaTokenisada[1]);

			DIR *directorio;
			struct dirent *file;
//			struct stat status;

			directorio = opendir(lineaTokenisada[1]);

			while((file = readdir(directorio)) != NULL){
//				stat(file->d_name, &status);
				printf("%s -- ", file->d_name);
			}

		}
		if(strcmp(lineaTokenisada[0], "cd") == 0){
			printf("CDara: %s \n", lineaTokenisada[1]);
		}
		if(strcmp(lineaTokenisada[0], "md5") == 0){
			printf("MD5eara: %s \n", lineaTokenisada[1]);
		}
		if(strcmp(lineaTokenisada[0], "cat") == 0){
			printf("CATeara: %s \n", lineaTokenisada[1]);
		}
	}
	free(linea);
}

