/*
 ============================================================================
 Name        : FunesMemory9.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/temporal.h>

int main(void) {

	char * tiempo = temporal_get_string_time();
	puts(tiempo);
	free(tiempo);

	return EXIT_SUCCESS;
}
