/*
 * fileFunctions.c
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#include "fileFunctions.h"

char* AUXFILE = "aux.txt";

#define MB_SIZE	(1024*1024)

off_t fsize(const char *filename, t_log *logger) {
	struct stat st;
	if (stat(filename, &st) == 0)
		return st.st_size;
	log_error(logger, "No se pudo determinar el tamanio del archivo: %s", strerror(errno));
	return -1;
}

int checkIfPathExists(char *path, t_log *logger){
	char **dir = string_split(path, "/");
	char *currentDir = string_new();
	struct stat st;
	int i, ret;
	for(i=0; dir[i+1] != NULL ;i++){
		log_trace(logger, "Parte del dir a analizar: %s", dir[i]);
		string_append(&currentDir, "/");
		string_append(&currentDir, dir[i]);
		log_trace(logger, "Chequeando si el dir existe: %s", currentDir);
		stat(currentDir, &st);
		if(!(stat(currentDir, &st)==0 && S_ISDIR(st.st_mode))){
			if(mkdir(currentDir, 0777)){
				log_error(logger, "Error en MKDIR de %s. %s\n",currentDir, strerror(errno));
				ret = EXIT_FAILURE;
				break;
			}
			log_trace(logger, "Cree dir: %s", currentDir);
		}
	}
	free(currentDir);
	freeSplit(dir);
	ret = EXIT_SUCCESS;
	return ret;
}

char* readAllFile(char *filename, t_log *logger) {
	FILE *file;
	off_t fileSize;
	char *fileContent;

	file = fopen(filename, "r");

	if (!file) {
		log_error(logger, "No se pudo abrir el archivo: %s", strerror(errno));
		return NULL;
	}
	fileSize = fsize(filename, logger);

	if (fileSize == 0) {
		fclose(file);
		return NULL;
	}
	fileContent = malloc(fileSize + 1);
	if (fileContent == NULL) {
		fclose(file);
		log_error(logger, "No se pudo alocar memoria para leer el archivo : %s",
				strerror(errno));
		return NULL;
	}
	fread(fileContent, sizeof(char), fileSize, file);
	//fileContent[fileSize] = '\0';

	if (ferror(file)) {
		fclose(file);
		log_error(logger, "Error al leer el archivo : %s", strerror(errno));
		return NULL;
	}
	fclose(file);

	return fileContent;
}

FILE* createFileWithData(char*filename, char* buffer, t_log* logger) {
	FILE *file;
	file = fopen(filename, "w");
	if (!file) {
		log_error(logger, "No se pudo abrir el archivo: %s", strerror(errno));
		return NULL;
	}
	txt_write_in_file(file, buffer);
	//fwrite(buffer, strlen(buffer), sizeof(buffer), file);
	if (ferror(file)) {
		fclose(file);
		log_error(logger, "Se produjo un error en el archivo");
		return NULL;
	}

	fclose(file);
	return file;
}

char* getNameFromFile(FILE* file) {
	char path[1024];
	char result[1024];
	int fd = fileno(file);

	sprintf(path, "/proc/self/fd/%d", fd);
	memset(result, 0, sizeof(result));
	readlink(path, result, sizeof(result) - 1);

	int fileLength = strlen(result);
	char* fileName = malloc(fileLength + 1);
	memcpy(fileName, result, fileLength);

	return fileName;
}

int appendFile(char* fileName, char* buffer, t_log* logger) {
	FILE* file = txt_open_for_append(fileName);

	if (!file) {
		log_error(logger, "No se pudo abrir el archivo: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	txt_write_in_file(file, buffer);
	if (ferror(file)) {
		fclose(file);
		log_error(logger, "Se produjo un error en el archivo");
		return EXIT_FAILURE;
	}
	txt_close_file(file);
	return EXIT_SUCCESS;
}

void deleteDirectory(char* path){
	char *archivosPath = string_new();
	char *aux;
	string_append(&archivosPath, path);
	DIR  *d = opendir(archivosPath);
	struct dirent *dir;

	if(d){
		while((dir = readdir(d))){
			aux = string_duplicate(archivosPath);
			string_append(&aux, "/");
			string_append(&aux, dir->d_name);
			remove(aux);
			free(aux);
		}
	}
	remove(archivosPath);
	free(archivosPath);
	closedir(d);
}

char* getFileName(char* path) {
	char** split = string_split(path, "/");
	int i = 0;
	for (i = 0; split[i]; i++);
	char* aux = string_duplicate(split[i - 1]);

	freeSplit(split);

	return aux;
}

