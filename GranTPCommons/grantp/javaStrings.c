/*
 * javaStrings.c
 *
 *  Created on: 8 sep. 2018
 *      Author: utnso
 */

#include "javaStrings.h"

char* getString(char* origen, int size) {
	char* str = (char*) malloc(size + 1);
	memcpy(str, origen, size);
	str[size] = '\0';
	return str;
}

void freeSplit(char** split) {
	int i;
	for(i = 0; split[i] != NULL; i++){
		free(split[i]);
	}

	free(split[i]); //libero el espacio que tiene NULL
	free(split);
}

