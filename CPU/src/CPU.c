/*
 ============================================================================
 Name        : CPU.c
 Author      : Alejandro Walsh
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include "CPU.h"

int main(int argc, char ** argv) {
	t_log * logger = log_create("CPU.log", "CPU", true, LOG_LEVEL_INFO);
	configCPU* config = cargarConfiguracion(argv[1], CPU, logger);

	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
}
