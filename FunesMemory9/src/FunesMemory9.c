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

int main(void) {
	char * tiempo = temporal_get_string_time();
	puts(tiempo);
	free(tiempo);

	return EXIT_SUCCESS;
}
